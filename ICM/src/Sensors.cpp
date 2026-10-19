#include "Sensors.h"
#include "Config.h"
#include <math.h>
#include <STM32FreeRTOS.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

/**
 * Sensors.cpp - ICM: Implementación lectura de sensores
 *
 * TODO: Calibrar las tablas NTC con los sensores reales del vehículo.
 * TODO: Verificar divisor de batería con voltímetro al arrancar.
 * TODO: Ajustar RPM_PULSES_PER_REV si la lectura no coincide con tacómetro original.
 */

// =============================================================================
// ESTADO INTERNO
// =============================================================================

static SensorData_t      g_sensor_data = {};
static SemaphoreHandle_t g_data_mutex  = nullptr;
static Adafruit_INA219   g_ina219(INA219_I2C_ADDR);

// RPM - variables actualizadas desde ISR (volatile)
static volatile uint32_t g_rpm_last_pulse_us = 0;
static volatile uint32_t g_rpm_period_us      = 0;
static volatile bool     g_rpm_new_pulse       = false;

// Media móvil ADC — PRO-003
static uint16_t g_map_buffer[SENSORS_AVG_SAMPLES] = {};
static uint16_t g_clt_buffer[SENSORS_AVG_SAMPLES] = {};
static uint16_t g_iat_buffer[SENSORS_AVG_SAMPLES] = {};
static uint8_t  g_avg_index = 0;

static uint16_t get_average(const uint16_t* buf) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SENSORS_AVG_SAMPLES; i++) sum += buf[i];
    return (uint16_t)(sum / SENSORS_AVG_SAMPLES);
}

// =============================================================================
// INIT
// =============================================================================

void Sensors_Init(void) {
    g_data_mutex = xSemaphoreCreateMutex();
    configASSERT(g_data_mutex != nullptr);

    // --- ADC ---
    analogReadResolution(12);   // 12 bits, 0–4095

    // --- INA219 (tensión batería) ---
#if INA219_ENABLED
    Wire.begin();   // I2C1: SDA=PB6, SCL=PB7 (por defecto en STM32F103)
    if (!g_ina219.begin()) {
        Serial.println(F("[Sensors] WARN: INA219 no encontrado en 0x40"));
        // No es fatal; batt_mv quedará a 0 y se seteará SENSOR_ERR_BATT
    } else {
        g_ina219.setCalibration_32V_2A();
        Serial.println(F("[Sensors] INA219 OK"));
    }
#else
    Serial.println(F("[Sensors] INA219 desactivado (INA219_ENABLED=0)"));
#endif

    // --- RPM ISR ---
    // INPUT_PULLDOWN: mantiene PA0 en LOW cuando el optoacoplador no está conectado,
    // evitando flancos espúreos que dispararían auto-flames en banco.
    pinMode(PIN_RPM_INPUT, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(PIN_RPM_INPUT), Sensors_RPM_ISR, RISING);

    memset(&g_sensor_data, 0, sizeof(g_sensor_data));
    Serial.println(F("[Sensors] Init OK"));
}

// =============================================================================
// LECTURA ADC
// =============================================================================

/**
 * Convierte lectura ADC de MAP (MPX4250AP) a mbar.
 * Fórmula sensor: Vout = Vcc × (0.00369 × kPa - 0.04)
 * → kPa = (Vout/Vcc + 0.04) / 0.00369
 */
static uint16_t adcToMAP_mbar(uint16_t adc_raw) {
    uint32_t vout_mv = (uint32_t)adc_raw * ADC_VREF_MV / ADC_COUNTS;

    // Clamp al rango del sensor
    if (vout_mv < MAP_VOUT_MIN_MV) return MAP_KPA_MIN * 10;
    if (vout_mv > MAP_VOUT_MAX_MV) return MAP_KPA_MAX * 10;

    // kPa = (Vout/3300 + 0.04) / 0.00369
    float ratio = (float)vout_mv / ADC_VREF_MV;
    float kpa   = (ratio + 0.04f) / 0.00369f;

    return (uint16_t)(kpa * 10.0f);   // Devolver en decibar (1 kPa = 10 mbar × 0.1)
                                        // TODO: verificar unidades con el resto del sistema
}

/**
 * Convierte lectura ADC de NTC 10K a temperatura × 10 (en °C).
 * Usa ecuación Steinhart-Hart simplificada (beta).
 */
static int16_t adcToNTC_cdeg(uint16_t adc_raw) {
    if (adc_raw == 0) return -400;   // Corto → error

    // Resistencia NTC: R = R_pull × ADC / (ADC_MAX - ADC)
    float r_ntc = (float)NTC_RPULL_OHM * adc_raw / (ADC_COUNTS - adc_raw);

    if (r_ntc <= 0.0f) return 1500; // Circuito abierto → error

    // Steinhart-Hart beta: 1/T = 1/T0 + (1/B) × ln(R/R0)
    float inv_t = 1.0f / NTC_T0_K + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0_OHM);
    float temp_k = 1.0f / inv_t;
    float temp_c = temp_k - 273.15f;

    return (int16_t)(temp_c * 10.0f);
}

/**
 * Lee tensión de batería en mV desde el INA219 vía I2C.
 * El INA219 mide el bus voltage directamente (0–26V, resolución 1.6mV).
 */
static uint16_t ina219ToBatt_mv(void) {
#if INA219_ENABLED
    float bus_v = g_ina219.getBusVoltage_V();
    if (bus_v < 0.0f) return 0;
    return (uint16_t)(bus_v * 1000.0f);
#else
    return 0;   // Sin hardware — batería reportada como 0
#endif
}

void Sensors_ReadAll(void) {
    // Llenar buffers circulares con lecturas ADC crudas (PRO-003)
    g_map_buffer[g_avg_index] = analogRead(PIN_ADC_MAP);
    g_clt_buffer[g_avg_index] = analogRead(PIN_ADC_CLT);
    g_iat_buffer[g_avg_index] = analogRead(PIN_ADC_IAT);
    g_avg_index = (g_avg_index + 1) % SENSORS_AVG_SAMPLES;

    // Calcular medias antes de convertir — filtra EMI de alternador/bobina
    uint16_t avg_map = get_average(g_map_buffer);
    uint16_t avg_clt = get_average(g_clt_buffer);
    uint16_t avg_iat = get_average(g_iat_buffer);

    uint8_t errors = 0;

    uint16_t map_mbar = adcToMAP_mbar(avg_map);
    int16_t  clt_cdeg = adcToNTC_cdeg(avg_clt);
    int16_t  iat_cdeg = adcToNTC_cdeg(avg_iat);
    uint16_t batt_mv  = ina219ToBatt_mv();

    // Detección de errores sobre valores promediados
    if (map_mbar < 100 || map_mbar > 2600) errors |= SENSOR_ERR_MAP;
    if (clt_cdeg < -400 || clt_cdeg > 1500) errors |= SENSOR_ERR_CLT;
    if (iat_cdeg < -400 || iat_cdeg > 900)  errors |= SENSOR_ERR_IAT;
    if (batt_mv  < INA219_BATT_MIN_MV || batt_mv > INA219_BATT_MAX_MV) errors |= SENSOR_ERR_BATT;

    if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        g_sensor_data.map_mbar    = map_mbar;
        g_sensor_data.clt_cdeg    = clt_cdeg;
        g_sensor_data.iat_cdeg    = iat_cdeg;
        g_sensor_data.batt_mv     = batt_mv;
        g_sensor_data.error_flags = errors;
        xSemaphoreGive(g_data_mutex);
    }
}

// =============================================================================
// RPM
// =============================================================================

void Sensors_RPM_ISR(void) {
    uint32_t now_us = micros();
    if (g_rpm_last_pulse_us > 0) {
        g_rpm_period_us = now_us - g_rpm_last_pulse_us;
    }
    g_rpm_last_pulse_us = now_us;
    g_rpm_new_pulse = true;
}

void Sensors_RPM_Update(void) {
    uint32_t now_ms = millis();

    // Timeout: si no hay pulso en RPM_TIMEOUT_MS → RPM = 0
    uint32_t last_pulse_ms = g_rpm_last_pulse_us / 1000;
    if ((now_ms - last_pulse_ms) > RPM_TIMEOUT_MS) {
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            g_sensor_data.rpm = 0;
            xSemaphoreGive(g_data_mutex);
        }
        return;
    }

    if (!g_rpm_new_pulse) return;
    g_rpm_new_pulse = false;

    // RPM = 60s / (period_s × pulses_per_rev)
    // period_s = g_rpm_period_us / 1e6
    // RPM = 60e6 / (period_us × pulses_per_rev)
    uint32_t period_us = g_rpm_period_us;
    if (period_us == 0) return;

    uint16_t rpm = (uint16_t)(60000000UL / ((uint32_t)period_us * RPM_PULSES_PER_REV));

    // Clamp a rango razonable
    if (rpm > 9000) rpm = 9000;

    if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        g_sensor_data.rpm = rpm;
        xSemaphoreGive(g_data_mutex);
    }
}

// =============================================================================
// GETTERS
// =============================================================================

const SensorData_t* Sensors_GetData(void) {
    return &g_sensor_data;
}

uint16_t Sensors_GetRPM(void)  { return g_sensor_data.rpm; }
uint16_t Sensors_GetMAP(void)  { return g_sensor_data.map_mbar; }
int16_t  Sensors_GetCLT(void)  { return g_sensor_data.clt_cdeg; }
int16_t  Sensors_GetIAT(void)  { return g_sensor_data.iat_cdeg; }
uint16_t Sensors_GetBatt(void) { return g_sensor_data.batt_mv; }
