#include "CAN.h"
#include "Config.h"
#include "../../shared/Protocol.h"
#include "../../shared/Messages.h"
#include <STM32FreeRTOS.h>
#include <STM32_CAN.h>

/**
 * CAN.cpp - BCM: Implementación CAN usando librería STM32_CAN (bxCAN)
 */

// =============================================================================
// VARIABLES INTERNAS
// =============================================================================

static CAN_RxCallback_t g_rx_callback = nullptr;

extern uint8_t g_light_flags;
extern uint8_t g_flame_pattern;
extern uint8_t g_flame_active;
extern uint8_t g_flame_frame;

// =============================================================================
// INIT
// =============================================================================

bool CAN_Init(void) {
    uint8_t err = STM32_CAN_Init(500000);
    if (err != STM32_CAN_OK) {
        Serial.printf("[CAN] ERROR: STM32_CAN_Init falló (%d)\n", err);
        return false;
    }

    // Filtro: aceptar comandos BCM (0x180-0x18F) y broadcast (0x7FF)
    // Máscara 0 = aceptar todo (simplificado para banco)
    STM32_CAN_SetFilter(0x000, 0x000);

    Serial.println(F("[CAN] BCM Init OK — 500 kbps, PB8/PB9"));
    return true;
}

// =============================================================================
// ENVÍO
// =============================================================================

bool CAN_Send(CAN_Frame_t* frame) {
    if (!frame || frame->dlc > 8) return false;

    STM32_CAN_Frame_t f = {};
    f.id  = frame->id;
    f.dlc = frame->dlc;
    memcpy(f.data, frame->data, frame->dlc);

    uint8_t err = STM32_CAN_Send(&f, 10);
    return (err == STM32_CAN_OK);
}

// =============================================================================
// RECEPCIÓN
// =============================================================================

static void stm32_rx_bridge(const STM32_CAN_Frame_t* f) {
    if (!g_rx_callback) return;
    CAN_Frame_t frame = {};
    frame.id  = f->id;
    frame.dlc = f->dlc;
    memcpy(frame.data, f->data, f->dlc);
    g_rx_callback(&frame);
}

void CAN_RegisterRxCallback(CAN_RxCallback_t cb) {
    g_rx_callback = cb;
    STM32_CAN_RegisterRxCallback(stm32_rx_bridge);
}

void CAN_ProcessRx(void) {
    STM32_CAN_Frame_t f;
    while (STM32_CAN_Receive(&f, 0) == STM32_CAN_OK) {
        if (g_rx_callback) {
            CAN_Frame_t frame = {};
            frame.id  = f.id;
            frame.dlc = f.dlc;
            memcpy(frame.data, f.data, f.dlc);
            g_rx_callback(&frame);
        }
    }
}

// =============================================================================
// MENSAJES ESPECÍFICOS BCM
// =============================================================================

void CAN_SendHeartbeat(void) {
    MSG_Heartbeat_t msg = {};
    msg.status   = STATUS_OK;
    msg.uptime_s = millis() / 1000;

    CAN_Frame_t frame = {};
    frame.id  = CAN_BCM_HEARTBEAT;
    frame.dlc = sizeof(MSG_Heartbeat_t);
    memcpy(frame.data, &msg, frame.dlc);
    CAN_Send(&frame);
}

void CAN_SendFlameState(void) {
    MSG_BCM_FlameState_t msg = {};
    msg.active  = g_flame_active;
    msg.pattern = g_flame_pattern;
    msg.frame   = g_flame_frame;

    CAN_Frame_t frame = {};
    frame.id  = CAN_BCM_FLAME_STATE;
    frame.dlc = sizeof(MSG_BCM_FlameState_t);
    memcpy(frame.data, &msg, frame.dlc);
    CAN_Send(&frame);
}

void CAN_SendLightsState(void) {
    MSG_BCM_LightsState_t msg = {};
    msg.light_flags = g_light_flags;

    CAN_Frame_t frame = {};
    frame.id  = CAN_BCM_LIGHTS_STATE;
    frame.dlc = sizeof(MSG_BCM_LightsState_t);
    memcpy(frame.data, &msg, frame.dlc);
    CAN_Send(&frame);
}
