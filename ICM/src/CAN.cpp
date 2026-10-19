#include "CAN.h"
#include "Config.h"
#include "Sensors.h"
#include "../../shared/Protocol.h"
#include "../../shared/Messages.h"
#include <STM32_CAN.h>

/**
 * CAN.cpp - ICM: Implementación CAN usando librería STM32_CAN (bxCAN)
 */

static CAN_RxCallback_t g_rx_callback = nullptr;

// =============================================================================
// INIT
// =============================================================================

bool CAN_Init(void) {
    uint8_t err = STM32_CAN_Init(500000);
    if (err != STM32_CAN_OK) {
        Serial.printf("[CAN] ERROR: STM32_CAN_Init falló (%d)\n", err);
        return false;
    }

    STM32_CAN_SetFilter(0x000, 0x000);  // Aceptar todo

    Serial.println(F("[CAN] ICM Init OK — 500 kbps, PB8/PB9"));
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

    return (STM32_CAN_Send(&f, 10) == STM32_CAN_OK);
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
// MENSAJES ESPECÍFICOS ICM
// =============================================================================

void CAN_SendHeartbeat(void) {
    MSG_Heartbeat_t msg = {};
    msg.status   = STATUS_OK;
    msg.uptime_s = millis() / 1000;

    CAN_Frame_t frame = {};
    frame.id  = CAN_ICM_HEARTBEAT;
    frame.dlc = sizeof(MSG_Heartbeat_t);
    memcpy(frame.data, &msg, frame.dlc);
    CAN_Send(&frame);
}

void CAN_SendEngineData(void) {
    const SensorData_t* s = Sensors_GetData();

    MSG_ICM_EngineData_t msg = {};
    msg.rpm      = s->rpm;
    msg.map_mbar = s->map_mbar;
    msg.clt_c    = (int8_t)(s->clt_cdeg / 10);
    msg.iat_c    = (int8_t)(s->iat_cdeg / 10);
    msg.batt_dv  = (uint8_t)(s->batt_mv / 100);

    CAN_Frame_t frame = {};
    frame.id  = CAN_ICM_ENGINE_DATA;
    frame.dlc = sizeof(MSG_ICM_EngineData_t);
    memcpy(frame.data, &msg, frame.dlc);
    CAN_Send(&frame);
}

void CAN_SendStatus(void) {
    const SensorData_t* s = Sensors_GetData();

    MSG_ICM_Status_t msg = {};
    msg.flags  = STATUS_OK;
    msg.errors = s->error_flags;

    CAN_Frame_t frame = {};
    frame.id  = CAN_ICM_STATUS;
    frame.dlc = sizeof(MSG_ICM_Status_t);
    memcpy(frame.data, &msg, frame.dlc);
    CAN_Send(&frame);
}
