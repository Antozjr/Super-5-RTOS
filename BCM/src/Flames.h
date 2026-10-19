#ifndef FLAMES_H
#define FLAMES_H

#include <Arduino.h>
#include <stdint.h>

/**
 * Flames.h - BCM: Sistema de efectos flames
 *
 * Gestiona 13 patrones de flames programables.
 * Cada patrón es una secuencia de pulsos PWM en PIN_FLAME_OUT.
 */

// =============================================================================
// CONSTANTES
// =============================================================================

#define FLAMES_NUM_PATTERNS     13
#define FLAMES_MAX_FRAMES       32   // Frames por patrón
#define FLAMES_PWM_FREQ_HZ      1000 // Frecuencia PWM de salida

// =============================================================================
// TIPOS
// =============================================================================

typedef struct {
    uint8_t duty;       // Duty cycle PWM (0-255)
    uint16_t duration;  // Duración del frame (ms)
} FlameFrame_t;

typedef struct {
    const char*   name;
    uint8_t       num_frames;
    FlameFrame_t  frames[FLAMES_MAX_FRAMES];
} FlamePattern_t;

// =============================================================================
// API PÚBLICA
// =============================================================================

/** Inicializa el sistema de flames (PWM, patrones). */
void Flames_Init(void);

/**
 * Activa un patrón de flames.
 * @param pattern  Número de patrón (0 - FLAMES_NUM_PATTERNS-1)
 * @param speed    Velocidad (1-255, 255=máxima)
 */
void Flames_Start(uint8_t pattern, uint8_t speed);

/** Detiene cualquier efecto flames activo. */
void Flames_Stop(void);

/**
 * Actualiza la animación (llamar cada TASK_FLAMES_MS ms).
 * Avanza el frame y aplica el PWM correspondiente.
 */
void Flames_Update(void);

/** @return true si hay un efecto activo actualmente. */
bool Flames_IsActive(void);

/** @return Número de patrón activo (0-12). */
uint8_t Flames_GetPattern(void);

/** @return Frame actual de la animación. */
uint8_t Flames_GetFrame(void);

#endif // FLAMES_H
