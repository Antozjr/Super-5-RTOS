#include "Engine.h"
#include "Config.h"
#include "Tables.h"
#include "Timing.h"
#include <math.h>
#include <STM32FreeRTOS.h>

/**
 * Engine.cpp - UCE: Control motor EFI
 *
 * Speed-density para inyección:
 *   PW = (VE × MAP × Displacement) / (RPM × AFR × Density_ref) × corrections
 *
 * TODO: Implementar correcciones completas (CLT warmup, IAT, lambda closed-loop).
 * TODO: Calibrar constante de inyección con medidor de flujo.
 * TODO: Implementar corte de combustible en deceleración (DFCO).
 */

// =============================================================================
// ESTADO INTERNO
// =============================================================================

static EngineData_t g_engine = {};

// Constante de inyección base (µs por % VE por kPa de MAP)
// TODO: Calibrar con caudalímetro real
#define INJ_BASE_CONSTANT   12.5f   // µs / (%VE × kPa) → ajustar en banco

// =============================================================================
// LECTURA ADC SENSORES
// =============================================================================

static uint16_t readMAP_mbar(void) {
    uint16_t raw = analogRead(PIN_ADC_MAP);
    uint32_t vout_mv = (uint32_t)raw * ADC_VREF_MV / ADC_RESOLUTION;
    if (vout_mv < MAP_VOUT_MIN_MV) return MAP_KPA_MIN * 10;
    if (vout_mv > MAP_VOUT_MAX_MV) return MAP_KPA_MAX * 10;
    float ratio = (float)vout_mv / ADC_VREF_MV;
    float kpa   = (ratio + 0.04f) / 0.00369f;
    return (uint16_t)(kpa * 10.0f);
}

static uint8_t readTPS_pct(void) {
    uint16_t raw = analogRead(PIN_ADC_TPS);
    uint32_t mv  = (uint32_t)raw * ADC_VREF_MV / ADC_RESOLUTION;
    if (mv <= TPS_MIN_MV) return 0;
    if (mv >= TPS_MAX_MV) return 100;
    return (uint8_t)((mv - TPS_MIN_MV) * 100 / (TPS_MAX_MV - TPS_MIN_MV));
}

static int16_t readNTC_cdeg(uint8_t pin) {
    uint16_t raw = analogRead(pin);
    if (raw == 0) return -400;
    float r = (float)NTC_RPULL_OHM * raw / (ADC_RESOLUTION - raw);
    if (r <= 0.0f) return 1500;
    float inv_t = 1.0f / NTC_T0_K + (1.0f / NTC_BETA) * logf(r / NTC_R0_OHM);
    return (int16_t)((1.0f / inv_t - 273.15f) * 10.0f);
}

static uint8_t readLambda_afr10(void) {
    uint16_t raw = analogRead(PIN_ADC_LAMBDA);
    uint32_t mv  = (uint32_t)raw * ADC_VREF_MV / ADC_RESOLUTION;
    // Interpolación lineal entre rico (200mV→AFR 11.0) y pobre (800mV→AFR 16.0)
    if (mv <= LAMBDA_RICH_MV)  return 110;
    if (mv >= LAMBDA_LEAN_MV)  return 160;
    return (uint8_t)(110 + (mv - LAMBDA_RICH_MV) * 50 / (LAMBDA_LEAN_MV - LAMBDA_RICH_MV));
}

static uint16_t readBatt_mv(void) {
    uint16_t raw = analogRead(PIN_ADC_BATT);
    uint32_t vadc_mv = (uint32_t)raw * ADC_VREF_MV / ADC_RESOLUTION;
    return (uint16_t)(vadc_mv * BATT_DIVIDER_NUM / BATT_DIVIDER_DEN);
}

// =============================================================================
// CÁLCULO AVANCE IGNICIÓN
// =============================================================================

float Engine_CalcAdvance(uint16_t rpm, uint16_t map_mbar) {
    // Interpolación en tabla 3D
    float advance = tableLookup_i8(TABLE_IGN, rpm, map_mbar);

    // TODO: Corrección por temperatura (retraso en frío)
    // if (g_engine.clt_cdeg < 400) advance -= (400 - g_engine.clt_cdeg) * 0.01f;

    // TODO: Corrección por knock (retraso activo)

    // Límites de seguridad
    if (advance < ADVANCE_MIN_DEG) advance = ADVANCE_MIN_DEG;
    if (advance > ADVANCE_MAX_DEG) advance = ADVANCE_MAX_DEG;

    return advance;
}

// =============================================================================
// CÁLCULO DWELL
// =============================================================================

uint32_t Engine_CalcDwell(uint16_t batt_mv) {
    // Compensar tensión de batería: a menor tensión, más dwell
    // Dwell = base × (12500 / batt_mv)
    uint32_t dwell = (uint32_t)DWELL_DEFAULT_US * 12500 / batt_mv;
    if (dwell > 5000) dwell = 5000;   // Cap de seguridad
    if (dwell < 2000) dwell = 2000;
    return dwell;
}

// =============================================================================
// CÁLCULO PULSO INYECCIÓN (Speed-Density)
// =============================================================================

uint32_t Engine_CalcInjPW(uint16_t rpm, uint16_t map_mbar,
                           int16_t clt_cdeg, int16_t iat_cdeg) {
    if (rpm < 100) return 0;   // Motor parado

    // VE interpolada
    float ve = tableLookup_u8(TABLE_VE, rpm, map_mbar);

    // Pulso base = VE × MAP × constante / RPM
    float map_kpa = map_mbar / 10.0f;
    float pw = INJ_BASE_CONSTANT * ve * map_kpa / rpm;

    // TODO: Corrección por temperatura refrigerante (enriquecimiento en frío)
    float clt_c = clt_cdeg / 10.0f;
    if (clt_c < 70.0f) {
        float correction = 1.0f + (70.0f - clt_c) * 0.005f;
        pw *= correction;
    }

    // TODO: Corrección por temperatura aire (IAT)
    // Aire frío = más denso = menos combustible necesario
    float iat_c = iat_cdeg / 10.0f;
    if (iat_c > 20.0f) {
        pw *= (1.0f - (iat_c - 20.0f) * 0.002f);
    }

    // TODO: Corrección lambda closed-loop (±10%)
    // TODO: Corrección aceleración (TPS rate)

    // Añadir tiempo muerto inyector
    uint32_t pw_us = (uint32_t)(pw * 1000.0f) + INJ_DEAD_TIME_US;

    // Límites
    if (pw_us > INJ_MAX_PW_US) pw_us = INJ_MAX_PW_US;
    if (pw_us < INJ_DEAD_TIME_US) pw_us = 0;

    return pw_us;
}

// =============================================================================
// INIT
// =============================================================================

void Engine_Init(void) {
    // Configurar pines ADC
    // analogReadResolution(12);

    // Configurar pines salida inyectores y bobinas
    pinMode(PIN_INJ_1, OUTPUT); digitalWrite(PIN_INJ_1, LOW);
    pinMode(PIN_INJ_2, OUTPUT); digitalWrite(PIN_INJ_2, LOW);
    pinMode(PIN_INJ_3, OUTPUT); digitalWrite(PIN_INJ_3, LOW);
    pinMode(PIN_INJ_4, OUTPUT); digitalWrite(PIN_INJ_4, LOW);

    // TODO: Configurar pines ignición (Timer1 en modo comparación)

    // Iniciar bomba combustible
    pinMode(PIN_FUEL_PUMP, OUTPUT);
    digitalWrite(PIN_FUEL_PUMP, HIGH);  // TODO: Apagar tras 2s si no hay RPM

    memset(&g_engine, 0, sizeof(g_engine));
    g_engine.mode = ENG_MODE_CRANKING;

    Serial.println(F("[Engine] Init OK"));
}

// =============================================================================
// UPDATE (llamado cada TASK_ENGINE_CTRL_MS)
// =============================================================================

void Engine_Update(void) {
    // 1. Leer sensores
    g_engine.map_mbar = readMAP_mbar();
    g_engine.tps_pct  = readTPS_pct();
    g_engine.clt_cdeg = readNTC_cdeg(PIN_ADC_CLT);
    g_engine.iat_cdeg = readNTC_cdeg(PIN_ADC_IAT);
    g_engine.lambda_afr = readLambda_afr10();
    g_engine.batt_mv  = readBatt_mv();
    g_engine.rpm      = Timing_GetRPM();

    // 2. Determinar modo
    if (g_engine.rpm < 200) {
        g_engine.mode = ENG_MODE_CRANKING;
    } else if (g_engine.tps_pct < 5 && g_engine.rpm < 1200) {
        g_engine.mode = ENG_MODE_IDLE;
    } else if (g_engine.clt_cdeg < 600) {  // < 60°C
        g_engine.mode = ENG_MODE_WARMUP;
    } else {
        g_engine.mode = ENG_MODE_NORMAL;
    }

    // 3. Calcular outputs si sincronizado
    if (!Timing_IsSynced()) {
        g_engine.error_flags |= ENG_ERR_NO_SYNC;
        return;
    }
    g_engine.error_flags &= ~ENG_ERR_NO_SYNC;

    g_engine.advance_deg = Engine_CalcAdvance(g_engine.rpm, g_engine.map_mbar);
    g_engine.dwell_us    = Engine_CalcDwell(g_engine.batt_mv);
    g_engine.inj_pw_us   = Engine_CalcInjPW(g_engine.rpm, g_engine.map_mbar,
                                              g_engine.clt_cdeg, g_engine.iat_cdeg);

    // 4. Programar eventos de timing
    // TODO: Calcular cuándo disparar para cada cilindro según posición cigüeñal
    // Orden de encendido Renault 5 GTL: 1-3-4-2
    // Timing_ScheduleIgnition(0, g_engine.advance_deg, g_engine.dwell_us);
    // Timing_ScheduleInjection(0, g_engine.inj_pw_us, 270.0f);
}

// =============================================================================
// GETTER
// =============================================================================

const EngineData_t* Engine_GetData(void) {
    return &g_engine;
}
