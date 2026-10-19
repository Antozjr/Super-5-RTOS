#ifndef TUNER_H
#define TUNER_H

#include <Arduino.h>
#include <stdint.h>

/**
 * Tuner.h - UCE: Protocolo TunerStudio (TSIO)
 *
 * Implementa el protocolo de comunicación con TunerStudio MS
 * sobre el puerto USB CDC del STM32F405.
 *
 * Comandos principales:
 *   'Q' → Identificación firmware
 *   'S' → Nombre ECU
 *   'A' → Datos en tiempo real (realtime data)
 *   'r' → Leer página de configuración
 *   'w' → Escribir página de configuración
 *   'e' → Echo test
 */

// =============================================================================
// CONSTANTES PROTOCOLO
// =============================================================================

#define TSIO_BAUD           115200
#define TSIO_SIGNATURE      "Super5_UCE v3.0"
#define TSIO_PAGE_COUNT     2       // Páginas de configuración

// Comandos TSIO
#define TSIO_CMD_IDENTIFY   'Q'
#define TSIO_CMD_NAME       'S'
#define TSIO_CMD_REALTIME   'A'
#define TSIO_CMD_READ_PAGE  'r'
#define TSIO_CMD_WRITE_PAGE 'w'
#define TSIO_CMD_ECHO       'e'
#define TSIO_CMD_BURN       'b'     // Guardar en EEPROM/Flash

// =============================================================================
// ESTRUCTURA REALTIME DATA (compatible TunerStudio)
// =============================================================================

typedef struct __attribute__((packed)) {
    uint16_t rpm;           // offset 0
    uint8_t  map_kpa;       // offset 2 (kPa, 0-250)
    uint8_t  tps_pct;       // offset 3
    int8_t   clt_c;         // offset 4 (°C + 40, para rangos negativos)
    int8_t   iat_c;         // offset 5
    uint8_t  batt_dv;       // offset 6 (deciVoltios)
    uint8_t  advance;       // offset 7 (grados + offset 10)
    uint16_t inj_pw;        // offset 8 (µs / 10)
    uint8_t  ve;            // offset 10 (%)
    uint8_t  afr;           // offset 11 (× 10)
    uint8_t  status;        // offset 12 (flags modo/error)
    uint8_t  lambda;        // offset 13 (AFR × 10)
    uint8_t  reserved[2];   // offset 14-15
} TSIO_RealtimeData_t;      // 16 bytes total

// =============================================================================
// API PÚBLICA
// =============================================================================

/** Inicializa el puerto serie TunerStudio. */
void Tuner_Init(void);

/**
 * Procesa un byte recibido del puerto TunerStudio.
 * Llamar desde Task_Tuner cada vez que hay datos disponibles.
 */
void Tuner_ProcessByte(uint8_t byte);

/**
 * Prepara y envía los datos en tiempo real.
 * Llamado cuando TunerStudio envía comando 'A'.
 */
void Tuner_SendRealtimeData(void);

/**
 * Procesa comandos pendientes de TunerStudio.
 * Llamar periódicamente desde Task_Tuner.
 */
void Tuner_Update(void);

#endif // TUNER_H
