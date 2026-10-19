#ifndef TIMING_H
#define TIMING_H

#include <Arduino.h>
#include <stdint.h>

/**
 * Timing.h - UCE: ISRs y gestión de timing crítico
 *
 * Funciones que manejan la rueda fónica 36-1 y los pulsos de ignición/inyección.
 * Estas rutinas son críticas en tiempo. NO usar malloc, printf, ni delays aquí.
 *
 * Flujo de eventos:
 *   ISR_CrankTooth → actualiza posición cigüeñal → programa timers IGN/INJ
 *   ISR_CamPulse   → determina ciclo (fase)
 *   ISR_IgnDwell   → cierra bobina (inicio dwell)
 *   ISR_IgnFire    → dispara bobina
 *   ISR_InjOpen    → abre inyector
 *   ISR_InjClose   → cierra inyector
 */

// =============================================================================
// TIPOS
// =============================================================================

typedef struct {
    volatile uint32_t tooth_count;          // Dientes contados desde el gap
    volatile uint32_t tooth_period_us;      // Período último diente (µs)
    volatile uint32_t last_tooth_us;        // Timestamp último diente
    volatile bool     gap_detected;         // Gap 36-1 detectado
    volatile bool     synced;               // Sincronizado (fase conocida)
    volatile uint16_t rpm;                  // RPM calculado
} CrankState_t;

typedef struct {
    volatile bool     phase_known;          // Fase (ciclo) conocida
    volatile uint8_t  cycle_tooth;          // Diente del cigüeñal en el pulso de CAM
} CamState_t;

// =============================================================================
// ESTADO COMPARTIDO (extern, accedido desde Engine.cpp)
// =============================================================================

extern volatile CrankState_t g_crank;
extern volatile CamState_t   g_cam;

// =============================================================================
// API ISRs (enlazadas a NVIC externamente)
// =============================================================================

/**
 * ISR: Flanco de diente en rueda fónica.
 * Detecta gap, calcula período, actualiza posición.
 * Tiempo objetivo: < 2µs
 */
void ISR_CrankTooth(void);

/**
 * ISR: Pulso sensor fase (árbol de levas).
 * Determina en qué ciclo estamos.
 */
void ISR_CamPulse(void);

// =============================================================================
// FUNCIONES DE PROGRAMACIÓN DE EVENTOS
// =============================================================================

/**
 * Programa el disparo de ignición del cilindro indicado.
 * @param cylinder  0-3
 * @param advance_deg Avance en grados BTDC
 * @param dwell_us    Tiempo de dwell en µs
 */
void Timing_ScheduleIgnition(uint8_t cylinder, float advance_deg, uint32_t dwell_us);

/**
 * Programa la apertura del inyector.
 * @param cylinder  0-3
 * @param pw_us     Ancho de pulso en µs
 * @param soi_deg   Start of Injection en grados BTDC
 */
void Timing_ScheduleInjection(uint8_t cylinder, uint32_t pw_us, float soi_deg);

/**
 * Inicializa timers hardware para ignición e inyección.
 */
void Timing_Init(void);

/**
 * @return RPM calculado desde los últimos dientes.
 */
uint16_t Timing_GetRPM(void);

/**
 * @return true si la ECU está sincronizada con el cigüeñal.
 */
bool Timing_IsSynced(void);

#endif // TIMING_H
