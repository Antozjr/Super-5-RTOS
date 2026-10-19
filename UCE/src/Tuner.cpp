#include "Tuner.h"
#include "Config.h"
#include "Engine.h"
#include "Tables.h"

/**
 * Tuner.cpp - UCE: Implementación protocolo TunerStudio
 *
 * TODO: Implementar lectura/escritura de páginas de configuración.
 * TODO: Implementar quemado en Flash (HAL_FLASH_Program).
 * TODO: Añadir CRC de verificación en escrituras.
 */

// =============================================================================
// ESTADO INTERNO
// =============================================================================

static uint8_t  g_rx_buffer[64];
static uint8_t  g_rx_idx    = 0;
static bool     g_cmd_ready = false;
static uint8_t  g_current_cmd = 0;

// =============================================================================
// INIT
// =============================================================================

void Tuner_Init(void) {
    // TunerStudio usa USB CDC (SerialUSB en STM32F405)
    // Serial.begin() ya se llama en main.cpp
    // TODO: Si se usa SerialUSB separado, inicializarlo aquí

    g_rx_idx    = 0;
    g_cmd_ready = false;
    Serial.println(F("[Tuner] Init - " TSIO_SIGNATURE));
}

// =============================================================================
// RECEPCIÓN BYTE A BYTE
// =============================================================================

void Tuner_ProcessByte(uint8_t byte) {
    // Protocolo TSIO: los comandos son un byte, algunos con parámetros a continuación
    // Implementación simplificada: detectar comando y ejecutar

    if (!g_cmd_ready) {
        g_current_cmd = byte;
        g_rx_idx = 0;

        switch (byte) {
            case TSIO_CMD_IDENTIFY:
            case TSIO_CMD_NAME:
            case TSIO_CMD_REALTIME:
            case TSIO_CMD_ECHO:
            case TSIO_CMD_BURN:
                g_cmd_ready = true;   // Comandos de un solo byte
                break;

            case TSIO_CMD_READ_PAGE:
            case TSIO_CMD_WRITE_PAGE:
                // Estos necesitan más bytes (página, offset, longitud)
                // TODO: Implementar máquina de estados para leer parámetros
                break;

            default:
                // Byte desconocido, ignorar
                break;
        }
    }
}

// =============================================================================
// ENVÍO DATOS TIEMPO REAL
// =============================================================================

void Tuner_SendRealtimeData(void) {
    const EngineData_t* e = Engine_GetData();

    TSIO_RealtimeData_t rt = {};
    rt.rpm      = e->rpm;
    rt.map_kpa  = (uint8_t)(e->map_mbar / 10);
    rt.tps_pct  = e->tps_pct;
    rt.clt_c    = (int8_t)(e->clt_cdeg / 10);
    rt.iat_c    = (int8_t)(e->iat_cdeg / 10);
    rt.batt_dv  = (uint8_t)(e->batt_mv / 100);
    rt.advance  = (uint8_t)(e->advance_deg + 10);   // Offset +10 para valores negativos
    rt.inj_pw   = (uint16_t)(e->inj_pw_us / 10);
    rt.ve       = (uint8_t)e->ve_pct;
    rt.afr      = e->lambda_afr;
    rt.status   = (e->mode & 0x0F) | ((e->error_flags & 0x0F) << 4);
    rt.lambda   = e->lambda_afr;

    // Enviar por Serial (USB CDC)
    Serial.write((uint8_t*)&rt, sizeof(rt));
}

// =============================================================================
// UPDATE
// =============================================================================

void Tuner_Update(void) {
    // Leer bytes disponibles
    while (Serial.available()) {
        Tuner_ProcessByte((uint8_t)Serial.read());
    }

    // Ejecutar comando si está listo
    if (!g_cmd_ready) return;
    g_cmd_ready = false;

    switch (g_current_cmd) {

        case TSIO_CMD_IDENTIFY:
            // Responder con firma del firmware
            Serial.print(TSIO_SIGNATURE);
            break;

        case TSIO_CMD_NAME:
            Serial.print(F("Super5 UCE v3"));
            break;

        case TSIO_CMD_REALTIME:
            Tuner_SendRealtimeData();
            break;

        case TSIO_CMD_ECHO:
            Serial.write(TSIO_CMD_ECHO);
            break;

        case TSIO_CMD_BURN:
            // TODO: Guardar tablas en Flash
            Serial.println(F("[Tuner] BURN - TODO: implementar"));
            break;

        case TSIO_CMD_READ_PAGE:
            // TODO: Enviar página de configuración solicitada
            Serial.println(F("[Tuner] READ_PAGE - TODO: implementar"));
            break;

        case TSIO_CMD_WRITE_PAGE:
            // TODO: Recibir y escribir página de configuración
            Serial.println(F("[Tuner] WRITE_PAGE - TODO: implementar"));
            break;

        default:
            break;
    }
}
