#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "Config.h"
#include "CAN.h"
#include "Engine.h"
#include "Tuner.h"
#include "Timing.h"

/**
 * main.cpp - UCE (Engine Control Unit)
 * STM32F405RG @ 168MHz
 */

void Task_Heartbeat(void*   params);
void Task_EngineCtrl(void*  params);
void Task_CAN_TX(void*      params);
void Task_CAN_RX(void*      params);
void Task_Tuner(void*       params);
void Task_Diagnostics(void* params);

void setup(void) {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("================================="));
    Serial.println(F("  Super5 RTOS v3 - UCE Node"));
    Serial.print(F("  Firmware: "));
    Serial.println(FIRMWARE_VERSION);
    Serial.println(F("================================="));

    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, HIGH);

    // Inicializar timing (rueda fónica + timers)
    Timing_Init();

    // Inicializar control motor
    Engine_Init();

    // Inicializar CAN
    if (!CAN_Init()) {
        Serial.println(F("[UCE] ERROR: CAN init fallido"));
        while (true) {
            digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
            delay(100);
        }
    }

    // Inicializar TunerStudio
    Tuner_Init();

    // Crear tareas FreeRTOS
    xTaskCreate(Task_Heartbeat,   "HB",  STACK_HEARTBEAT,    NULL, PRIO_HEARTBEAT,    NULL);
    xTaskCreate(Task_EngineCtrl,  "ENG", STACK_ENGINE_CTRL,  NULL, PRIO_ENGINE_CTRL,  NULL);
    xTaskCreate(Task_CAN_TX,      "CTX", STACK_CAN_TX,       NULL, PRIO_CAN_TX,       NULL);
    xTaskCreate(Task_CAN_RX,      "CRX", STACK_CAN_RX,       NULL, PRIO_CAN_RX,       NULL);
    xTaskCreate(Task_Tuner,       "TUN", STACK_TUNER,        NULL, PRIO_TUNER,        NULL);
    xTaskCreate(Task_Diagnostics, "DIG", STACK_DIAG,         NULL, PRIO_DIAG,         NULL);

    Serial.println(F("[UCE] Iniciando scheduler..."));
    vTaskStartScheduler();

    while (true);
}

void loop(void) {}
