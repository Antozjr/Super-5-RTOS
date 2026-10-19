#include "CAN.h"
#include "Config.h"
#include "../../shared/Protocol.h"
#include "../../shared/Messages.h"
#include <SPI.h>
#include <mcp2515.h>

/**
 * CAN.cpp - Gateway: CAN via MCP2515 (SPI)
 *
 * El ESP32 no tiene bxCAN compatible, así que usamos un MCP2515 externo.
 * El módulo MCP2515 lleva cristal de 8 MHz y transceptor TJA1050 integrado.
 *
 * Pines SPI (Config.h):
 *   SCK  = GPIO18   MISO = GPIO19
 *   MOSI = GPIO23   CS   = GPIO5
 *   INT  = GPIO4    (flanco FALLING → g_mcp_irq = true)
 *
 * Flujo de recepción:
 *   ISR (IRAM) → g_mcp_irq = true
 *   CAN_Task_RX (Core 0) → mcp2515.readMessage() → g_rx_queue → callbacks
 *
 * Flujo de envío:
 *   CAN_Send() → g_tx_queue
 *   CAN_Task_TX (Core 0) → mcp2515.sendMessage()
 */

// =============================================================================
// INSTANCIA MCP2515
// =============================================================================

static MCP2515 g_mcp2515(PIN_MCP2515_CS);

// Flag IRQ: seteado en ISR (IRAM), leído y limpiado en CAN_Task_RX
static volatile bool g_mcp_irq = false;

// Timestamp del último frame recibido (PRO-010).
// Escrito desde Core 0 (CAN_Task_RX), leído desde Core 1 (Task_PowerMgr).
// uint32_t es atómico en Xtensa LX6 — sin mutex necesario.
static uint32_t g_last_can_rx_ms = 0;

// =============================================================================
// ESTADO INTERNO
// =============================================================================

static CAN_RxCallback_t g_rx_callbacks[CAN_MAX_RX_CALLBACKS] = {};
static uint8_t          g_rx_callback_count = 0;

static QueueHandle_t g_rx_queue = nullptr;
static QueueHandle_t g_tx_queue = nullptr;

// =============================================================================
// ISR
// =============================================================================

void IRAM_ATTR MCP2515_ISR(void) {
    g_mcp_irq = true;
}

// =============================================================================
// INIT
// =============================================================================

bool CAN_Init(void) {
    // Colas FreeRTOS
    g_rx_queue = xQueueCreate(CAN_QUEUE_SIZE, sizeof(CAN_Frame_t));
    g_tx_queue = xQueueCreate(CAN_QUEUE_SIZE, sizeof(CAN_Frame_t));

    if (!g_rx_queue || !g_tx_queue) {
        Serial.println(F("[CAN] ERROR: No hay memoria para colas"));
        return false;
    }

    // SPI — velocidad máxima MCP2515: 10 MHz
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_MCP2515_CS);
    delay(100);   // Dar tiempo al MCP2515 para estabilizarse tras el encendido

    // MCP2515 reset (3 intentos con pausa entre ellos)
    MCP2515::ERROR reset_err = MCP2515::ERROR_FAIL;
    for (uint8_t i = 0; i < 3; i++) {
        reset_err = g_mcp2515.reset();
        if (reset_err == MCP2515::ERROR_OK) break;
        Serial.printf("[CAN] MCP2515 reset intento %d fallido\n", i + 1);
        delay(50);
    }
    if (reset_err != MCP2515::ERROR_OK) {
        Serial.println(F("[CAN] ERROR: MCP2515 reset fallido — verificar SPI y CS"));
        return false;
    }

    // Cristal del módulo: 8 MHz → coincide con MCP_8MHZ
    if (g_mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) != MCP2515::ERROR_OK) {
        Serial.println(F("[CAN] ERROR: MCP2515 setBitrate fallido"));
        return false;
    }

    // --- Test loopback interno ---
    if (g_mcp2515.setLoopbackMode() == MCP2515::ERROR_OK) {
        struct can_frame test_tx = {}, test_rx = {};
        test_tx.can_id  = 0x7FE;
        test_tx.can_dlc = 1;
        test_tx.data[0] = 0xAB;
        g_mcp2515.sendMessage(&test_tx);
        delay(5);
        if (g_mcp2515.readMessage(&test_rx) == MCP2515::ERROR_OK
            && test_rx.data[0] == 0xAB) {
            Serial.println(F("[CAN] Loopback OK"));
        } else {
            Serial.println(F("[CAN] ERROR: Loopback fallido — fallo interno MCP2515"));
            return false;
        }
    }

    if (g_mcp2515.setNormalMode() != MCP2515::ERROR_OK) {
        Serial.println(F("[CAN] ERROR: MCP2515 setNormalMode fallido"));
        return false;
    }

    // Interrupción en flanco FALLING (INT activo bajo en MCP2515)
    pinMode(PIN_MCP2515_INT, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_MCP2515_INT), MCP2515_ISR, FALLING);

    // Inicializar timestamp: el timer de inactividad arranca desde el boot
    g_last_can_rx_ms = millis();

    pinMode(PIN_LED_STATUS, OUTPUT);
    Serial.println(F("[CAN] MCP2515 OK — 500 kbps, cristal 8 MHz"));
    return true;
}

// =============================================================================
// ENVÍO
// =============================================================================

bool CAN_Send(const CAN_Frame_t* frame) {
    if (!frame) return false;
    return xQueueSend(g_tx_queue, frame, 0) == pdTRUE;
}

bool CAN_SendCommand(uint32_t id, const uint8_t* data, uint8_t dlc) {
    CAN_Frame_t frame = {};
    frame.id  = id;
    frame.dlc = dlc > 8 ? 8 : dlc;
    memcpy(frame.data, data, frame.dlc);
    return CAN_Send(&frame);
}

// =============================================================================
// RECEPCIÓN
// =============================================================================

bool CAN_Receive(CAN_Frame_t* frame) {
    return xQueueReceive(g_rx_queue, frame, 0) == pdTRUE;
}

void CAN_RegisterRxCallback(CAN_RxCallback_t cb) {
    if (!cb) return;
    if (g_rx_callback_count >= CAN_MAX_RX_CALLBACKS) {
        Serial.println(F("[CAN] WARN: límite de callbacks alcanzado"));
        return;
    }
    g_rx_callbacks[g_rx_callback_count++] = cb;
}

// =============================================================================
// TAREA RX — Core 0
// =============================================================================

void CAN_Task_RX(void* params) {
    (void)params;
    struct can_frame mcp_frame;
    uint32_t last_poll_ms = 0;

    for (;;) {
        // Leer si hay IRQ o cada 20 ms por polling (cubre el caso de INT no conectado)
        bool do_read = g_mcp_irq;
        uint32_t now = millis();
        if ((now - last_poll_ms) >= 20) {
            do_read = true;
            last_poll_ms = now;
        }

        if (do_read) {
            g_mcp_irq = false;

            // Vaciar el buffer de recepción del MCP2515
            while (g_mcp2515.readMessage(&mcp_frame) == MCP2515::ERROR_OK) {
                CAN_Frame_t frame;
                frame.id           = mcp_frame.can_id & CAN_SFF_MASK;  // 11-bit standard frame
                frame.dlc          = mcp_frame.can_dlc;
                frame.timestamp_ms = millis();
                memcpy(frame.data, mcp_frame.data, frame.dlc);

                // Debug RX — quitar cuando el bus esté validado
                Serial.printf("[CAN RX] ID=0x%03lX DLC=%d data=", (unsigned long)frame.id, frame.dlc);
                for (uint8_t i = 0; i < frame.dlc; i++) Serial.printf("%02X ", frame.data[i]);
                Serial.println();

                // Actualizar timestamp de actividad CAN (PRO-010)
                g_last_can_rx_ms = millis();

                // Encolar para la tarea WebSocket
                xQueueSend(g_rx_queue, &frame, 0);

                // Invocar callbacks registrados (WebSocket, etc.)
                for (uint8_t i = 0; i < g_rx_callback_count; i++) {
                    g_rx_callbacks[i](&frame);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_CAN_RX_MS));
    }
}

// =============================================================================
// POWER MANAGEMENT (PRO-010)
// =============================================================================

uint32_t CAN_GetLastRxMs(void) {
    return g_last_can_rx_ms;
}

void CAN_DrainBuffer(void) {
    struct can_frame f;
    while (g_mcp2515.readMessage(&f) == MCP2515::ERROR_OK) { /* descartar */ }
    // Limpiar CANINTF (flags TX, error, wake) para que INT vuelva a HIGH.
    // Sin esto el ESP32 despierta inmediatamente del deep sleep.
    g_mcp2515.clearInterrupts();
    delay(5);
}

// =============================================================================
// TAREA TX — Core 0
// =============================================================================

void CAN_Task_TX(void* params) {
    (void)params;
    CAN_Frame_t frame;
    struct can_frame mcp_frame;

    for (;;) {
        while (xQueueReceive(g_tx_queue, &frame, 0) == pdTRUE) {
            mcp_frame.can_id  = frame.id;
            mcp_frame.can_dlc = frame.dlc;
            memcpy(mcp_frame.data, frame.data, frame.dlc);

            MCP2515::ERROR err = g_mcp2515.sendMessage(&mcp_frame);
            if (err != MCP2515::ERROR_OK) {
                Serial.printf("[CAN TX] ERROR enviando ID=0x%03lX (err=%d)\n",
                              (unsigned long)frame.id, (int)err);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(TASK_CAN_TX_MS));
    }
}
