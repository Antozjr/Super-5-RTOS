#ifndef STM32_CAN_H
#define STM32_CAN_H

/**
 * STM32_CAN.h - Wrapper del periférico bxCAN del STM32F103/F405
 *
 * Proporciona una API sencilla sobre el bxCAN hardware:
 *   - Inicialización a velocidad configurable (500 kbps por defecto)
 *   - Envío bloqueante con timeout
 *   - Recepción por interrupción con cola FIFO
 *   - Filtrado por ID/máscara
 *
 * Uso:
 *   STM32_CAN_Init(500000);
 *   STM32_CAN_SetFilter(0x000, 0x000);
 *   STM32_CAN_Send(&frame);
 *   STM32_CAN_Receive(&frame, 10);
 *
 * Compatible con STM32duino (Arduino framework) sobre STM32F405RG.
 * Requiere PlatformIO board: genericSTM32F405RG
 */

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
    bool     rtr;
} STM32_CAN_Frame_t;

typedef void (*STM32_CAN_RxCb_t)(const STM32_CAN_Frame_t* frame);

#define STM32_CAN_OK            0
#define STM32_CAN_ERR_TIMEOUT   1
#define STM32_CAN_ERR_BUSOFF    2
#define STM32_CAN_ERR_PARAM     3
#define STM32_CAN_QUEUE_SIZE    16

uint8_t  STM32_CAN_Init(uint32_t baud_rate);
void     STM32_CAN_SetFilter(uint16_t filter_id, uint16_t mask);
uint8_t  STM32_CAN_Send(const STM32_CAN_Frame_t* frame, uint32_t timeout_ms);
uint8_t  STM32_CAN_Receive(STM32_CAN_Frame_t* out_frame, uint32_t timeout_ms);
void     STM32_CAN_RegisterRxCallback(STM32_CAN_RxCb_t cb);
uint8_t  STM32_CAN_RxAvailable(void);
uint32_t STM32_CAN_GetErrorStatus(void);
void     STM32_CAN_RecoverBusOff(void);

#endif // STM32_CAN_H
