#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <IWatchdog.h>
#include "Config.h"
#include "CAN.h"
#include "Flames.h"

// Declaraciones de tareas (implementadas en Tasks.cpp)
void Task_Heartbeat(void* params);
void Task_Monitor(void*   params);
void Task_Horn(void*      params);
void Task_Flames(void*    params);
void Task_CAN_TX(void*    params);
void Task_CAN_RX(void*    params);
void Task_Watchdog(void*  params);
void Task_PowerMgr(void*  params);

// Cola bocina — definida en Tasks.cpp, creada aquí antes de lanzar tareas
extern QueueHandle_t g_horn_queue;

// =============================================================================
// SETUP
// =============================================================================

void setup(void) {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("================================="));
    Serial.println(F("  Super5 RTOS v3 - BCM Node"));
    Serial.print(F("  Firmware: "));
    Serial.println(FIRMWARE_VERSION);
    Serial.println(F("================================="));

    // --- GPIO salidas relé ---
    pinMode(PIN_LED_STATUS,  OUTPUT);
    pinMode(PIN_HORN_OUT,    OUTPUT);
    pinMode(PIN_LIGHT_POS,   OUTPUT);
    pinMode(PIN_LIGHT_DEL,   OUTPUT);
    pinMode(PIN_LIGHT_FOG_F, OUTPUT);
    pinMode(PIN_LIGHT_FOG_R, OUTPUT);

    // --- GPIO entradas mando (PC817, pulldown 10kΩ externo) ---
    pinMode(PIN_IGN_SENSE,   INPUT);
    pinMode(PIN_IN_HORN,     INPUT);
    pinMode(PIN_IN_LIGHT_POS,INPUT);
    pinMode(PIN_IN_LIGHT_DEL,INPUT);
    pinMode(PIN_IN_FOG_F,    INPUT);
    pinMode(PIN_IN_FOG_R,    INPUT);

    // Todo apagado al arrancar
    digitalWrite(PIN_LED_STATUS,  HIGH);  // activo en bajo
    digitalWrite(PIN_HORN_OUT,    LOW);
    digitalWrite(PIN_LIGHT_POS,   LOW);
    digitalWrite(PIN_LIGHT_DEL,   LOW);
    digitalWrite(PIN_LIGHT_FOG_F, LOW);
    digitalWrite(PIN_LIGHT_FOG_R, LOW);

    // --- CAN ---
    if (!CAN_Init()) {
        Serial.println(F("[BCM] ERROR: CAN init fallido"));
        while (true) {
            digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
            delay(100);
        }
    }
    Serial.println(F("[BCM] CAN OK"));

    // --- Flames (PWM Timer1 CH1 en PA8) ---
    Flames_Init();
    Serial.println(F("[BCM] Flames OK"));

    // --- Cola bocina ---
    g_horn_queue = xQueueCreate(HORN_QUEUE_LEN, sizeof(uint8_t));
    configASSERT(g_horn_queue != nullptr);

    // --- IWDG — arrancar antes de crear tareas (PRO-001) ---
    IWatchdog.begin(IWDG_TIMEOUT_MS * 1000UL);  // argumento en microsegundos
    Serial.printf("[BCM] IWDG OK — timeout %d ms\n", IWDG_TIMEOUT_MS);

    // --- Tareas FreeRTOS ---
    BaseType_t ret;

    ret = xTaskCreate(Task_Heartbeat, "HB",  STACK_HEARTBEAT, NULL, PRIO_HEARTBEAT, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_Monitor,   "MON", STACK_MONITOR,   NULL, PRIO_MONITOR,   NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_Horn,      "HRN", STACK_HORN,      NULL, PRIO_HORN,      NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_Flames,    "FLM", STACK_FLAMES,    NULL, PRIO_FLAMES,    NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_CAN_TX,    "CTX", STACK_CAN_TX,    NULL, PRIO_CAN_TX,    NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_CAN_RX,    "CRX", STACK_CAN_RX,    NULL, PRIO_CAN_RX,    NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_Watchdog,  "WDG", STACK_WATCHDOG,  NULL, PRIO_WATCHDOG,  NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(Task_PowerMgr, "PWR", STACK_POWER_MGR, NULL, PRIO_POWER_MGR, NULL);
    configASSERT(ret == pdPASS);

    Serial.println(F("[BCM] Tareas creadas — iniciando scheduler"));
    vTaskStartScheduler();

    Serial.println(F("[BCM] ERROR: scheduler no pudo iniciar"));
    while (true);
}

// =============================================================================
// LOOP — nunca se ejecuta con FreeRTOS
// =============================================================================

void loop(void) {}
