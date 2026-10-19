#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <IWatchdog.h>
#include "Config.h"
#include "CAN.h"
#include "Sensors.h"

/**
 * main.cpp - ICM (Instrumentation Control Module)
 * STM32F103C8 @ 72MHz
 */

void Task_Heartbeat(void*  params);
void Task_Sensors(void*    params);
void Task_RPM(void*        params);
void Task_CAN_TX(void*     params);
void Task_CAN_RX(void*     params);
void Task_Watchdog(void*   params);

void setup(void) {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("================================="));
    Serial.println(F("  Super5 RTOS v3 - ICM Node"));
    Serial.print(F("  Firmware: "));
    Serial.println(FIRMWARE_VERSION);
    Serial.println(F("================================="));

    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, HIGH);

    if (!CAN_Init()) {
        Serial.println(F("[ICM] ERROR: CAN init fallido"));
        while (true) {
            digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
            delay(100);
        }
    }

    Sensors_Init();

    // --- IWDG — arrancar antes de crear tareas (PRO-009.1) ---
    IWatchdog.begin(IWDG_TIMEOUT_MS * 1000UL);  // argumento en microsegundos
    Serial.printf("[ICM] IWDG OK — timeout %d ms\n", IWDG_TIMEOUT_MS);

    xTaskCreate(Task_Heartbeat, "HB",  STACK_HEARTBEAT, NULL, PRIO_HEARTBEAT, NULL);
    xTaskCreate(Task_Sensors,   "SEN", STACK_SENSORS,   NULL, PRIO_SENSORS,   NULL);
    xTaskCreate(Task_RPM,       "RPM", STACK_RPM,       NULL, PRIO_RPM,       NULL);
    xTaskCreate(Task_CAN_TX,    "CTX", STACK_CAN_TX,    NULL, PRIO_CAN_TX,    NULL);
    xTaskCreate(Task_CAN_RX,    "CRX", STACK_CAN_RX,    NULL, PRIO_CAN_RX,    NULL);
    xTaskCreate(Task_Watchdog,  "WDG", STACK_WATCHDOG,  NULL, PRIO_WATCHDOG,  NULL);

    Serial.println(F("[ICM] Iniciando scheduler..."));
    vTaskStartScheduler();

    while (true);
}

void loop(void) {}
