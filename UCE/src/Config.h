#ifndef CONFIG_H
#define CONFIG_H

/**
 * Config.h - UCE (Engine Control Unit)
 * STM32F405RG @ 168MHz
 *
 * Control motor EFI completo para Renault 5 GTL 1989.
 * Motor: 1.1L 4 cilindros, rueda fónica 36-1.
 */

// =============================================================================
// HARDWARE PINS
// =============================================================================

#define PIN_LED_STATUS      PC13

// CAN Bus (STM32F405 CAN1 en PB8/PB9)
#define PIN_CAN_RX          PB8
#define PIN_CAN_TX          PB9

// Rueda fónica (36-1) → Interrupción externa alta prioridad
#define PIN_CRANK_INPUT     PA0     // EXTI0, Timer2 ETR

// Sensor fase (árbol de levas)
#define PIN_CAM_INPUT       PA1     // EXTI1

// Inyectores (4 canales)
#define PIN_INJ_1           PC6     // Timer8 CH1
#define PIN_INJ_2           PC7     // Timer8 CH2
#define PIN_INJ_3           PC8     // Timer8 CH3
#define PIN_INJ_4           PC9     // Timer8 CH4

// Bobinas ignición (4 canales, CDI o bobina)
// IGN_1/2: Timer4 CH1/CH2 en PB6/PB7 — sin conflicto
// IGN_3/4: remapeados a Timer1 CH1/CH2 en PE9/PE10 — libera PB8/PB9 para CAN1
#define PIN_IGN_1           PB6     // Timer4 CH1
#define PIN_IGN_2           PB7     // Timer4 CH2
#define PIN_IGN_3           PE9     // Timer1 CH1 (antes PB8 — conflicto CAN resuelto)
#define PIN_IGN_4           PE10    // Timer1 CH2 (antes PB9 — conflicto CAN resuelto)

// Sensores ADC (ADC1)
#define PIN_ADC_MAP         PA2     // MPX4250AP - presión múltiple
#define PIN_ADC_TPS         PA3     // Potenciómetro TPS 0-5V (divisor a 3.3V)
#define PIN_ADC_CLT         PA4     // NTC 10K - refrigerante
#define PIN_ADC_IAT         PA5     // NTC 10K - aire
#define PIN_ADC_LAMBDA      PA6     // Sonda lambda NTK (0-1V banda estrecha)
#define PIN_ADC_BATT        PA7     // Tensión batería

// Bomba combustible (relé)
#define PIN_FUEL_PUMP       PD0
// IAC (paso a paso o solenoide)
#define PIN_IAC             PD1

// TunerStudio (USB CDC virtual)
// Usar Serial USB del STM32F405 (micro-USB)

// =============================================================================
// MOTOR - Configuración
// =============================================================================

#define ENGINE_CYLINDERS        4
#define ENGINE_CRANK_TEETH      36      // Dientes rueda fónica
#define ENGINE_CRANK_MISSING    1       // Dientes faltantes (36-1)
#define ENGINE_STROKE           4       // 4 tiempos

// Ángulo por diente
#define CRANK_DEGREES_PER_TOOTH (360.0f / ENGINE_CRANK_TEETH)  // 10°

// Posición TDC cilindro 1 (dientes antes del gap)
#define TDC_CYL1_TOOTH          18      // TODO: calibrar con timing light

// =============================================================================
// CALIBRACIÓN ADC
// =============================================================================

#define ADC_VREF_MV             3300
#define ADC_RESOLUTION          4096

// TPS: 0V = 0%, 3.3V = 100% (tras divisor de 5V)
#define TPS_MIN_MV              100     // mV en reposo
#define TPS_MAX_MV              3200    // mV a plena carga

// Lambda banda estrecha: 0V = rico, ~0.45V = estequiométrico, ~1V = pobre
#define LAMBDA_RICH_MV          200
#define LAMBDA_STOICH_MV        450
#define LAMBDA_LEAN_MV          800

// Mismas fórmulas NTC que ICM
#define NTC_RPULL_OHM           10000
#define NTC_BETA                3950
#define NTC_R0_OHM              10000
#define NTC_T0_K                298

// =============================================================================
// IGNICIÓN
// =============================================================================

#define DWELL_DEFAULT_US        3500    // µs tiempo cierre bobina
#define ADVANCE_MIN_DEG         0       // Grados mínimos avance
#define ADVANCE_MAX_DEG         40      // Grados máximos avance

// =============================================================================
// INYECCIÓN
// =============================================================================

#define INJ_DEAD_TIME_US        800     // µs tiempo muerto inyector
#define INJ_MAX_PW_US           20000   // µs pulso máximo (100% ciclo a 3000rpm)

// =============================================================================
// CAN PROTOCOL
// =============================================================================

#define CAN_SPEED               500000
#define CAN_NODE_ID             0x04

// =============================================================================
// TIMING TAREAS (ms)
// =============================================================================

#define TASK_HEARTBEAT_MS       1000
#define TASK_SENSORS_MS         10      // 100 Hz
#define TASK_ENGINE_CTRL_MS     5       // 200 Hz (control principal)
#define TASK_CAN_TX_MS          20      // 50 Hz
#define TASK_CAN_RX_MS          10
#define TASK_TUNER_MS           5       // 200 Hz (TunerStudio latencia baja)
#define TASK_DIAG_MS            500

// =============================================================================
// STACK SIZES
// =============================================================================

#define STACK_HEARTBEAT         128
#define STACK_SENSORS           512
#define STACK_ENGINE_CTRL       512
#define STACK_CAN_TX            256
#define STACK_CAN_RX            256
#define STACK_TUNER             512
#define STACK_DIAG              256

// =============================================================================
// PRIORIDADES
// =============================================================================

#define PRIO_HEARTBEAT          1
#define PRIO_SENSORS            3
#define PRIO_ENGINE_CTRL        4       // Máxima tarea normal
#define PRIO_CAN_TX             2
#define PRIO_CAN_RX             3
#define PRIO_TUNER              2
#define PRIO_DIAG               1

// ISRs van por encima de todas (NVIC)

// =============================================================================
// VERSION
// =============================================================================

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION        "3.0.0"
#endif
#define NODE_NAME               "UCE"

#endif // CONFIG_H
