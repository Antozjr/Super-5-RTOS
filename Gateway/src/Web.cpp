#include "Web.h"
#include "Config.h"
#include "CAN.h"
#include "../../shared/Protocol.h"
#include "../../shared/Messages.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

/**
 * Web.cpp - Gateway: Servidor HTTP REST
 *
 * TODO: Añadir autenticación básica para comandos POST.
 * TODO: Implementar WebSocket en WebSocket.cpp.
 * TODO: Cargar assets desde SPIFFS (pio run -t uploadfs).
 */

// =============================================================================
// SERVIDOR
// =============================================================================

static AsyncWebServer g_server(HTTP_PORT);

// Cache de datos (actualizado desde callback CAN)
static MSG_ICM_EngineData_t g_engine_data = {};
static MSG_BCM_FlameState_t g_flame_state = {};
static uint32_t g_last_icm_ms = 0;
static uint32_t g_last_bcm_ms = 0;

// =============================================================================
// CALLBACK CAN → actualiza cache
// =============================================================================

void Web_OnCANFrame(const CAN_Frame_t* frame) {
    switch (frame->id) {
        case CAN_ICM_ENGINE_DATA:
            memcpy(&g_engine_data, frame->data, sizeof(MSG_ICM_EngineData_t));
            g_last_icm_ms = millis();
            break;
        case CAN_BCM_FLAME_STATE:
            memcpy(&g_flame_state, frame->data, sizeof(MSG_BCM_FlameState_t));
            g_last_bcm_ms = millis();
            break;
        default:
            break;
    }
}

// =============================================================================
// HANDLERS HTTP
// =============================================================================

static void handleStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["firmware"] = FIRMWARE_VERSION;
    doc["uptime_s"] = millis() / 1000;
    doc["icm_age_ms"] = millis() - g_last_icm_ms;
    doc["bcm_age_ms"] = millis() - g_last_bcm_ms;
    doc["wifi_clients"] = WiFi.softAPgetStationNum();

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

static void handleEngine(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["rpm"]      = g_engine_data.rpm;
    doc["map_mbar"] = g_engine_data.map_mbar;
    doc["clt_c"]    = g_engine_data.clt_c;
    doc["iat_c"]    = g_engine_data.iat_c;
    doc["batt_v"]   = g_engine_data.batt_dv / 10.0f;
    doc["age_ms"]   = millis() - g_last_icm_ms;

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

static void handleBCM(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["flame_active"]  = (bool)g_flame_state.active;
    doc["flame_pattern"] = g_flame_state.pattern;
    doc["flame_frame"]   = g_flame_state.frame;
    doc["age_ms"]        = millis() - g_last_bcm_ms;

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

static void handleCommand(AsyncWebServerRequest* req, uint8_t* bodyData,
                           size_t len, size_t index, size_t total) {
    // POST /api/v1/command
    // Body JSON: { "target": "bcm", "cmd": "flame", "pattern": 3, "speed": 200 }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, bodyData, len);
    if (err) {
        req->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
        return;
    }

    const char* target = doc["target"] | "";
    const char* cmd    = doc["cmd"]    | "";

    if (strcmp(target, "bcm") == 0 && strcmp(cmd, "flame") == 0) {
        uint8_t data[2] = {
            (uint8_t)(doc["pattern"] | 0),
            (uint8_t)(doc["speed"]   | 128)
        };
        CAN_SendCommand(CAN_CMD_BCM_FLAME, data, 2);
        req->send(200, "application/json", "{\"ok\":true}");

    } else {
        req->send(400, "application/json", "{\"error\":\"unknown command\"}");
    }
}

// =============================================================================
// INIT
// =============================================================================

void Web_Init(void) {
    // Montar SPIFFS (archivos estáticos)
    if (!SPIFFS.begin(true)) {
        Serial.println(F("[Web] SPIFFS no disponible - usando respuesta inline"));
    }

    // Registrar callback CAN
    CAN_RegisterRxCallback(Web_OnCANFrame);

    // Rutas estáticas
    g_server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // API REST
    g_server.on(API_STATUS,  HTTP_GET,  handleStatus);
    g_server.on(API_ENGINE,  HTTP_GET,  handleEngine);
    g_server.on(API_BCM,     HTTP_GET,  handleBCM);
    g_server.on(API_COMMAND, HTTP_POST,
        [](AsyncWebServerRequest* req){},  // onRequest vacío
        nullptr,                            // onUpload vacío
        handleCommand);                     // onBody

    // 404
    g_server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });

    g_server.begin();
    Serial.printf("[Web] HTTP server en %s:%d\n", WIFI_IP, HTTP_PORT);
}

void Web_Update(void) {
    // AsyncWebServer no necesita update() - usa callbacks internos
}
