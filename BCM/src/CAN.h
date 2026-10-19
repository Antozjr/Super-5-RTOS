#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <stdint.h>

/**
 * CAN.h - BCM: Interfaz del bus CAN
 * STM32F103C8 - CAN periférico hardware (bxCAN)
 */

// =============================================================================
// TIPOS
// =============================================================================

typedef struct {
    uint32_t id;        // ID CAN (11 bits estándar)
    uint8_t  dlc;       // Data Length Code (0-8)
    uint8_t  data[8];   // Payload
} CAN_Frame_t;

// Callback para mensajes recibidos
typedef void (*CAN_RxCallback_t)(CAN_Frame_t* frame);

// =============================================================================
// API PÚBLICA
// =============================================================================

/**
 * Inicializa el periférico CAN a 500 kbps.
 * Configura filtros básicos y habilita interrupciones.
 * @return true si OK
 */
bool CAN_Init(void);

/**
 * Envía un frame CAN.
 * @param frame Puntero al frame a enviar
 * @return true si encolado correctamente
 */
bool CAN_Send(CAN_Frame_t* frame);

/**
 * Registra callback para mensajes recibidos.
 * Se llama desde la ISR de recepción.
 */
void CAN_RegisterRxCallback(CAN_RxCallback_t cb);

/**
 * Procesa mensajes pendientes en la cola de recepción.
 * Llamar periódicamente desde Task_CAN_RX.
 */
void CAN_ProcessRx(void);

// Funciones de envío de mensajes específicos BCM
void CAN_SendHeartbeat(void);
void CAN_SendFlameState(void);
void CAN_SendLightsState(void);

#endif // CAN_H
