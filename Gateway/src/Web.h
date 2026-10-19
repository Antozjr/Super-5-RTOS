#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

/**
 * Web.h - Gateway: Servidor HTTP REST
 *
 * Endpoints disponibles:
 *   GET  /api/v1/status    → Estado gateway y nodos
 *   GET  /api/v1/engine    → Datos motor (último frame ICM/UCE)
 *   GET  /api/v1/bcm       → Estado BCM
 *   POST /api/v1/command   → Enviar comando CAN (JSON)
 *   GET  /                 → Dashboard (index.html desde SPIFFS)
 */

void Web_Init(void);
void Web_Update(void);   // Llamar periódicamente si no se usa async

#endif // WEB_H
