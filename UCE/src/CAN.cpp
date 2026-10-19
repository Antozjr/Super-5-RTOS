#include "CAN.h"
#include "Config.h"
#include "Engine.h"
#include "../../shared/Protocol.h"
#include "../../shared/Messages.h"

/**
 * CAN.cpp - UCE: Implementación CAN
 * STM32F405 CAN1 en PB8/PB9
 * TODO: Implementar bxCAN igual que BCM/src/CAN.cpp
 */

static CAN_RxCallback_t g_rx_callback = nullptr;

bool CAN_Init(void) {
    // TODO: bxCAN STM32F405, misma configuración 500kbps que F103
    // Diferencia: clock APB1 a 42MHz (no 36MHz como F103)
    // BTR para 500kbps @ 42MHz: Prescaler=3, BS1=11, BS2=2
    Serial.println(F("[CAN] UCE Init 500kbps - TODO"));
    return true;
}

bool CAN_Send(CAN_Frame_t* frame) {
    if (!frame || frame->dlc > 8) return false;
    return true;
}

void CAN_RegisterRxCallback(CAN_RxCallback_t cb) { g_rx_callback = cb; }
void CAN_ProcessRx(void) { /* TODO: FIFO bxCAN */ }

void CAN_SendHeartbeat(void) {
    MSG_Heartbeat_t msg = {};
    msg.status   = STATUS_OK;
    msg.uptime_s = millis() / 1000;
    CAN_Frame_t f = { CAN_UCE_HEARTBEAT, sizeof(MSG_Heartbeat_t) };
    memcpy(f.data, &msg, f.dlc);
    CAN_Send(&f);
}

void CAN_SendEngineData(void) {
    const EngineData_t* e = Engine_GetData();
    MSG_UCE_Engine_t msg  = {};
    msg.rpm      = e->rpm;
    msg.map_mbar = e->map_mbar;
    msg.tps_pct  = e->tps_pct;
    msg.lambda_afr = e->lambda_afr;
    CAN_Frame_t f = { CAN_UCE_ENGINE, sizeof(MSG_UCE_Engine_t) };
    memcpy(f.data, &msg, f.dlc);
    CAN_Send(&f);
}

void CAN_SendTemps(void) {
    const EngineData_t* e = Engine_GetData();
    MSG_UCE_Temps_t msg   = {};
    msg.clt_cdeg = e->clt_cdeg;
    msg.iat_cdeg = e->iat_cdeg;
    CAN_Frame_t f = { CAN_UCE_TEMPS, sizeof(MSG_UCE_Temps_t) };
    memcpy(f.data, &msg, f.dlc);
    CAN_Send(&f);
}

void CAN_SendIgnition(void) {
    const EngineData_t* e = Engine_GetData();
    MSG_UCE_Ignition_t msg = {};
    msg.advance_cdeg = (int16_t)(e->advance_deg * 10.0f);
    msg.dwell_us     = (uint16_t)(e->dwell_us);
    msg.knock        = 0;  // TODO: implementar detección knock
    CAN_Frame_t f = { CAN_UCE_IGNITION, sizeof(MSG_UCE_Ignition_t) };
    memcpy(f.data, &msg, f.dlc);
    CAN_Send(&f);
}

void CAN_SendInjection(void) {
    const EngineData_t* e = Engine_GetData();
    MSG_UCE_Injection_t msg = {};
    msg.pw_us      = (uint16_t)(e->inj_pw_us);
    msg.ve_pct     = (uint8_t)(e->ve_pct);
    msg.corrections = 0;  // TODO: bitmask correcciones activas
    CAN_Frame_t f = { CAN_UCE_INJECTION, sizeof(MSG_UCE_Injection_t) };
    memcpy(f.data, &msg, f.dlc);
    CAN_Send(&f);
}
