#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * Protocol.h - Definición global de IDs CAN del sistema Super5 RTOS v3
 *
 * Compartido por todos los nodos. NO modificar sin actualizar todos los nodos.
 *
 * Arquitectura: BCM (STM32F103) | ICM (STM32F103) | UCE (STM32F405) | Gateway (ESP32)
 * Bus CAN: 500 kbps
 */

// =============================================================================
// BCM - Body Control Module (0x100 - 0x1FF)
// =============================================================================
#define CAN_BCM_HEARTBEAT       0x100   // [status(1), uptime_s(4)]          1000ms
#define CAN_BCM_LIGHTS_STATE    0x115   // [flags(1)]                         500ms
#define CAN_BCM_FLAME_CONFIG    0x120   // [pattern(1), speed(1), color(1)]   on-demand
#define CAN_BCM_FLAME_STATE     0x121   // [active(1), pattern(1), frame(1)]  100ms

// Comandos hacia BCM
#define CAN_CMD_BCM_LIGHTS      0x181   // [flags(1)] bitmask luces
#define CAN_CMD_BCM_FLAME       0x182   // [pattern(1), speed(1)]
#define CAN_CMD_BCM_HORN        0x183   // [state(1)] 0=off 1=on

// =============================================================================
// ICM - Instrumentation Control Module (0x310 - 0x31F)
// =============================================================================
#define CAN_ICM_HEARTBEAT       0x310   // [status(1), uptime_s(4)]          1000ms
#define CAN_ICM_ENGINE_DATA     0x311   // [rpm(2), map(2), clt(1), iat(1), batt(1)] 50ms
#define CAN_ICM_STATUS          0x312   // [flags(1), errors(1)]             500ms
#define CAN_ICM_ADC_RAW         0x313   // [adc0(2), adc1(2), adc2(2), adc3(2)] debug

// =============================================================================
// UCE - Engine Control Unit (0x400 - 0x41F)
// =============================================================================
#define CAN_UCE_HEARTBEAT       0x400   // [status(1), uptime_s(4)]          1000ms
#define CAN_UCE_ENGINE          0x410   // [rpm(2), map(2), tps(1), lambda(1)] 20ms
#define CAN_UCE_TEMPS           0x411   // [clt(2), iat(2)]                  100ms
#define CAN_UCE_IGNITION        0x412   // [advance(2), dwell(2), knock(1)]  50ms
#define CAN_UCE_INJECTION       0x413   // [pw_us(2), ve(1), corrections(1)] 50ms
#define CAN_UCE_STATUS          0x414   // [flags(1), errors(2)]             500ms

// Comandos hacia UCE
#define CAN_CMD_UCE_TUNE        0x480   // [table(1), idx(1), value(2)]  calibración
#define CAN_CMD_UCE_MODE        0x481   // [mode(1)] 0=normal 1=idle 2=cranking

// =============================================================================
// GATEWAY - (no emite IDs propios, reenvía todos los demás)
// =============================================================================
#define CAN_GW_HEARTBEAT        0x700   // [status(1), clients(1)]           1000ms

// =============================================================================
// BROADCAST
// =============================================================================
#define CAN_BROADCAST_RESET     0x7FF   // [0xFF] → todos los nodos se reinician

// =============================================================================
// FLAGS DE STATUS (byte 0 en HEARTBEAT)
// =============================================================================
#define STATUS_OK               0x01
#define STATUS_ERROR            0x02
#define STATUS_INIT             0x04
#define STATUS_CALIBRATING      0x08

// =============================================================================
// FLAGS DE LUCES BCM (byte 0 en LIGHTS_STATE)
// =============================================================================
#define LIGHT_POSITION          (1 << 0)
#define LIGHT_LOW_BEAM          (1 << 1)   // BCM: habilita circuito delantero (cruce/largas via palanca)
#define LIGHT_FOG_FRONT         (1 << 2)
#define LIGHT_FOG_REAR          (1 << 3)

#endif // PROTOCOL_H
