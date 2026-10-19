#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "Config.h"
#include "CAN.h"
#include "Web.h"

/**
 * main.cpp - Gateway (WiFi ↔ CAN Bridge)
 * ESP32 @ 240MHz dual-core
 *
 * Core 0: Tareas CAN (tiempo real, menor latencia)
 * Core 1: Tareas Web/WebSocket (no críticas en tiempo)
 */

// Declaraciones de tareas
void CAN_Task_RX(void* params);
void CAN_Task_TX(void* params);
void Task_WebSocket(void* params);

void Task_Heartbeat(void* params) {
    for (;;) {
        digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
        Serial.printf("[GW] UP=%lus CLIENTS=%d\n",
                      millis()/1000, WiFi.softAPgetStationNum());
        vTaskDelay(pdMS_TO_TICKS(TASK_HEARTBEAT_MS));
    }
}

// =============================================================================
// POWER MANAGEMENT (PRO-010)
// =============================================================================

static void GW_EnterDeepSleep(void) {
    Serial.printf("[PWR] Sin tráfico CAN durante %ds — iniciando deep sleep\n",
                  POWER_SLEEP_TIMEOUT_MS / 1000);

    // 1. Desactivar ISR para evitar activación espuria durante la preparación
    detachInterrupt(digitalPinToInterrupt(PIN_MCP2515_INT));

    // 2. Vaciar el buffer del MCP2515 para que INT vuelva a HIGH (inactivo)
    //    antes de configurar el wake-up. Sin este paso, el ESP32 despertaría
    //    inmediatamente si INT ya estuviera en LOW por mensajes no leídos.
    CAN_DrainBuffer();

    // 3. Apagar radio WiFi (libera ~160mA)
    WiFi.softAPdisconnect(true);  // desconecta clientes y detiene beacons
    WiFi.mode(WIFI_OFF);
    delay(300);

    // 4. Configurar wake-up: flanco bajo en GPIO4 (MCP2515 INT activo bajo).
    //    GPIO4 = RTC_GPIO10 en ESP32 — compatible con ext0 wakeup.
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);

    Serial.println(F("[PWR] Wake-up configurado en GPIO4 (MCP2515 INT) — durmiendo"));
    Serial.flush();

    esp_deep_sleep_start();
    // No se llega aquí — el wake-up provoca un reset completo del ESP32
}

void Task_PowerMgr(void* params) {
    (void)params;
    // Verificar inactividad CAN cada 5 segundos para no consumir CPU
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if ((millis() - CAN_GetLastRxMs()) > POWER_SLEEP_TIMEOUT_MS) {
            GW_EnterDeepSleep();
        }
    }
}

// =============================================================================
// SETUP
// =============================================================================

void setup(void) {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("================================="));
    Serial.println(F("  Super5 RTOS v3 - Gateway"));
    Serial.print(F("  Firmware: "));
    Serial.println(FIRMWARE_VERSION);
    Serial.println(F("================================="));

    // Informar si el arranque viene de un wake-up por CAN (GPIO4)
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println(F("[PWR] Wake-up desde deep sleep por actividad CAN"));
    }

    pinMode(PIN_LED_STATUS, OUTPUT);

    // --- WiFi Access Point ---
    Serial.printf("[GW] Iniciando WiFi AP: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_AP);
    IPAddress local_ip, gateway, subnet;
    local_ip.fromString(WIFI_IP);
    gateway.fromString(WIFI_GATEWAY);
    subnet.fromString(WIFI_SUBNET);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[GW] AP IP: %s\n", WiFi.softAPIP().toString().c_str());

    // --- CAN ---
    bool can_ok = CAN_Init();
    if (!can_ok) {
        Serial.println(F("[GW] WARN: MCP2515 no disponible — modo WiFi-only"));
    }

    // --- Web Server ---
    Web_Init();

    // --- Tareas FreeRTOS ---
    // Core 0: CAN (solo si el hardware está presente)
    if (can_ok) {
        xTaskCreatePinnedToCore(CAN_Task_RX, "CRX", 4096, NULL, 5, NULL, 0);
        xTaskCreatePinnedToCore(CAN_Task_TX, "CTX", 4096, NULL, 4, NULL, 0);
    }

    // Core 1: Web/WS + Power management (no crítico)
    xTaskCreatePinnedToCore(Task_WebSocket, "WSS", 8192, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(Task_Heartbeat, "HB",  2048, NULL, 1, NULL, 1);
    if (can_ok) {
        xTaskCreatePinnedToCore(Task_PowerMgr, "PWR", 2048, NULL, 1, NULL, 1);
    }

    Serial.println(F("[GW] Sistema listo"));
}

// =============================================================================
// LOOP - ESP32 usa FreeRTOS internamente, loop() es la tarea idle del Core 1
// =============================================================================

void loop(void) {
    // AsyncWebServer no necesita llamada en loop
    delay(1000);
}
