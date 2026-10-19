#ifndef STM32_CAN_H
#define STM32_CAN_H

/**
 * STM32_CAN.h - Wrapper del periférico bxCAN del STM32F103
 *
 * Proporciona una API sencilla sobre el bxCAN hardware:
 *   - Inicialización a velocidad configurable (500 kbps por defecto)
 *   - Envío bloqueante con timeout
 *   - Recepción por interrupción con cola FIFO
 *   - Filtrado por ID/máscara
 *
 * Uso:
 *   STM32_CAN_Init(500000);
 *   STM32_CAN_SetFilter(0x180, 0x780);
 *   STM32_CAN_Send(&frame);
 *   STM32_CAN_Receive(&frame, 10);  // timeout 10 ms
 *
 * Compatible con STM32duino (Arduino framework) sobre STM32F103C8.
 * Requiere PlatformIO board: bluepill_f103c8
 */

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// TIPOS
// =============================================================================

typedef struct {
    uint32_t id;        // ID CAN estándar (11 bits, 0x000-0x7FF)
    uint8_t  dlc;       // Data Length Code (0-8)
    uint8_t  data[8];   // Payload (solo dlc bytes son válidos)
    bool     rtr;       // Remote Transmission Request
} STM32_CAN_Frame_t;

// Callback invocado desde la ISR de recepción FIFO0
typedef void (*STM32_CAN_RxCb_t)(const STM32_CAN_Frame_t* frame);

// =============================================================================
// CONSTANTES
// =============================================================================

#define STM32_CAN_OK            0
#define STM32_CAN_ERR_TIMEOUT   1
#define STM32_CAN_ERR_BUSOFF    2
#define STM32_CAN_ERR_PARAM     3
#define STM32_CAN_QUEUE_SIZE    16   // Frames en cola RX software

// =============================================================================
// API PÚBLICA
// =============================================================================

/**
 * Inicializa el periférico bxCAN.
 * Configura PB8 (RX) y PB9 (TX) remapeados, habilita FIFO0 e interrupciones.
 * @param baud_rate  Velocidad en bps (ej: 500000)
 * @return STM32_CAN_OK o código de error
 */
uint8_t STM32_CAN_Init(uint32_t baud_rate);

/**
 * Configura el filtro de aceptación de IDs.
 * Solo se reciben frames cuyo (ID & mask) == (filter_id & mask).
 * Para recibir todos los IDs: STM32_CAN_SetFilter(0x000, 0x000).
 * @param filter_id  ID base del filtro
 * @param mask       Máscara de bits significativos
 */
void STM32_CAN_SetFilter(uint16_t filter_id, uint16_t mask);

/**
 * Envía un frame CAN. Bloquea hasta que el mailbox queda libre o timeout.
 * @param frame      Frame a enviar (id, dlc, data)
 * @param timeout_ms Timeout en ms (0 = no esperar, retorna error si no hay mailbox)
 * @return STM32_CAN_OK o código de error
 */
uint8_t STM32_CAN_Send(const STM32_CAN_Frame_t* frame, uint32_t timeout_ms);

/**
 * Lee el siguiente frame de la cola RX software.
 * La cola se llena desde la ISR de recepción.
 * @param out_frame  Destino donde copiar el frame recibido
 * @param timeout_ms Timeout en ms (0 = no bloquear)
 * @return STM32_CAN_OK si hay datos, STM32_CAN_ERR_TIMEOUT si cola vacía
 */
uint8_t STM32_CAN_Receive(STM32_CAN_Frame_t* out_frame, uint32_t timeout_ms);

/**
 * Registra un callback invocado desde la ISR al recibir un frame.
 * PRECAUCIÓN: El callback corre en contexto de interrupción.
 *             Debe ser breve y no llamar a APIs bloqueantes.
 * @param cb Función callback (NULL para desregistrar)
 */
void STM32_CAN_RegisterRxCallback(STM32_CAN_RxCb_t cb);

/**
 * Devuelve el número de frames en la cola RX pendientes de leer.
 */
uint8_t STM32_CAN_RxAvailable(void);

/**
 * Lee y limpia el registro de errores del bxCAN.
 * Útil para detectar bus-off o errores de pasivo.
 * @return Valor del registro ESR del CAN (0 = sin errores)
 */
uint32_t STM32_CAN_GetErrorStatus(void);

/**
 * Fuerza la recuperación de un estado bus-off.
 * Llamar si STM32_CAN_GetErrorStatus() indica BOFF.
 */
void STM32_CAN_RecoverBusOff(void);

#endif // STM32_CAN_H
