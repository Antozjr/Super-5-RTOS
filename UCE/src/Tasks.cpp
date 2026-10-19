#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "Config.h"
#include "CAN.h"
#include "Engine.h"
#include "Tuner.h"
#include "Timing.h"

/**
 * Tasks.cpp - UCE: Tareas FreeRTOS
 *
 * Task_Sensors      → Lectura ADC (100 Hz)
 * Task_EngineCtrl   → Control ignición/inyección (200 Hz)
 * Task_CAN_TX       → Transmisión telemetría (50 Hz)
 * Task_CAN_RX       → Recepción comandos (100 Hz)
 * Task_Tuner        → Protocolo TunerStudio (200 Hz)
 * Task_Diagnostics  → Errores y logging (2 Hz)
 * Task_Heartbeat    → Keepalive (1 Hz)
 *
 * ISRs (fuera de FreeRTOS, bare-metal):
 *   ISR_CrankTooth  → Rueda fónica (máxima prioridad NVIC)
 *   ISR_CamPulse    → Sensor fase
 */

// =============================================================================
// CALLBACK CAN RX
// =============================================================================

static void onCANReceived(CAN_Frame_t* frame) {
    if (!frame) return;
    switch (frame->id) {
        case CAN_CMD_UCE_TUNE:
            // TODO: Actualizar tabla de calibración
            // frame->data[0]=tabla, [1]=idx, [2-3]=valor
            Serial.printf("[UCE] CMD TUNE t=%d i=%d\n", frame->data[0], frame->data[1]);
            break;
        case CAN_CMD_UCE_MODE:
            Serial.printf("[UCE] CMD MODE %d\n", frame->data[0]);
            break;
        case 0x7FF:
            if (frame->data[0] == 0xFF) NVIC_SystemReset();
            break;
        default: break;
    }
}

// =============================================================================
// TAREAS
// =============================================================================

void Task_Heartbeat(void* params) {
    (void)params;
    TickType_t t = xTaskGetTickCount();
    for (;;) {
        digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
        CAN_SendHeartbeat();
        vTaskDelayUntil(&t, pdMS_TO_TICKS(TASK_HEARTBEAT_MS));
    }
}

void Task_EngineCtrl(void* params) {
    (void)params;
    TickType_t t = xTaskGetTickCount();
    for (;;) {
        Engine_Update();
        vTaskDelayUntil(&t, pdMS_TO_TICKS(TASK_ENGINE_CTRL_MS));
    }
}

void Task_CAN_TX(void* params) {
    (void)params;
    TickType_t t = xTaskGetTickCount();
    uint8_t cycle = 0;
    for (;;) {
        CAN_SendEngineData();
        if (cycle % 5 == 0) {
            CAN_SendTemps();
            CAN_SendIgnition();
            CAN_SendInjection();
        }
        cycle++;
        vTaskDelayUntil(&t, pdMS_TO_TICKS(TASK_CAN_TX_MS));
    }
}

void Task_CAN_RX(void* params) {
    (void)params;
    TickType_t t = xTaskGetTickCount();
    CAN_RegisterRxCallback(onCANReceived);
    for (;;) {
        CAN_ProcessRx();
        vTaskDelayUntil(&t, pdMS_TO_TICKS(TASK_CAN_RX_MS));
    }
}

void Task_Tuner(void* params) {
    (void)params;
    TickType_t t = xTaskGetTickCount();
    for (;;) {
        Tuner_Update();
        vTaskDelayUntil(&t, pdMS_TO_TICKS(TASK_TUNER_MS));
    }
}

void Task_Diagnostics(void* params) {
    (void)params;
    TickType_t t = xTaskGetTickCount();
    for (;;) {
        const EngineData_t* e = Engine_GetData();
        if (e->error_flags) {
            Serial.printf("[UCE] Errores: 0x%02X RPM=%d MAP=%d\n",
                          e->error_flags, e->rpm, e->map_mbar);
        }
        vTaskDelayUntil(&t, pdMS_TO_TICKS(TASK_DIAG_MS));
    }
}
