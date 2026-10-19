#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include "Protocol.h"

/**
 * Messages.h - Estructuras de datos para los mensajes CAN del sistema Super5 RTOS v3
 *
 * Usar estas estructuras con memcpy hacia/desde los 8 bytes del payload CAN.
 * Todos los campos son little-endian (por defecto en ARM Cortex-M).
 */

// =============================================================================
// HEARTBEAT (compartido por todos los nodos)
// =============================================================================
typedef struct __attribute__((packed)) {
    uint8_t  status;        // FLAGS: STATUS_OK, STATUS_ERROR, etc.
    uint32_t uptime_s;      // Segundos desde el arranque
    uint8_t  reserved[3];
} MSG_Heartbeat_t;

// =============================================================================
// BCM - Body Control Module
// =============================================================================
typedef struct __attribute__((packed)) {
    uint8_t light_flags;    // bitmask LIGHT_*
    uint8_t reserved[7];
} MSG_BCM_LightsState_t;

typedef struct __attribute__((packed)) {
    uint8_t pattern;        // 0-12: número de patrón flames
    uint8_t speed;          // 0-255: velocidad animación
    uint8_t color;          // 0-7: selección color (si aplica HW)
    uint8_t reserved[5];
} MSG_BCM_FlameConfig_t;

typedef struct __attribute__((packed)) {
    uint8_t active;         // 0=inactivo, 1=activo
    uint8_t pattern;        // Patrón ejecutándose
    uint8_t frame;          // Frame actual de la animación
    uint8_t reserved[5];
} MSG_BCM_FlameState_t;

// =============================================================================
// ICM - Instrumentation Control Module
// =============================================================================
typedef struct __attribute__((packed)) {
    uint16_t rpm;           // RPM motor (0-9000)
    uint16_t map_mbar;      // Presión múltiple admisión (mbar)
    int8_t   clt_c;         // Temperatura refrigerante (°C, -40..+150)
    int8_t   iat_c;         // Temperatura aire admisión (°C, -40..+125)
    uint8_t  batt_dv;       // Tensión batería en deciVoltios (120 = 12.0V)
    uint8_t  reserved;
} MSG_ICM_EngineData_t;

typedef struct __attribute__((packed)) {
    uint8_t  flags;         // Flags estado ICM
    uint8_t  errors;        // Bitmask errores sensores
    uint8_t  reserved[6];
} MSG_ICM_Status_t;

typedef struct __attribute__((packed)) {
    uint16_t adc0;          // ADC canal 0 raw (0-4095)
    uint16_t adc1;          // ADC canal 1 raw
    uint16_t adc2;          // ADC canal 2 raw
    uint16_t adc3;          // ADC canal 3 raw
} MSG_ICM_AdcRaw_t;

// =============================================================================
// UCE - Engine Control Unit
// =============================================================================
typedef struct __attribute__((packed)) {
    uint16_t rpm;           // RPM motor (0-9000)
    uint16_t map_mbar;      // MAP (mbar)
    uint8_t  tps_pct;       // Posición acelerador (0-100%)
    uint8_t  lambda_afr;    // Lambda × 10 (140 = AFR 14.0)
    uint8_t  reserved[2];
} MSG_UCE_Engine_t;

typedef struct __attribute__((packed)) {
    int16_t clt_cdeg;       // Temp. refrigerante × 10 (°C)
    int16_t iat_cdeg;       // Temp. aire × 10 (°C)
    uint8_t reserved[4];
} MSG_UCE_Temps_t;

typedef struct __attribute__((packed)) {
    int16_t  advance_cdeg;  // Avance encendido × 10 (grados ATDC)
    uint16_t dwell_us;      // Tiempo cierre bobina (µs)
    uint8_t  knock;         // Nivel knock (0-255)
    uint8_t  reserved[3];
} MSG_UCE_Ignition_t;

typedef struct __attribute__((packed)) {
    uint16_t pw_us;         // Pulso inyección (µs)
    uint8_t  ve_pct;        // Eficiencia volumétrica (%)
    uint8_t  corrections;   // Bitmask correcciones activas
    uint8_t  reserved[4];
} MSG_UCE_Injection_t;

// =============================================================================
// COMANDOS (Gateway → nodos)
// =============================================================================
typedef struct __attribute__((packed)) {
    uint8_t  flags;         // Bitmask LIGHT_*
    uint8_t  reserved[7];
} MSG_CMD_Lights_t;

typedef struct __attribute__((packed)) {
    uint8_t  pattern;       // Patrón flames
    uint8_t  speed;         // Velocidad
    uint8_t  reserved[6];
} MSG_CMD_Flame_t;

typedef struct __attribute__((packed)) {
    uint8_t  table;         // 0=VE, 1=Ignition, 2=AFR
    uint8_t  idx;           // Índice en la tabla
    uint16_t value;         // Valor a escribir
    uint8_t  reserved[4];
} MSG_CMD_Tune_t;

// =============================================================================
// TAMAÑO MÁXIMO PAYLOAD CAN
// =============================================================================
#define CAN_MAX_DLC  8

#endif // MESSAGES_H
