#ifndef CONFIG_H
#define CONFIG_H

/**
 * Config.h - ICM (Instrumentation Control Module)
 * STM32F103C8 @ 72MHz
 *
 * Sensores: RPM, MAP, CLT, IAT, Batería
 */

// =============================================================================
// HARDWARE PINS
// =============================================================================

#define PIN_LED_STATUS      PC13

// CAN Bus
#define PIN_CAN_RX          PB8
#define PIN_CAN_TX          PB9

// RPM (optoacoplador PC817 en entrada digital con interrupción)
#define PIN_RPM_INPUT       PA0     // Interrupción externa EXTI0

// Sensores ADC (PA1-PA3 → ADC1 canales 1-3)
#define PIN_ADC_MAP         PA1     // MPX4250AP → presión múltiple
#define PIN_ADC_CLT         PA2     // NTC 10K   → temperatura refrigerante
#define PIN_ADC_IAT         PA3     // NTC 10K   → temperatura aire
// PA4 libre (antes divisor batería, sustituido por INA219 vía I2C)

// INA219 — tensión batería vía I2C
// SDA = PB6 (I2C1), SCL = PB7 (I2C1) — pines por defecto Wire en STM32F103
#define INA219_I2C_ADDR     0x40    // ADDR pin a GND (dirección por defecto)

// Poner a 1 cuando el módulo INA219 esté físicamente conectado.
// Con 0: Wire no se inicializa y batt_mv queda a 0 (evita HardFault sin hardware).
#define INA219_ENABLED      0

// =============================================================================
// ADC - CALIBRACIÓN SENSORES
// =============================================================================

// ADC referencia
#define ADC_VREF_MV         3300    // mV
#define ADC_COUNTS          4096    // 12 bits (usar este, no ADC_RESOLUTION que conflicta con Arduino.h)

// Media móvil (PRO-003): número de muestras por canal MAP/CLT/IAT
#define SENSORS_AVG_SAMPLES 8

// MAP - MPX4250AP: Vout = Vcc × (0.00369 × kPa - 0.04)
// Rango: 20kPa (0.25V) → 250kPa (4.75V)
#define MAP_VOUT_MIN_MV     250
#define MAP_VOUT_MAX_MV     4750
#define MAP_KPA_MIN         20
#define MAP_KPA_MAX         250

// CLT / IAT - NTC 10K (divisor con R_pull = 10K)
// Usar tabla Steinhart-Hart o lookup table
#define NTC_RPULL_OHM       10000
#define NTC_BETA            3950
#define NTC_R0_OHM          10000
#define NTC_T0_K            298     // 25°C en Kelvin

// INA219 — rangos de calibración
// Batería coche: 10.5V (descargada) – 14.8V (alternador)
// Usar calibración 32V/2A: bus voltage range 26V HW, resolución 1.6mV
#define INA219_CAL_MAX_V    32      // setCalibration_32V_2A()
#define INA219_BATT_MIN_MV  8000    // Por debajo: batería muy descargada → error
#define INA219_BATT_MAX_MV  16000   // Por encima: fallo regulador → error

// =============================================================================
// RPM - Cálculo
// =============================================================================

// Número de pulsos por revolución del distribuidor (Renault 5 GTL)
#define RPM_PULSES_PER_REV  2       // 4 cilindros / 2 = 2 pulsos por vuelta
#define RPM_TIMEOUT_MS      500     // Sin pulso = 0 RPM

// =============================================================================
// CAN PROTOCOL
// =============================================================================

#define CAN_SPEED           500000
#define CAN_NODE_ID         0x03

// =============================================================================
// IWDG (PRO-009.1)
// =============================================================================

#define IWDG_TIMEOUT_MS         1000
#define WDG_FLAG_SENSORS        (1 << 0)
#define WDG_FLAG_CAN_RX         (1 << 1)
#define WDG_FLAGS_ALL           (WDG_FLAG_SENSORS | WDG_FLAG_CAN_RX)

// =============================================================================
// TIMING TAREAS (ms)
// =============================================================================

#define TASK_HEARTBEAT_MS   1000
#define TASK_SENSORS_MS     50      // 20 Hz lectura sensores
#define TASK_RPM_MS         10      // 100 Hz cálculo RPM
#define TASK_CAN_TX_MS      50      // 20 Hz transmisión
#define TASK_CAN_RX_MS      10
#define TASK_DIAG_MS        500

// =============================================================================
// STACK SIZES
// Reducidos respecto al BCM: Wire + INA219 consumen RAM estática adicional
// en el ICM, dejando menos heap disponible para FreeRTOS.
// =============================================================================

#define STACK_HEARTBEAT     80
#define STACK_SENSORS       192
#define STACK_RPM           80
#define STACK_CAN_TX        128
#define STACK_CAN_RX        128
#define STACK_WATCHDOG      80
#define STACK_DIAG          80

// =============================================================================
// PRIORIDADES
// =============================================================================

#define PRIO_HEARTBEAT      1
#define PRIO_SENSORS        2
#define PRIO_RPM            3       // Alta: tiempo real
#define PRIO_CAN_TX         2
#define PRIO_CAN_RX         4
#define PRIO_WATCHDOG       1
#define PRIO_DIAG           1

// =============================================================================
// VERSION
// =============================================================================

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION    "3.1.0"
#endif
#define NODE_NAME           "ICM"

#endif // CONFIG_H
