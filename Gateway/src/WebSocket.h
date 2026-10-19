#ifndef WEBSOCKET_H
#define WEBSOCKET_H

/**
 * WebSocket.h - Gateway: Streaming de datos en tiempo real
 *
 * Expone la tarea FreeRTOS que gestiona el servidor WebSocket (puerto WS_PORT).
 * Emite JSON a todos los clientes conectados cada WS_BROADCAST_MS ms.
 *
 * Formato JSON emitido:
 * {
 *   "t":       12345,   // timestamp millis()
 *   "rpm":     2500,    // RPM motor
 *   "map":     850,     // presión múltiple (mbar)
 *   "clt":     85,      // temperatura refrigerante (°C)
 *   "iat":     28,      // temperatura aire admisión (°C)
 *   "batt":    13.2,    // tensión batería (V)
 *   "flame":   1,       // 0=inactivo, 1=flames activos
 *   "pattern": 3        // patrón en ejecución
 * }
 *
 * La tarea debe asignarse a Core 1 (servidor web) con:
 *   xTaskCreatePinnedToCore(Task_WebSocket, "WS", STACK_WS, NULL, PRIO_WS, NULL, 1);
 */

/**
 * Tarea FreeRTOS del servidor WebSocket.
 * Inicializa WebSocketsServer en WS_PORT, procesa eventos de clientes
 * y hace broadcast de los datos del motor cada WS_BROADCAST_MS ms.
 *
 * Los datos se actualizan automáticamente escuchando el bus CAN
 * (IDs: CAN_ICM_ENGINE_DATA, CAN_BCM_FLAME_STATE).
 *
 * @param params No utilizado (pasar NULL).
 */
void Task_WebSocket(void* params);

#endif // WEBSOCKET_H
