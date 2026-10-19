#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <stdint.h>

/**
 * CAN.h - Gateway: Interfaz CAN vía MCP2515 (SPI)
 */

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
    uint32_t timestamp_ms;   // Timestamp local de recepción
} CAN_Frame_t;

typedef void (*CAN_RxCallback_t)(const CAN_Frame_t* frame);

#define CAN_MAX_RX_CALLBACKS  4   // Callbacks simultáneos (Web, WebSocket, etc.)

bool CAN_Init(void);
bool CAN_Send(const CAN_Frame_t* frame);
bool CAN_Receive(CAN_Frame_t* frame);   // Non-blocking, return false si vacío

/**
 * Registra un callback invocado cada vez que llega un frame CAN.
 * Soporta hasta CAN_MAX_RX_CALLBACKS registros simultáneos.
 * Llamar una vez por módulo en su función de init.
 */
void CAN_RegisterRxCallback(CAN_RxCallback_t cb);

void CAN_Task_RX(void* params);         // Tarea Core 0
void CAN_Task_TX(void* params);         // Tarea Core 0

// Envío de comandos hacia los nodos
bool CAN_SendCommand(uint32_t id, const uint8_t* data, uint8_t dlc);

// Power management (PRO-010)
uint32_t CAN_GetLastRxMs(void);   // Timestamp del último frame recibido
void     CAN_DrainBuffer(void);   // Vaciar buffer MCP2515 antes de deep sleep

#endif // CAN_H
