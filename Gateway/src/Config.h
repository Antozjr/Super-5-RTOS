#ifndef CONFIG_H
#define CONFIG_H

/**
 * Config.h - Gateway (WiFi ↔ CAN Bridge)
 * ESP32 @ 240MHz dual-core
 *
 * Core 0: CAN RX/TX (tiempo real)
 * Core 1: HTTP/WebSocket (servidor web)
 */

// =============================================================================
// HARDWARE PINS (MCP2515 SPI)
// =============================================================================

// MCP2515 CAN Controller via SPI
#define PIN_SPI_SCK         18
#define PIN_SPI_MISO        19
#define PIN_SPI_MOSI        23
#define PIN_MCP2515_CS      5
#define PIN_MCP2515_INT     4   // Interrupción MCP2515

// LED de estado
#define PIN_LED_STATUS      2   // LED integrado ESP32

// =============================================================================
// WIFI
// =============================================================================

#define WIFI_SSID           "CHANGE_ME"
#define WIFI_PASSWORD       "CHANGE_ME_MIN_8_CHARS"   // No usar el valor por defecto — definir localmente antes de flashear
#define WIFI_IP             "192.168.4.1"
#define WIFI_GATEWAY        "192.168.4.1"
#define WIFI_SUBNET         "255.255.255.0"

// =============================================================================
// SERVIDORES
// =============================================================================

#define HTTP_PORT           80
#define WS_PORT             81

// REST API endpoints
#define API_BASE            "/api/v1"
#define API_STATUS          "/api/v1/status"
#define API_ENGINE          "/api/v1/engine"
#define API_BCM             "/api/v1/bcm"
#define API_COMMAND         "/api/v1/command"

// =============================================================================
// CAN
// =============================================================================

#define CAN_BUS_SPEED       500000   // 500 kbps
#define CAN_QUEUE_SIZE      32       // Mensajes en cola RX/TX

// =============================================================================
// WEBSOCKET
// =============================================================================

#define WS_BROADCAST_MS     100      // Intervalo broadcast a clientes (ms)
#define WS_MAX_CLIENTS      4

// =============================================================================
// TIMING TAREAS
// =============================================================================

#define TASK_CAN_RX_MS      5
#define TASK_CAN_TX_MS      20
#define TASK_WS_BCAST_MS    100
#define TASK_HEARTBEAT_MS   1000

// =============================================================================
// POWER MANAGEMENT
// =============================================================================

// Tiempo sin tráfico CAN antes de entrar en deep sleep (ms).
// Con heartbeats a 1 Hz, equivale a 30 latidos perdidos consecutivos.
// Funciona en conjunción con PRO-011 (BCM STOP mode): cuando BCM/ICM
// dejan de transmitir al apagarse el contacto, este timeout se dispara.
#define POWER_SLEEP_TIMEOUT_MS  30000

// =============================================================================
// VERSION
// =============================================================================

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION    "3.2.0"
#endif
#define NODE_NAME           "Gateway"

#endif // CONFIG_H
