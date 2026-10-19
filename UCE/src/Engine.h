#ifndef ENGINE_H
#define ENGINE_H

#include <Arduino.h>
#include <stdint.h>

/**
 * Engine.h - UCE: Interfaz control motor EFI
 *
 * Módulo principal de cálculo de ignición e inyección.
 * Usa las tablas de Tables.h y el timing de Timing.h.
 */

// =============================================================================
// ESTRUCTURA DATOS MOTOR
// =============================================================================

typedef struct {
    uint16_t rpm;           // RPM actual
    uint16_t map_mbar;      // MAP (mbar)
    uint8_t  tps_pct;       // TPS (%)
    int16_t  clt_cdeg;      // Temperatura refrigerante × 10
    int16_t  iat_cdeg;      // Temperatura aire × 10
    uint8_t  lambda_afr;    // Lambda × 10
    uint16_t batt_mv;       // Tensión batería

    float    advance_deg;   // Avance calculado (grados BTDC)
    uint32_t dwell_us;      // Dwell calculado (µs)
    uint32_t inj_pw_us;     // Pulso inyección (µs)
    float    ve_pct;        // VE interpolada (%)
    float    afr_target;    // AFR objetivo

    uint8_t  mode;          // 0=normal, 1=idle, 2=cranking, 3=warmup
    uint8_t  error_flags;   // Errores activos
} EngineData_t;

// Flags de error
#define ENG_ERR_NO_SYNC     (1 << 0)
#define ENG_ERR_MAP         (1 << 1)
#define ENG_ERR_CLT         (1 << 2)
#define ENG_ERR_LAMBDA      (1 << 3)
#define ENG_ERR_OVERREV     (1 << 4)

// Modos de operación
#define ENG_MODE_NORMAL     0
#define ENG_MODE_IDLE       1
#define ENG_MODE_CRANKING   2
#define ENG_MODE_WARMUP     3

// =============================================================================
// API PÚBLICA
// =============================================================================

/** Inicializa el módulo de control motor. */
void Engine_Init(void);

/**
 * Actualiza sensores ADC y calcula ignición/inyección.
 * Llamar desde Task_EngineCtrl periódicamente.
 */
void Engine_Update(void);

/** @return Puntero a los datos actuales del motor (read-only). */
const EngineData_t* Engine_GetData(void);

// Cálculos individuales (llamados internamente por Engine_Update)
float    Engine_CalcAdvance(uint16_t rpm, uint16_t map_mbar);
uint32_t Engine_CalcDwell(uint16_t batt_mv);
uint32_t Engine_CalcInjPW(uint16_t rpm, uint16_t map_mbar, int16_t clt_cdeg, int16_t iat_cdeg);

#endif // ENGINE_H
