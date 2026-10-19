#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <stdint.h>

/**
 * Sensors.h - ICM: Interfaz lectura de sensores
 *
 * Sensores disponibles:
 *   - RPM  (optoacoplador PC817, ISR EXTI0)
 *   - MAP  (MPX4250AP, ADC PA1)
 *   - CLT  (NTC 10K, ADC PA2)
 *   - IAT  (NTC 10K, ADC PA3)
 *   - BATT (INA219 I2C 0x40, PB6/PB7)
 */

// =============================================================================
// ESTRUCTURA DATOS SENSORES
// =============================================================================

typedef struct {
    uint16_t rpm;           // RPM (0-9000)
    uint16_t map_mbar;      // Presión múltiple admisión (mbar)
    int16_t  clt_cdeg;      // Temperatura refrigerante × 10 (°C)
    int16_t  iat_cdeg;      // Temperatura aire × 10 (°C)
    uint16_t batt_mv;       // Tensión batería (mV)
    uint8_t  error_flags;   // Bitmask errores (ver ERROR_*)
} SensorData_t;

// Flags de error de sensores
#define SENSOR_ERR_RPM      (1 << 0)
#define SENSOR_ERR_MAP      (1 << 1)
#define SENSOR_ERR_CLT      (1 << 2)
#define SENSOR_ERR_IAT      (1 << 3)
#define SENSOR_ERR_BATT     (1 << 4)

// =============================================================================
// API PÚBLICA
// =============================================================================

/** Inicializa ADC y configura interrupción RPM. */
void Sensors_Init(void);

/** Lee todos los sensores ADC y actualiza g_sensor_data. */
void Sensors_ReadAll(void);

/**
 * ISR llamada en cada pulso del optoacoplador RPM.
 * Calcular período y derivar RPM.
 * DEBE ser rápida (no hacer operaciones lentas aquí).
 */
void Sensors_RPM_ISR(void);

/**
 * Actualiza el cálculo de RPM (llamar periódicamente).
 * Detecta timeout (0 RPM si no hay pulso en RPM_TIMEOUT_MS).
 */
void Sensors_RPM_Update(void);

/** @return Puntero a los datos actuales de sensores (thread-safe con mutex). */
const SensorData_t* Sensors_GetData(void);

// Getters individuales
uint16_t Sensors_GetRPM(void);
uint16_t Sensors_GetMAP(void);      // mbar
int16_t  Sensors_GetCLT(void);      // °C × 10
int16_t  Sensors_GetIAT(void);      // °C × 10
uint16_t Sensors_GetBatt(void);     // mV

#endif // SENSORS_H
