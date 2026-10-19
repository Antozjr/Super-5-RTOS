#include "Flames.h"
#include "Config.h"

/**
 * Flames.cpp - BCM: Implementación sistema flames
 *
 * 13 patrones predefinidos. Cada patrón es una tabla de frames con
 * duty cycle y duración. La velocidad escala la duración de cada frame.
 *
 * TODO: Cargar/guardar patrones personalizados desde EEPROM.
 */

// =============================================================================
// PATRONES PREDEFINIDOS
// =============================================================================

static const FlamePattern_t g_patterns[FLAMES_NUM_PATTERNS] = {

    // 0 - Single burst
    { "Single burst", 4, {
        {255, 50}, {180, 30}, {100, 20}, {0, 100}
    }},

    // 1 - Double tap
    { "Double tap", 6, {
        {255, 40}, {0, 30}, {255, 40}, {180, 20}, {80, 20}, {0, 200}
    }},

    // 2 - Long burn
    { "Long burn", 6, {
        {200, 100}, {255, 150}, {220, 100}, {180, 80}, {100, 60}, {0, 50}
    }},

    // 3 - Stutter
    { "Stutter", 10, {
        {255,20},{0,15},{255,20},{0,15},{255,20},{0,15},
        {200,30},{100,20},{50,20},{0,100}
    }},

    // 4 - Slow build
    { "Slow build", 8, {
        {30,50},{60,50},{100,50},{150,50},{200,50},{255,80},{180,60},{0,100}
    }},

    // 5 - Machine gun
    { "Machine gun", 12, {
        {255,15},{0,10},{255,15},{0,10},{255,15},{0,10},
        {255,15},{0,10},{255,15},{0,10},{200,20},{0,80}
    }},

    // 6 - Fade out
    { "Fade out", 6, {
        {255,40},{210,50},{160,60},{100,70},{50,60},{0,50}
    }},

    // 7 - Pop
    { "Pop", 3, {
        {255,25}, {100,20}, {0,200}
    }},

    // 8 - Sequence
    { "Sequence", 8, {
        {255,30},{0,20},{255,30},{0,40},{200,50},{0,20},{150,40},{0,150}
    }},

    // 9 - Rumble
    { "Rumble", 10, {
        {180,40},{220,30},{180,40},{200,30},{220,40},
        {180,30},{200,40},{220,30},{150,40},{0,100}
    }},

    // 10 - Firework
    { "Firework", 8, {
        {50,30},{100,30},{200,30},{255,40},{200,30},{130,30},{60,30},{0,150}
    }},

    // 11 - Pulse
    { "Pulse", 6, {
        {255,60},{200,40},{255,60},{200,40},{255,60},{0,80}
    }},

    // 12 - Rapid fire
    { "Rapid fire", 14, {
        {255,12},{0,8},{255,12},{0,8},{255,12},{0,8},{255,12},{0,8},
        {255,12},{0,8},{255,12},{0,8},{200,30},{0,100}
    }},
};

// =============================================================================
// ESTADO INTERNO
// =============================================================================

static bool    g_active         = false;
static uint8_t g_current_pattern= 0;
static uint8_t g_current_frame  = 0;
static uint8_t g_speed          = 128;
static uint32_t g_frame_deadline= 0;

// =============================================================================
// IMPLEMENTACIÓN
// =============================================================================

void Flames_Init(void) {
    // PA8 = Timer1 CH1 — analogWrite activa PWM automáticamente en STM32duino
    analogWrite(PIN_FLAME_OUT, 0);

    g_active = false;
    g_current_frame = 0;
    Serial.print(F("[Flames] Init OK - "));
    Serial.print(FLAMES_NUM_PATTERNS);
    Serial.println(F(" patrones, PWM activo"));
}

void Flames_Start(uint8_t pattern, uint8_t speed) {
    if (pattern >= FLAMES_NUM_PATTERNS) {
        Serial.printf("[Flames] Patrón inválido: %d\n", pattern);
        return;
    }

    g_current_pattern = pattern;
    g_current_frame   = 0;
    g_speed           = (speed == 0) ? 1 : speed;
    g_active          = true;
    g_frame_deadline  = millis();

    Serial.printf("[Flames] Start patrón=%d (%s) speed=%d\n",
                  pattern, g_patterns[pattern].name, speed);
}

void Flames_Stop(void) {
    g_active = false;
    g_current_frame = 0;
    analogWrite(PIN_FLAME_OUT, 0);
    Serial.println(F("[Flames] Stop"));
}

void Flames_Update(void) {
    if (!g_active) return;

    uint32_t now = millis();
    if (now < g_frame_deadline) return;  // Todavía no es hora del siguiente frame

    const FlamePattern_t& pat = g_patterns[g_current_pattern];
    const FlameFrame_t&   frm = pat.frames[g_current_frame];

    analogWrite(PIN_FLAME_OUT, frm.duty);

    // Calcular duración escalada por velocidad (más speed = más rápido)
    uint32_t duration_ms = (uint32_t)frm.duration * 255 / g_speed;
    if (duration_ms < 5) duration_ms = 5;
    g_frame_deadline = now + duration_ms;

    // Avanzar frame
    g_current_frame++;
    if (g_current_frame >= pat.num_frames) {
        // Patrón completado: detener (no loop)
        Flames_Stop();
    }
}

bool Flames_IsActive(void) {
    return g_active;
}

uint8_t Flames_GetPattern(void) {
    return g_current_pattern;
}

uint8_t Flames_GetFrame(void) {
    return g_current_frame;
}
