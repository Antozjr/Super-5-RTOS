#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <IWatchdog.h>
#include "Config.h"
#include "CAN.h"
#include "Sensors.h"
#include "../../shared/Protocol.h"

/**
 * Tasks.cpp - ICM: Tareas FreeRTOS
 *
 * Task_Heartbeat   → Keepalive 1Hz
 * Task_Sensors     → Lectura ADC + media móvil (20 Hz) — PRO-003
 * Task_RPM         → Cálculo RPM desde ISR (100 Hz)
 * Task_CAN_TX      → Transmisión datos + status (20 Hz / 2 Hz)
 * Task_CAN_RX      → Recepción comandos (100 Hz)
 * Task_Watchdog    → Kick IWDG si tareas críticas están vivas — PRO-009.1
 */

// =============================================================================
// ESTADO GLOBAL
// =============================================================================

volatile uint8_t g_wdg_flags = 0;

// =============================================================================
// CALLBACK CAN RX
// =============================================================================

static void onCANReceived(CAN_Frame_t* frame) {
    if (!frame) return;
    switch (frame->id) {
        case CAN_BROADCAST_RESET:
            if (frame->data[0] == 0xFF) NVIC_SystemReset();
            break;
        default:
            break;
    }
}

// =============================================================================
// TAREAS
// =============================================================================

void Task_Heartbeat(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
        CAN_SendHeartbeat();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_HEARTBEAT_MS));
    }
}

void Task_Sensors(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        g_wdg_flags |= WDG_FLAG_SENSORS;

        Sensors_ReadAll();

        // Auto-flames en overrun de gas (PRO-009.4)
        // Condición: RPM alto + MAP bajo = corte de combustible en deceleración
        uint16_t rpm = Sensors_GetRPM();
        uint16_t map = Sensors_GetMAP();
        if (rpm > 3500 && map < 300) {
            CAN_Frame_t frame = {};
            frame.id      = CAN_CMD_BCM_FLAME;
            frame.dlc     = 2;
            frame.data[0] = 12;  // Patrón 12 (Rapid fire): último patrón válido (0–12)
            frame.data[1] = 200; // Velocidad máxima
            CAN_Send(&frame);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_SENSORS_MS));
    }
}

void Task_RPM(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        Sensors_RPM_Update();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_RPM_MS));
    }
}

void Task_CAN_TX(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Status cada 500ms: 500 / TASK_CAN_TX_MS = 10 ciclos
    static const uint8_t STATUS_EVERY = TASK_DIAG_MS / TASK_CAN_TX_MS;
    uint8_t status_tick = 0;

    for (;;) {
        CAN_SendEngineData();

        // PRO-009.5: enviar CAN_ICM_STATUS periódicamente sin tarea extra
        if (++status_tick >= STATUS_EVERY) {
            status_tick = 0;
            CAN_SendStatus();
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CAN_TX_MS));
    }
}

void Task_CAN_RX(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    CAN_RegisterRxCallback(onCANReceived);
    for (;;) {
        g_wdg_flags |= WDG_FLAG_CAN_RX;
        CAN_ProcessRx();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CAN_RX_MS));
    }
}

// =============================================================================
// TAREA: WATCHDOG — PRO-009.1
// Mismo patrón que en BCM: kick solo si Task_Sensors y Task_CAN_RX están vivas.
// =============================================================================

void Task_Watchdog(void* params) {
    (void)params;
    for (;;) {
        if ((g_wdg_flags & WDG_FLAGS_ALL) == WDG_FLAGS_ALL) {
            g_wdg_flags = 0;
            IWatchdog.reload();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
