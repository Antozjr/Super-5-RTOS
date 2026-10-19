#include "Timing.h"
#include "Config.h"

/**
 * Timing.cpp - UCE: ISRs y timing crítico
 *
 * IMPORTANTE: Estas rutinas se ejecutan en contexto de interrupción.
 * NO llamar funciones de FreeRTOS que no sean ISR-safe.
 * NO usar printf, malloc, ni ninguna función que pueda bloquear.
 *
 * TODO: Implementar con timers hardware (TIM2 como contador de dientes).
 * TODO: Validar brecha 36-1 por período largo (2× período normal).
 * TODO: Programar TIM1 para salidas ignición (modo comparación).
 */

// =============================================================================
// ESTADO COMPARTIDO (definición de extern en Timing.h)
// =============================================================================

volatile CrankState_t g_crank = {};
volatile CamState_t   g_cam   = {};

// =============================================================================
// INIT
// =============================================================================

void Timing_Init(void) {
    // --- Configurar pin rueda fónica ---
    pinMode(PIN_CRANK_INPUT, INPUT);
    // attachInterrupt(digitalPinToInterrupt(PIN_CRANK_INPUT), ISR_CrankTooth, RISING);

    // --- Configurar pin árbol de levas ---
    pinMode(PIN_CAM_INPUT, INPUT);
    // attachInterrupt(digitalPinToInterrupt(PIN_CAM_INPUT), ISR_CamPulse, RISING);

    // --- TODO: Configurar TIM2 como contador de flancos ---
    // TIM2->CR1 = 0;
    // TIM2->PSC = 0;                 // Sin prescaler (168MHz)
    // TIM2->ARR = 0xFFFFFFFF;        // TIM2 es 32 bits en F405
    // TIM2->CR1 |= TIM_CR1_CEN;     // Habilitar

    // --- TODO: Configurar TIM1 CH1-CH4 para ignición ---
    // (modo comparación, OC toggle o PWM negado)

    memset((void*)&g_crank, 0, sizeof(g_crank));
    memset((void*)&g_cam,   0, sizeof(g_cam));

    Serial.println(F("[Timing] Init OK - Esperando sincronismo..."));
}

// =============================================================================
// ISR RUEDA FÓNICA (36-1)
// =============================================================================

void IRAM_ATTR ISR_CrankTooth(void) {
    uint32_t now_us = micros();
    uint32_t period = now_us - g_crank.last_tooth_us;
    g_crank.last_tooth_us = now_us;

    // Detección de brecha: período > 1.5× período normal
    if (g_crank.tooth_period_us > 0 && period > (g_crank.tooth_period_us * 3 / 2)) {
        // Brecha detectada → reset contador
        g_crank.tooth_count  = 0;
        g_crank.gap_detected = true;

        // Si también tenemos fase de CAM, estamos sincronizados
        if (g_cam.phase_known) {
            g_crank.synced = true;
        }
    } else {
        g_crank.tooth_count++;
    }

    g_crank.tooth_period_us = period;

    // Calcular RPM: 60s × 1e6µs / (period_us × teeth_per_rev)
    // teeth_per_rev = ENGINE_CRANK_TEETH - ENGINE_CRANK_MISSING = 35
    if (period > 0) {
        g_crank.rpm = (uint16_t)(60000000UL / ((uint32_t)period * 35));
        if (g_crank.rpm > 9000) g_crank.rpm = 9000;
    }

    // TODO: Programar evento de ignición para el cilindro que toca
    // Calcular cuántos µs faltan hasta TDC - advance_deg
    // Programar TIM1 capture compare register
}

// =============================================================================
// ISR SENSOR FASE (árbol de levas)
// =============================================================================

void IRAM_ATTR ISR_CamPulse(void) {
    // Registrar en qué diente del cigüeñal estamos cuando llega el pulso de CAM
    g_cam.cycle_tooth = (uint8_t)g_crank.tooth_count;
    g_cam.phase_known = true;

    // Con esto sabemos si estamos en la primera o segunda vuelta del ciclo 4T
    // y podemos asignar el orden de encendido correcto
}

// =============================================================================
// PROGRAMAR EVENTOS
// =============================================================================

void Timing_ScheduleIgnition(uint8_t cylinder, float advance_deg, uint32_t dwell_us) {
    if (!g_crank.synced) return;
    if (cylinder >= ENGINE_CYLINDERS) return;

    // TODO: Calcular dientes hasta el evento de ignición
    // float teeth_until_fire = (TDC_CYL1_TOOTH - advance_deg / CRANK_DEGREES_PER_TOOTH);
    // float time_until_fire_us = teeth_until_fire × g_crank.tooth_period_us;
    // Programar TIM1->CCR[cylinder] = TIM1->CNT + time_until_fire_us * (timer_freq/1e6)
    (void)advance_deg; (void)dwell_us;
}

void Timing_ScheduleInjection(uint8_t cylinder, uint32_t pw_us, float soi_deg) {
    if (!g_crank.synced) return;
    if (cylinder >= ENGINE_CYLINDERS) return;

    // TODO: Similar a ignición, programar apertura y cierre inyector
    (void)pw_us; (void)soi_deg;
}

// =============================================================================
// GETTERS
// =============================================================================

uint16_t Timing_GetRPM(void) {
    // Timeout: si no hay pulso en RPM_TIMEOUT_MS → 0
    if ((millis() - g_crank.last_tooth_us / 1000) > 500) return 0;
    return g_crank.rpm;
}

bool Timing_IsSynced(void) {
    return g_crank.synced;
}
