#include <Arduino.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "CAN.h"
#include "../../shared/Protocol.h"
#include "../../shared/Messages.h"

/**
 * WebSocket.cpp - Gateway: Streaming de datos en tiempo real
 *
 * Puerto WS_PORT (81). Emite JSON a todos los clientes conectados
 * con los datos del motor cada WS_BROADCAST_MS milisegundos.
 *
 * Formato JSON emitido:
 * {
 *   "t": 12345,        // timestamp ms
 *   "rpm": 2500,
 *   "map": 850,        // mbar
 *   "clt": 85,         // °C
 *   "iat": 28,
 *   "batt": 13.2,
 *   "flame": 1,        // 0/1
 *   "pattern": 3
 * }
 *
 * TODO: Añadir autenticación WS (token en primer mensaje).
 * TODO: Implementar comandos bidireccionales (cliente → gateway → CAN).
 */

// =============================================================================
// SERVIDOR WEBSOCKET
// =============================================================================

static WebSocketsServer g_ws(WS_PORT);
static uint8_t g_client_count = 0;

// Cache datos
static MSG_ICM_EngineData_t g_eng = {};
static MSG_BCM_FlameState_t g_flm = {};

// =============================================================================
// CALLBACK EVENTOS WS
// =============================================================================

static void onWebSocketEvent(uint8_t client_id, WStype_t type,
                              uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            g_client_count++;
            Serial.printf("[WS] Cliente %d conectado - total: %d\n",
                          client_id, g_client_count);
            // Enviar datos actuales inmediatamente al nuevo cliente
            break;

        case WStype_DISCONNECTED:
            if (g_client_count > 0) g_client_count--;
            Serial.printf("[WS] Cliente %d desconectado\n", client_id);
            break;

        case WStype_TEXT:
            // TODO: Procesar comandos entrantes del cliente
            // Ej: {"cmd":"flame","pattern":5,"speed":200}
            Serial.printf("[WS] Msg cliente %d: %s\n", client_id, payload);
            break;

        default:
            break;
    }
}

// =============================================================================
// BROADCAST
// =============================================================================

static void WS_Broadcast(void) {
    if (g_client_count == 0) return;

    JsonDocument doc;
    doc["t"]       = millis();
    doc["rpm"]     = g_eng.rpm;
    doc["map"]     = g_eng.map_mbar;
    doc["clt"]     = g_eng.clt_c;
    doc["iat"]     = g_eng.iat_c;
    doc["batt"]    = g_eng.batt_dv / 10.0f;
    doc["flame"]   = (bool)g_flm.active;
    doc["pattern"] = g_flm.pattern;

    String json;
    serializeJson(doc, json);
    g_ws.broadcastTXT(json);
}

// =============================================================================
// CALLBACK CAN → actualiza cache local
// =============================================================================

static void onCANFrame(const CAN_Frame_t* frame) {
    switch (frame->id) {
        case CAN_ICM_ENGINE_DATA:
            memcpy(&g_eng, frame->data, sizeof(MSG_ICM_EngineData_t));
            break;
        case CAN_BCM_FLAME_STATE:
            memcpy(&g_flm, frame->data, sizeof(MSG_BCM_FlameState_t));
            break;
        default:
            break;
    }
}

// =============================================================================
// TAREA WEBSOCKET (Core 1)
// =============================================================================

void Task_WebSocket(void* params) {
    (void)params;

    // Registrar callback CAN para actualizar la caché local
    CAN_RegisterRxCallback(onCANFrame);

    g_ws.begin();
    g_ws.onEvent(onWebSocketEvent);
    Serial.printf("[WS] Server en puerto %d\n", WS_PORT);

    TickType_t t = xTaskGetTickCount();
    uint32_t last_broadcast = 0;

    for (;;) {
        g_ws.loop();   // Procesar eventos WS

        uint32_t now = millis();
        if (now - last_broadcast >= WS_BROADCAST_MS) {
            last_broadcast = now;
            WS_Broadcast();
        }

        vTaskDelay(pdMS_TO_TICKS(5));  // Yield frecuente para respuesta rápida
    }
}
