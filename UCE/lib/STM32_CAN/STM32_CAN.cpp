/**
 * STM32_CAN.cpp - Implementación bxCAN para STM32F405RG (nodo UCE)
 *
 * El STM32F405 tiene el mismo periférico bxCAN que el F103 pero con
 * diferente mapa de registros CMSIS (stm32f4xx.h) y clock APB1 a 42 MHz.
 *
 * Timing para 500 kbps @ APB1=42 MHz:
 *   prescaler=6, BS1=TQ8, BS2=TQ5 → 14 tq → 500 kbps
 *
 * PINs: CAN1_RX = PB8, CAN1_TX = PB9 (AF CAN1, mismo que F103)
 *
 * PINs ignición: IGN_3=PE9 (Timer1 CH1), IGN_4=PE10 (Timer1 CH2). Sin conflicto con CAN.
 */

#include "STM32_CAN.h"
#include <string.h>
#include <stm32f4xx.h>

static STM32_CAN_RxCb_t  s_rx_callback  = NULL;
static STM32_CAN_Frame_t s_rx_queue[STM32_CAN_QUEUE_SIZE];
static volatile uint8_t  s_rx_head      = 0;
static volatile uint8_t  s_rx_tail      = 0;

// 500 kbps @ APB1=42 MHz: prescaler=6, BS1=8tq, BS2=5tq → 14 tq
#define CAN_PRESCALER   6
#define CAN_BS1         CAN_BTR_TS1_7    // 8 tq
#define CAN_BS2         CAN_BTR_TS2_4    // 5 tq

static inline bool queue_push(const STM32_CAN_Frame_t* f) {
    uint8_t next = (s_rx_head + 1) % STM32_CAN_QUEUE_SIZE;
    if (next == s_rx_tail) return false;
    s_rx_queue[s_rx_head] = *f;
    s_rx_head = next;
    return true;
}

static inline bool queue_pop(STM32_CAN_Frame_t* f) {
    if (s_rx_tail == s_rx_head) return false;
    *f = s_rx_queue[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % STM32_CAN_QUEUE_SIZE;
    return true;
}

uint8_t STM32_CAN_Init(uint32_t baud_rate) {
    (void)baud_rate;

    // Habilitar clocks: CAN1, GPIOB
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // PB8 → CAN1_RX (AF9), PB9 → CAN1_TX (AF9)
    GPIOB->MODER   &= ~(0xF << 16); GPIOB->MODER   |= (0xA << 16);  // AF
    GPIOB->OSPEEDR |=  (0xF << 16);                                    // High speed
    GPIOB->AFR[1]  &= ~(0xFF << 0); GPIOB->AFR[1]  |= (0x99 << 0);  // AF9

    // Modo inicialización
    CAN1->MCR |= CAN_MCR_INRQ;
    uint32_t timeout = 1000000;
    while (!(CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    CAN1->BTR = (CAN_PRESCALER - 1) | CAN_BS1 | CAN_BS2;
    CAN1->MCR &= ~(CAN_MCR_TTCM | CAN_MCR_ABOM | CAN_MCR_AWUM |
                   CAN_MCR_NART | CAN_MCR_RFLM | CAN_MCR_TXFP);

    // Filtro pasatodo
    CAN1->FMR |= CAN_FMR_FINIT;
    CAN1->FA1R &= ~(1<<0); CAN1->FS1R |= (1<<0); CAN1->FM1R &= ~(1<<0);
    CAN1->sFilterRegister[0].FR1 = 0; CAN1->sFilterRegister[0].FR2 = 0;
    CAN1->FA1R |= (1<<0); CAN1->FMR &= ~CAN_FMR_FINIT;

    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_SetPriority(CAN1_RX0_IRQn, 5);
    NVIC_EnableIRQ(CAN1_RX0_IRQn);

    CAN1->MCR &= ~CAN_MCR_INRQ;
    timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_INAK) && --timeout);
    return timeout ? STM32_CAN_OK : STM32_CAN_ERR_TIMEOUT;
}

void STM32_CAN_SetFilter(uint16_t filter_id, uint16_t mask) {
    CAN1->FMR |= CAN_FMR_FINIT; CAN1->FA1R &= ~(1<<0);
    CAN1->sFilterRegister[0].FR1 = ((uint32_t)filter_id << 21);
    CAN1->sFilterRegister[0].FR2 = ((uint32_t)mask      << 21);
    CAN1->FA1R |= (1<<0); CAN1->FMR &= ~CAN_FMR_FINIT;
}

uint8_t STM32_CAN_Send(const STM32_CAN_Frame_t* frame, uint32_t timeout_ms) {
    if (!frame || frame->dlc > 8) return STM32_CAN_ERR_PARAM;
    uint32_t mbox = 0xFF, start = millis();
    do {
        uint32_t tsr = CAN1->TSR;
        if (tsr & CAN_TSR_TME0) { mbox = 0; break; }
        if (tsr & CAN_TSR_TME1) { mbox = 1; break; }
        if (tsr & CAN_TSR_TME2) { mbox = 2; break; }
    } while ((millis() - start) < timeout_ms);
    if (mbox == 0xFF) return STM32_CAN_ERR_TIMEOUT;
    CAN_TxMailBox_TypeDef* mb = &CAN1->sTxMailBox[mbox];
    mb->TIR  = (frame->id << 21) | (frame->rtr ? CAN_TI0R_RTR : 0);
    mb->TDTR = frame->dlc & 0x0F;
    mb->TDLR = frame->data[0] | (frame->data[1]<<8) | (frame->data[2]<<16) | (frame->data[3]<<24);
    mb->TDHR = frame->data[4] | (frame->data[5]<<8) | (frame->data[6]<<16) | (frame->data[7]<<24);
    mb->TIR |= CAN_TI0R_TXRQ;
    return STM32_CAN_OK;
}

uint8_t STM32_CAN_Receive(STM32_CAN_Frame_t* out_frame, uint32_t timeout_ms) {
    uint32_t start = millis();
    do {
        if (queue_pop(out_frame)) return STM32_CAN_OK;
        if (timeout_ms == 0) break;
        delay(1);
    } while ((millis() - start) < timeout_ms);
    return STM32_CAN_ERR_TIMEOUT;
}

void     STM32_CAN_RegisterRxCallback(STM32_CAN_RxCb_t cb) { s_rx_callback = cb; }
uint8_t  STM32_CAN_RxAvailable(void) { return (s_rx_head >= s_rx_tail) ? (s_rx_head - s_rx_tail) : (STM32_CAN_QUEUE_SIZE - s_rx_tail + s_rx_head); }
uint32_t STM32_CAN_GetErrorStatus(void) { return CAN1->ESR; }
void     STM32_CAN_RecoverBusOff(void) { CAN1->MCR |= CAN_MCR_INRQ; uint32_t t=100000; while(--t); CAN1->MCR &= ~CAN_MCR_INRQ; }

// F405 usa CAN1_RX0_IRQn (no USB_LP_CAN1_RX0 como el F103)
extern "C" void CAN1_RX0_IRQHandler(void) {
    while (CAN1->RF0R & CAN_RF0R_FMP0) {
        STM32_CAN_Frame_t f = {};
        CAN_FIFOMailBox_TypeDef* mb = &CAN1->sFIFOMailBox[0];
        f.id  = (mb->RIR >> 21) & 0x7FF;
        f.rtr = (mb->RIR & CAN_RI0R_RTR) != 0;
        f.dlc = mb->RDTR & 0x0F;
        f.data[0]=(mb->RDLR>>0)&0xFF; f.data[1]=(mb->RDLR>>8)&0xFF;
        f.data[2]=(mb->RDLR>>16)&0xFF; f.data[3]=(mb->RDLR>>24)&0xFF;
        f.data[4]=(mb->RDHR>>0)&0xFF;  f.data[5]=(mb->RDHR>>8)&0xFF;
        f.data[6]=(mb->RDHR>>16)&0xFF; f.data[7]=(mb->RDHR>>24)&0xFF;
        CAN1->RF0R |= CAN_RF0R_RFOM0;
        if (s_rx_callback) s_rx_callback(&f);
        queue_push(&f);
    }
}
