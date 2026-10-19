#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <stdint.h>

/**
 * CAN.h - ICM: Interfaz CAN
 */

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
} CAN_Frame_t;

typedef void (*CAN_RxCallback_t)(CAN_Frame_t* frame);

bool CAN_Init(void);
bool CAN_Send(CAN_Frame_t* frame);
void CAN_RegisterRxCallback(CAN_RxCallback_t cb);
void CAN_ProcessRx(void);

// Mensajes específicos ICM
void CAN_SendHeartbeat(void);
void CAN_SendEngineData(void);
void CAN_SendStatus(void);

#endif // CAN_H
