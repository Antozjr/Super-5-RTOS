#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <IWatchdog.h>
#include "Config.h"
#include "CAN.h"
#include "Flames.h"
#include "../../shared/Protocol.h"

/**
 * Tasks.cpp - BCM: Tareas FreeRTOS
 *
 * Tareas:
 *   Task_Heartbeat  - Keepalive 1Hz
 *   Task_Monitor    - Fail-safe CAN (PRO-002) + watchdog flag
 *   Task_Horn       - Control bocina via cola FreeRTOS
 *   Task_Flames     - Motor animación flames (PWM real — PRO-005)
 *   Task_CAN_TX     - Transmisión periódica CAN
 *   Task_CAN_RX     - Recepción y despacho mensajes CAN
 *   Task_Watchdog   - Kick IWDG si todas las tareas críticas están vivas (PRO-001)
 */

// =============================================================================
// ESTADO GLOBAL DEL NODO
// =============================================================================

uint8_t  g_light_flags   = 0x00;  // bitmask LIGHT_*
uint8_t  g_flame_pattern = 0;
uint8_t  g_flame_active  = 0;
uint8_t  g_flame_frame   = 0;

// Fail-safe CAN (PRO-002)
volatile uint32_t g_last_can_rx_ms = 0;
volatile bool     g_failsafe_active = false;

// Flags de vitalidad de tareas para IWDG (PRO-001)
volatile uint8_t g_wdg_flags = 0;

// Estado ignición (PRO-011): true = contacto ON, false = contacto OFF
volatile bool g_ign_on = true;

// Cola bocina (creada en main.cpp antes de lanzar tareas)
QueueHandle_t g_horn_queue = nullptr;

// =============================================================================
// HELPERS INTERNOS
// =============================================================================

static void apply_lights(uint8_t flags) {
    digitalWrite(PIN_LIGHT_POS,   (flags & LIGHT_POSITION)  ? HIGH : LOW);
    digitalWrite(PIN_LIGHT_DEL,   (flags & LIGHT_LOW_BEAM)  ? HIGH : LOW);
    digitalWrite(PIN_LIGHT_FOG_F, (flags & LIGHT_FOG_FRONT) ? HIGH : LOW);
    digitalWrite(PIN_LIGHT_FOG_R, (flags & LIGHT_FOG_REAR)  ? HIGH : LOW);
}

// =============================================================================
// CALLBACK CAN RX
// =============================================================================

static void onCANReceived(CAN_Frame_t* frame) {
    if (!frame) return;

    g_last_can_rx_ms = millis();  // actualizar timestamp para fail-safe

    switch (frame->id) {

        case CAN_CMD_BCM_LIGHTS:
            if (!g_failsafe_active) {
                g_light_flags = frame->data[0];
                apply_lights(g_light_flags);
            }
            Serial.printf("[BCM] CMD lights: 0x%02X\n", frame->data[0]);
            break;

        case CAN_CMD_BCM_FLAME:
            if (!g_failsafe_active) {
                Flames_Start(frame->data[0], frame->data[1]);
            }
            break;

        case CAN_CMD_BCM_HORN:
            if (g_horn_queue) {
                uint8_t cmd = frame->data[0] ? 1u : 0u;
                xQueueSend(g_horn_queue, &cmd, 0);
            }
            break;

        case CAN_BROADCAST_RESET:
            if (frame->data[0] == 0xFF) {
                Serial.println(F("[BCM] BROADCAST RESET"));
                NVIC_SystemReset();
            }
            break;

        default:
            break;
    }
}

// =============================================================================
// TAREA: HEARTBEAT
// =============================================================================

void Task_Heartbeat(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
        if (g_ign_on) {
            CAN_SendHeartbeat();
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_HEARTBEAT_MS));
    }
}

// =============================================================================
// TAREA: MONITOR — fail-safe CAN (PRO-002) + watchdog flag
// =============================================================================

void Task_Monitor(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t failsafe_entry = 0;

    for (;;) {
        g_wdg_flags |= WDG_FLAG_MONITOR;

        uint32_t now = millis();
        bool can_alive = (now - g_last_can_rx_ms) < FAILSAFE_CAN_TIMEOUT_MS;

        if (!g_ign_on) {
            if (g_failsafe_active) {
                g_failsafe_active = false;
            }
        } else if (!can_alive && !g_failsafe_active) {
            g_failsafe_active = true;
            failsafe_entry = now;
            Flames_Stop();
            g_light_flags = LIGHT_POSITION;
            apply_lights(g_light_flags);
            Serial.println(F("[BCM] FAIL-SAFE: CAN perdido — luces posicion ON"));

        } else if (!can_alive && g_failsafe_active) {
            if ((now - failsafe_entry) > FAILSAFE_LIGHTS_TIMEOUT_MS) {
                g_light_flags = 0x00;
                apply_lights(0x00);
            }

        } else if (can_alive && g_failsafe_active) {
            g_failsafe_active = false;
            Serial.println(F("[BCM] FAIL-SAFE: CAN restaurado"));
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_MONITOR_MS));
    }
}

// =============================================================================
// TAREA: HORN
// =============================================================================

void Task_Horn(void* params) {
    (void)params;
    uint8_t cmd;

    for (;;) {
        if (xQueueReceive(g_horn_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            digitalWrite(PIN_HORN_OUT, cmd ? HIGH : LOW);
            Serial.printf("[BCM] Horn: %s\n", cmd ? "ON" : "OFF");
        }
    }
}

// =============================================================================
// TAREA: FLAMES
// =============================================================================

void Task_Flames(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        Flames_Update();
        g_flame_active  = Flames_IsActive() ? 1 : 0;
        g_flame_pattern = Flames_GetPattern();
        g_flame_frame   = Flames_GetFrame();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_FLAMES_MS));
    }
}

// =============================================================================
// TAREA: CAN TX
// =============================================================================

void Task_CAN_TX(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (g_ign_on) {
            CAN_SendFlameState();
            CAN_SendLightsState();
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CAN_TX_MS));
    }
}

// =============================================================================
// TAREA: CAN RX
// =============================================================================

void Task_CAN_RX(void* params) {
    (void)params;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Grace period: evita fail-safe inmediato antes de que el bus esté activo
    g_last_can_rx_ms = millis();

    CAN_RegisterRxCallback(onCANReceived);

    for (;;) {
        g_wdg_flags |= WDG_FLAG_CAN_RX;
        CAN_ProcessRx();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CAN_RX_MS));
    }
}

// =============================================================================
// TAREA: POWER MANAGER — PRO-011
// Monitoriza PA10 (optoacoplador PC817 detectando +15).
// HIGH = contacto ON → operación normal.
// LOW estable 1s → apagar salidas y suspender TX CAN para ahorrar batería.
// El IWDG sigue corriendo vía Task_Watchdog; no se usa STOP mode para garantizar
// compatibilidad con el IWDG de hardware (LSI no para en STOP en STM32F1).
// =============================================================================

void Task_PowerMgr(void* params) {
    (void)params;
    uint32_t ign_off_start = 0;

    for (;;) {
        bool ign = (digitalRead(PIN_IGN_SENSE) == HIGH);

        if (ign) {
            ign_off_start = 0;
            if (!g_ign_on) {
                g_ign_on = true;
                g_last_can_rx_ms = millis();  // evitar fail-safe inmediato al volver
                Serial.println(F("[BCM][PWR] Contacto ON"));
            }
        } else {
            if (ign_off_start == 0) ign_off_start = millis();
            if (g_ign_on && (millis() - ign_off_start) >= IGN_OFF_DEBOUNCE_MS) {
                g_ign_on = false;
                g_failsafe_active = false;
                apply_lights(0x00);
                Flames_Stop();
                ign_off_start = 0;
                Serial.println(F("[BCM][PWR] Contacto OFF — BCM en reposo"));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================================================================
// TAREA: WATCHDOG — PRO-001
// Kika el IWDG solo cuando todas las tareas críticas han marcado su flag.
// Si alguna tarea crítica muere, su flag no se marca → no hay kick → reset.
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
