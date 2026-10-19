#ifndef CONFIG_H
#define CONFIG_H

/**
 * Config.h - BCM (Body Control Module)
 * STM32F103C8 @ 72MHz
 *
 * Centraliza todos los pines y constantes del nodo BCM.
 * Modificar aquí antes de compilar si el hardware cambia.
 */

// =============================================================================
// HARDWARE PINS
// =============================================================================

// LED de estado (activo en bajo en Blue Pill)
#define PIN_LED_STATUS      PC13

// CAN Bus (STM32F103 CAN remapeado a PB8/PB9)
#define PIN_CAN_RX          PB8
#define PIN_CAN_TX          PB9

// Bocina
#define PIN_HORN_OUT        PA3     // Salida relé bocina (Sec87 pos.9)

// Luces — salidas relé
#define PIN_LIGHT_POS       PB0     // Luces posición     (Sec87 pos.6)
#define PIN_LIGHT_DEL       PB1     // Luces delanteras   (Sec87 pos.10 — cruce/largas via palanca)
#define PIN_LIGHT_FOG_F     PB4     // Antiniebla del.    (Sec87 pos.8)
#define PIN_LIGHT_FOG_R     PB5     // Antiniebla tras.   (Sec81 conector dorado vía 6)

// Entradas mando — 12V coche → PC817 → STM32 (pulldown 10kΩ externo)
#define PIN_IN_HORN         PA4     // Botón bocina stalk (pin86 zócalo pos.9)
#define PIN_IN_LIGHT_POS    PA5     // Mando luces posición (pin86 zócalo pos.6)
#define PIN_IN_LIGHT_DEL    PA6     // Mando luces delanteras (pin86 zócalo pos.10)
#define PIN_IN_FOG_F        PA7     // Interruptor antiniebla del. (pin86 zócalo pos.8)
#define PIN_IN_FOG_R        PB10    // Interruptor antiniebla tras.

// Flames (salida PWM o digital hacia controlador pirotécnico)
#define PIN_FLAME_OUT       PA8     // Salida principal flames (Timer1 CH1)
#define PIN_FLAME_TRIGGER   PA9     // Trigger disparo

// EEPROM (I2C AT24C32 o similar)
#define PIN_I2C_SDA         PB6
#define PIN_I2C_SCL         PB7

// =============================================================================
// CAN PROTOCOL
// =============================================================================
#define CAN_SPEED           500000  // 500 kbps
#define CAN_NODE_ID         0x01    // ID del nodo BCM

// Filtros CAN: aceptar comandos hacia BCM
#define CAN_FILTER_ID       0x180   // Base comandos BCM
#define CAN_FILTER_MASK     0x780   // Aceptar 0x180-0x18F y 0x7FF (broadcast)

// =============================================================================
// TIMING TAREAS (ms)
// =============================================================================
#define TASK_HEARTBEAT_MS   1000
#define TASK_CAN_TX_MS      50
#define TASK_CAN_RX_MS      10
#define TASK_MONITOR_MS     20
#define TASK_HORN_MS        20
#define TASK_FLAMES_MS      33     // ~30 fps animación flames

// =============================================================================
// STACK SIZES (palabras de 32 bits)
// =============================================================================
#define STACK_HEARTBEAT     128
#define STACK_CAN_TX        256
#define STACK_CAN_RX        256
#define STACK_MONITOR       256
#define STACK_HORN          128
#define STACK_FLAMES        512

// =============================================================================
// PRIORIDADES TAREAS (mayor número = mayor prioridad)
// =============================================================================
#define PRIO_HEARTBEAT      1
#define PRIO_MONITOR        3
#define PRIO_HORN           3
#define PRIO_FLAMES         2
#define PRIO_CAN_TX         2
#define PRIO_CAN_RX         4   // Máxima: respuesta rápida al bus CAN

// =============================================================================
// FLAMES - Configuración
// =============================================================================
#define FLAME_PATTERNS      13   // Número de patrones disponibles
#define FLAME_MAX_SPEED     255
#define FLAME_DEFAULT_PATTERN 0

// =============================================================================
// WATCHDOG (IWDG) — PRO-001
// =============================================================================
#define IWDG_TIMEOUT_MS         1000    // 1s — margen suficiente durante boot

// Flags de las tareas críticas que deben estar vivas para kik ear el IWDG
#define WDG_FLAG_CAN_RX         (1 << 0)
#define WDG_FLAG_MONITOR        (1 << 1)
#define WDG_FLAGS_ALL           (WDG_FLAG_CAN_RX | WDG_FLAG_MONITOR)

// =============================================================================
// FAIL-SAFE CAN — PRO-002
// =============================================================================
#define FAILSAFE_CAN_TIMEOUT_MS      5000UL           // 5s sin CAN → modo seguro
#define FAILSAFE_LIGHTS_TIMEOUT_MS   (5UL*60UL*1000UL) // 5min en fail-safe → apagar luces

// =============================================================================
// BOCINA
// =============================================================================
#define HORN_QUEUE_LEN          4       // Comandos on/off en la cola FreeRTOS

// =============================================================================
// STACK WATCHDOG
// =============================================================================
#define STACK_WATCHDOG          128
#define PRIO_WATCHDOG           1       // Baja — solo corre cuando los críticos descansan

// =============================================================================
// POWER MANAGEMENT — PRO-011
// =============================================================================
#define PIN_IGN_SENSE           PA10    // HIGH = +15 presente; LOW = contacto off
#define IGN_OFF_DEBOUNCE_MS     1000UL  // PA10 LOW estable 1s → apagar salidas y entrar en reposo
#define STACK_POWER_MGR         128
#define PRIO_POWER_MGR          2

// =============================================================================
// VERSION
// =============================================================================
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION    "3.3.0"
#endif
#define NODE_NAME           "BCM"

#endif // CONFIG_H
