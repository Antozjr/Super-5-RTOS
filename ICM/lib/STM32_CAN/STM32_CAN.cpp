/**
 * STM32_CAN.cpp - Implementación bxCAN para STM32F103 (nodo ICM)
 * Idéntico al BCM. Ver BCM/lib/STM32_CAN/STM32_CAN.cpp para comentarios completos.
 */

#include "STM32_CAN.h"
#include <string.h>
#include <stm32f1xx.h>

static STM32_CAN_RxCb_t  s_rx_callback  = NULL;
static STM32_CAN_Frame_t s_rx_queue[STM32_CAN_QUEUE_SIZE];
static volatile uint8_t  s_rx_head      = 0;
static volatile uint8_t  s_rx_tail      = 0;

#define CAN_APB1_HZ     36000000UL
#define CAN_BS1_TICKS   13U
#define CAN_BS2_TICKS   4U
#define CAN_BS1         (((CAN_BS1_TICKS - 1U)) << 16)
#define CAN_BS2         (((CAN_BS2_TICKS - 1U)) << 20)

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
    uint32_t prescaler = CAN_APB1_HZ / (baud_rate * (1U + CAN_BS1_TICKS + CAN_BS2_TICKS));
    if (prescaler == 0) prescaler = 1;
    RCC->APB1ENR  |= RCC_APB1ENR_CAN1EN;
    RCC->APB2ENR  |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    AFIO->MAPR &= ~AFIO_MAPR_CAN_REMAP;
    AFIO->MAPR |=  AFIO_MAPR_CAN_REMAP_REMAP2;
    GPIOB->CRH &= ~(0xF << 0); GPIOB->CRH |= (0x4 << 0);
    GPIOB->CRH &= ~(0xF << 4); GPIOB->CRH |= (0xB << 4);

    // Salir de modo sleep (estado por defecto tras reset)
    CAN1->MCR &= ~CAN_MCR_SLEEP;
    uint32_t timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_SLAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    CAN1->MCR |= CAN_MCR_INRQ;
    timeout = 1000000;
    while (!(CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;
    // Timing + loopback para test
    CAN1->BTR = (prescaler - 1U) | CAN_BS1 | CAN_BS2 | CAN_BTR_LBKM;
    CAN1->MCR &= ~(CAN_MCR_TTCM | CAN_MCR_AWUM | CAN_MCR_NART |
                   CAN_MCR_RFLM | CAN_MCR_TXFP);
    CAN1->MCR |= CAN_MCR_ABOM;
    CAN1->FMR |= CAN_FMR_FINIT;
    CAN1->FA1R &= ~(1 << 0); CAN1->FS1R |= (1 << 0); CAN1->FM1R &= ~(1 << 0);
    CAN1->sFilterRegister[0].FR1 = 0; CAN1->sFilterRegister[0].FR2 = 0;
    CAN1->FA1R |= (1 << 0); CAN1->FMR &= ~CAN_FMR_FINIT;
    CAN1->MCR &= ~CAN_MCR_INRQ;
    timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;

    // Test loopback
    CAN1->sTxMailBox[0].TIR  = (0x7FEU << 21);
    CAN1->sTxMailBox[0].TDTR = 1;
    CAN1->sTxMailBox[0].TDLR = 0xAB;
    CAN1->sTxMailBox[0].TDHR = 0;
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
    uint32_t t = 100000;
    while (!(CAN1->RF0R & CAN_RF0R_FMP0) && --t);
    bool ok = (t > 0) &&
              ((CAN1->sFIFOMailBox[0].RIR >> 21) == 0x7FE) &&
              ((CAN1->sFIFOMailBox[0].RDLR & 0xFF) == 0xAB);
    CAN1->RF0R |= CAN_RF0R_RFOM0;
    if (!ok) return STM32_CAN_ERR_TIMEOUT;

    // Quitar LBKM → modo normal
    CAN1->MCR |= CAN_MCR_INRQ;
    timeout = 1000000;
    while (!(CAN1->MSR & CAN_MSR_INAK) && --timeout);
    if (!timeout) return STM32_CAN_ERR_TIMEOUT;
    CAN1->BTR = (prescaler - 1U) | CAN_BS1 | CAN_BS2;
    CAN1->IER |= CAN_IER_FMPIE0;
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5);
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    CAN1->MCR &= ~CAN_MCR_INRQ;
    timeout = 1000000;
    while ((CAN1->MSR & CAN_MSR_INAK) && --timeout);
    return timeout ? STM32_CAN_OK : STM32_CAN_ERR_TIMEOUT;
}

void STM32_CAN_SetFilter(uint16_t filter_id, uint16_t mask) {
    CAN1->FMR |= CAN_FMR_FINIT; CAN1->FA1R &= ~(1 << 0);
    CAN1->sFilterRegister[0].FR1 = ((uint32_t)filter_id << 21);
    CAN1->sFilterRegister[0].FR2 = ((uint32_t)mask      << 21);
    CAN1->FA1R |= (1 << 0); CAN1->FMR &= ~CAN_FMR_FINIT;
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

extern "C" void USB_LP_CAN1_RX0_IRQHandler(void) {
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
