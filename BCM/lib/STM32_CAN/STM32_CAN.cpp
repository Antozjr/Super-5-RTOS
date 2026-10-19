/**
 * STM32_CAN.cpp - Implementación del wrapper bxCAN para STM32F103
 *
 * Registros accedidos directamente (CMSIS). No depende de HAL para
 * mantener la latencia de interrupción baja y evitar conflictos con
 * el framework Arduino/STM32duino.
 *
 * PINs: CAN_RX = PB8 (remapeado), CAN_TX = PB9 (remapeado)
 */

#include "STM32_CAN.h"
#include <string.h>

// Incluir CMSIS para STM32F1
#include <stm32f1xx.h>

// =============================================================================
// INTERNOS
// =============================================================================

static STM32_CAN_RxCb_t  s_rx_callback  = NULL;
static STM32_CAN_Frame_t s_rx_queue[STM32_CAN_QUEUE_SIZE];
static volatile uint8_t  s_rx_head      = 0;
static volatile uint8_t  s_rx_tail      = 0;

// Timing CAN: BS1=13tq, BS2=4tq → periodo = 18tq (punto de muestra al 77%)
// BitRate = APB1 / (prescaler × 18)
// Prescaler calculado dinámicamente en STM32_CAN_Init() según baud_rate.
// APB1 en STM32F103 @72MHz con PPRE1=/2: 36 MHz.
#define CAN_APB1_HZ     36000000UL
#define CAN_BS1_TICKS   13U
#define CAN_BS2_TICKS   4U
#define CAN_BS1         (((CAN_BS1_TICKS - 1U)) << 16)
#define CAN_BS2         (((CAN_BS2_TICKS - 1U)) << 20)

// =============================================================================
// HELPERS COLA RX (operaciones atómicas: se llaman desde ISR)
// =============================================================================

static inline bool queue_push(const STM32_CAN_Frame_t* f) {
    uint8_t next = (s_rx_head + 1) % STM32_CAN_QUEUE_SIZE;
    if (next == s_rx_tail) return false;  // Cola llena
    s_rx_queue[s_rx_head] = *f;
    s_rx_head = next;
    return true;
}

static inline bool queue_pop(STM32_CAN_Frame_t* f) {
    if (s_rx_tail == s_rx_head) return false;  // Cola vacía
    *f = s_rx_queue[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % STM32_CAN_QUEUE_SIZE;
    return true;
}

// =============================================================================
// INIT
// =============================================================================

uint8_t STM32_CAN_Init(uint32_t baud_rate) {
    uint32_t prescaler = CAN_APB1_HZ / (baud_rate * (1U + CAN_BS1_TICKS + CAN_BS2_TICKS));
    if (prescaler == 0) prescaler = 1;

    // 1. Habilitar clocks: CAN1, GPIOB, AFIO
    RCC->APB1ENR  |= RCC_APB1ENR_CAN1EN;
    RCC->APB2ENR  |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    // 2. Remap CAN → PB8/PB9 (AFIO_MAPR bit[14:13] = 10)
    AFIO->MAPR &= ~AFIO_MAPR_CAN_REMAP;
    AFIO->MAPR |=  AFIO_MAPR_CAN_REMAP_REMAP2;

    // 3. Configurar PB8 (CAN_RX) como entrada flotante
    GPIOB->CRH &= ~(0xF << 0);
    GPIOB->CRH |=  (0x4 << 0);   // Input floating

    // 4. Configurar PB9 (CAN_TX) como salida AF push-pull 50 MHz
    GPIOB->CRH &= ~(0xF << 4);
    GPIOB->CRH |=  (0xB << 4);   // AF output push-pull 50 MHz

    // 5. Salir de modo sleep (estado por defecto tras reset)
    CAN1->MCR &= ~CAN_MCR_SLEEP;
    uint32_t timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_SLAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    // 6. Entrar en modo inicialización
    CAN1->MCR |= CAN_MCR_INRQ;
    timeout = 1000000;
    while (!(CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    // 6. Configurar timing + loopback para test interno
    CAN1->BTR = (prescaler - 1U) | CAN_BS1 | CAN_BS2 | CAN_BTR_LBKM;

    // 7. ABOM activo (recuperación automática bus-off), sin silent
    CAN1->MCR &= ~(CAN_MCR_TTCM | CAN_MCR_AWUM | CAN_MCR_NART |
                   CAN_MCR_RFLM | CAN_MCR_TXFP);
    CAN1->MCR |=  CAN_MCR_ABOM;

    // 8. Filtro pasatodo
    CAN1->FMR  |=  CAN_FMR_FINIT;
    CAN1->FA1R &= ~(1 << 0);
    CAN1->FS1R |=  (1 << 0);
    CAN1->FM1R &= ~(1 << 0);
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FA1R |=  (1 << 0);
    CAN1->FMR  &= ~CAN_FMR_FINIT;

    // 9. Salir de init → modo loopback activo
    CAN1->MCR &= ~CAN_MCR_INRQ;
    timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    // 10. Test loopback: enviar frame de prueba y verificar recepción
    CAN1->sTxMailBox[0].TIR  = (0x7FEU << 21);   // ID=0x7FE, no RTR
    CAN1->sTxMailBox[0].TDTR = 1;                 // DLC=1
    CAN1->sTxMailBox[0].TDLR = 0xAB;              // data[0]=0xAB
    CAN1->sTxMailBox[0].TDHR = 0;
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;    // Solicitar TX

    uint32_t t = 100000;
    while (!(CAN1->RF0R & CAN_RF0R_FMP0) && --t); // Esperar frame en FIFO0

    bool loopback_ok = (t > 0) &&
                       ((CAN1->sFIFOMailBox[0].RIR >> 21) == 0x7FE) &&
                       ((CAN1->sFIFOMailBox[0].RDLR & 0xFF) == 0xAB);
    CAN1->RF0R |= CAN_RF0R_RFOM0;  // Liberar FIFO

    if (!loopback_ok) return STM32_CAN_ERR_TIMEOUT;  // Periférico no funciona

    // 11. Volver a init para quitar LBKM → modo normal
    CAN1->MCR |= CAN_MCR_INRQ;
    timeout = 1000000;
    while (!(CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    CAN1->BTR = (prescaler - 1U) | CAN_BS1 | CAN_BS2;  // Sin LBKM

    // 12. Habilitar interrupción FIFO0
    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5);
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

    // 13. Salir de init → modo normal
    CAN1->MCR &= ~CAN_MCR_INRQ;
    timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    return STM32_CAN_OK;
}

// =============================================================================
// FILTRO
// =============================================================================

void STM32_CAN_SetFilter(uint16_t filter_id, uint16_t mask) {
    CAN1->FMR  |=  CAN_FMR_FINIT;
    CAN1->FA1R &= ~(1 << 0);
    // Frame estándar: ID en bits [31:21] del FR1 (modo 32 bits, máscara)
    CAN1->sFilterRegister[0].FR1 = ((uint32_t)filter_id << 21);
    CAN1->sFilterRegister[0].FR2 = ((uint32_t)mask      << 21);
    CAN1->FA1R |=  (1 << 0);
    CAN1->FMR  &= ~CAN_FMR_FINIT;
}

// =============================================================================
// ENVÍO
// =============================================================================

uint8_t STM32_CAN_Send(const STM32_CAN_Frame_t* frame, uint32_t timeout_ms) {
    if (!frame || frame->dlc > 8) return STM32_CAN_ERR_PARAM;

    // Buscar mailbox libre
    uint32_t mbox = 0xFF;
    uint32_t start = millis();
    do {
        uint32_t tsr = CAN1->TSR;
        if (tsr & CAN_TSR_TME0) { mbox = 0; break; }
        if (tsr & CAN_TSR_TME1) { mbox = 1; break; }
        if (tsr & CAN_TSR_TME2) { mbox = 2; break; }
    } while ((millis() - start) < timeout_ms);

    if (mbox == 0xFF) return STM32_CAN_ERR_TIMEOUT;

    CAN_TxMailBox_TypeDef* mb = &CAN1->sTxMailBox[mbox];

    // ID estándar
    mb->TIR  = (frame->id << 21) | (frame->rtr ? CAN_TI0R_RTR : 0);
    mb->TDTR = frame->dlc & 0x0F;

    // Datos (little-endian, 4 bytes por registro)
    mb->TDLR = ((uint32_t)frame->data[0])        |
               ((uint32_t)frame->data[1] <<  8)  |
               ((uint32_t)frame->data[2] << 16)  |
               ((uint32_t)frame->data[3] << 24);
    mb->TDHR = ((uint32_t)frame->data[4])        |
               ((uint32_t)frame->data[5] <<  8)  |
               ((uint32_t)frame->data[6] << 16)  |
               ((uint32_t)frame->data[7] << 24);

    // Solicitar transmisión
    mb->TIR |= CAN_TI0R_TXRQ;

    return STM32_CAN_OK;
}

// =============================================================================
// RECEPCIÓN
// =============================================================================

uint8_t STM32_CAN_Receive(STM32_CAN_Frame_t* out_frame, uint32_t timeout_ms) {
    uint32_t start = millis();
    do {
        if (queue_pop(out_frame)) return STM32_CAN_OK;
        if (timeout_ms == 0) break;
        delay(1);
    } while ((millis() - start) < timeout_ms);
    return STM32_CAN_ERR_TIMEOUT;
}

void STM32_CAN_RegisterRxCallback(STM32_CAN_RxCb_t cb) {
    s_rx_callback = cb;
}

uint8_t STM32_CAN_RxAvailable(void) {
    return (s_rx_head >= s_rx_tail)
        ? (s_rx_head - s_rx_tail)
        : (STM32_CAN_QUEUE_SIZE - s_rx_tail + s_rx_head);
}

// =============================================================================
// ERRORES
// =============================================================================

uint32_t STM32_CAN_GetErrorStatus(void) {
    return CAN1->ESR;
}

void STM32_CAN_RecoverBusOff(void) {
    // Volver al modo inicialización y salir fuerza la recuperación bus-off
    CAN1->MCR |=  CAN_MCR_INRQ;
    uint32_t t = 100000; while (--t);
    CAN1->MCR &= ~CAN_MCR_INRQ;
}

// =============================================================================
// ISR - CAN1 RX FIFO0
// =============================================================================

extern "C" void USB_LP_CAN1_RX0_IRQHandler(void) {
    while (CAN1->RF0R & CAN_RF0R_FMP0) {
        STM32_CAN_Frame_t f = {};
        CAN_FIFOMailBox_TypeDef* mb = &CAN1->sFIFOMailBox[0];

        f.id  = (mb->RIR >> 21) & 0x7FF;
        f.rtr = (mb->RIR & CAN_RI0R_RTR) != 0;
        f.dlc = mb->RDTR & 0x0F;

        f.data[0] = (mb->RDLR >>  0) & 0xFF;
        f.data[1] = (mb->RDLR >>  8) & 0xFF;
        f.data[2] = (mb->RDLR >> 16) & 0xFF;
        f.data[3] = (mb->RDLR >> 24) & 0xFF;
        f.data[4] = (mb->RDHR >>  0) & 0xFF;
        f.data[5] = (mb->RDHR >>  8) & 0xFF;
        f.data[6] = (mb->RDHR >> 16) & 0xFF;
        f.data[7] = (mb->RDHR >> 24) & 0xFF;

        // Liberar FIFO
        CAN1->RF0R |= CAN_RF0R_RFOM0;

        // Invocar callback si está registrado
        if (s_rx_callback) s_rx_callback(&f);

        // Encolar para lectura desde tarea
        queue_push(&f);
    }
}
