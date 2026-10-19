# Instalación y despliegue — Super5 RTOS v3

> **Desarrollo detenido.** El vehículo se perdió en un incendio antes de llegar a la Fase 3, así que
> las Fases 2–4 quedaron sin completar. Esta guía se conserva como referencia técnica (compilación,
> wiring, troubleshooting) para quien quiera reutilizar el firmware o la arquitectura en otro vehículo.

**Vehículo:** RENAULT-5 GTL 5P · G-C1J · 1989

---

## Índice

1. [Compilar y verificar](#1-compilar-y-verificar)
2. [Fases de despliegue](#2-fases-de-despliegue)
3. [Instalación física por nodo](#3-instalación-física-por-nodo)
4. [Sistema de flames](#4-sistema-de-flames)
5. [Flujo de datos extremo a extremo](#5-flujo-de-datos-extremo-a-extremo)
6. [Herramientas útiles](#6-herramientas-útiles)
7. [Verificación y resolución de problemas](#7-verificación-y-resolución-de-problemas)
8. [Preguntas frecuentes](#8-preguntas-frecuentes)

---

## 1. Compilar y verificar

Antes de tocar hardware, verifica que todo compila:

```bash
cd BCM     && pio run   # ✅ debe terminar con SUCCESS
cd ../ICM  && pio run   # ✅ debe terminar con SUCCESS
cd ../UCE  && pio run   # ⚠️ no compila — ver TODOs UCE-1..4 en `docs/CHANGELOG.md`
cd ../Gateway && pio run && pio run --target buildfs
```

Para flashear cada nodo:

```bash
cd BCM && pio run --target upload       # ST-Link requerido
cd ../ICM && pio run --target upload
cd ../UCE && pio run --target upload
cd ../Gateway
pio run --target upload                  # Firmware ESP32
pio run --target uploadfs                # SPIFFS: dashboard web
```

---

## 2. Fases de despliegue

El orden correcto es de menos a más riesgo. **Nunca saltar fases.**

### Fase 1 — Validación del bus CAN en banco

**Estado:** ✅ Completada **18/05/2026** — BCM + ICM + Gateway, 3 nodos en bus CAN físico a 500 kbps

**Objetivo:** confirmar que los nodos se comunican correctamente antes de tocar el coche.

**Lecciones aprendidas (no repetir):**
- **TJA1050 requiere 5V obligatoriamente** — VCC al pin VIN/5V del Blue Pill, nunca a 3V3. Con 3.3V el transceptor no transmite nada y no da ningún error visible.
- **ST-Link: SWCLK y SWDIO no son intercambiables** — si da `init mode failed`, verificar que los cables no están cruzados antes de intentar el truco del RESET.
- **Truco RESET para ST-Link:** si sigue dando `init mode failed`, mantener pulsado el botón RESET del Blue Pill durante el upload y soltar al aparecer `Connecting...`.
- **Conectores vampiro:** fiables solo si muerden bien el cable. Un mal contacto hace que el nodo no aparezca en el bus sin ningún error visible.
- **Masa común obligatoria:** todos los nodos deben compartir GND. Si se alimentan desde el mismo ordenador por USB, ya es común. Con fuentes distintas, añadir cable GND entre nodos.

**Criterio de éxito:** tres nodos intercambian mensajes sin errores durante 5 minutos continuos. ✅

---

### Fase 2 — Prototipo PCB del BCM

**Estado:** 🔴 Detenida sin completar — PCB diseñado en KiCad (Gerbers listos), nunca fabricado

**Objetivo:** construir el nodo BCM sobre PCB real, alimentado a 12V.

**Material:**
- PCBs doble cara 5×7 cm
- Buck converter LM2596 12V→5V (en casa)
- Condensadores electrolíticos 100µF + 10µF (pedidos)
- Condensadores cerámicos 100nF desacoplo (pedidos)
- Relés SRD-05VDC-SL-C × 10 (pedidos)
- Transistores BC547 (pedidos)
- Diodos Schottky rueda libre (pedidos)
- Optoacopladores PC817 (pedidos)

**Pasos:**
1. Diseñar el esquema: entradas coche → PC817 → GPIO STM32 → BC547 → relé → carga.
2. Montar regulador buck + filtrado. Verificar 5V estables bajo carga con polímetro.
3. Soldar STM32 + circuito de relés.
4. Alimentar desde fuente de laboratorio a 12V simulando batería del coche.
5. Repetir validación CAN de Fase 1 pero con el PCB real.
6. Probar cada salida de relé en vacío.

**Criterio de éxito:** BCM arranca, aparece en el bus CAN, activa/desactiva relés por comando CAN, alimentado a 12V.

---

### Fase 3 — Interceptación en el vehículo

**Estado:** 🔴 Detenida sin completar — el vehículo se perdió antes de llegar a esta fase

**Objetivo:** instalar BCM físicamente en el coche e interceptar los circuitos del panel de relés.

**Acceso físico (Sec 87 del manual):**
- Quitar la guantera + 2 tornillos → panel de relés detrás del tablero
- 12 posiciones (5–12): intermitentes, luces posición, antiniebla, bocina, luces cruce, carentera

**Pasos:**
1. Fotografiar el arnés original antes de tocar nada.
2. Identificar el conector del relé a interceptar (empezar por luces de posición).
3. Extraer el relé original → insertar BCM en serie.
4. **Modo transparente:** si el BCM no está alimentado, el circuito original debe seguir funcionando.
5. Verificar cada circuito interceptado con el motor apagado.
6. Arrancar y comprobar ausencia de errores CAN con el alternador en marcha.
7. Añadir filtrado si aparecen glitches (ferrita en alimentación BCM).

**Conexión desmontable:** usar Faston hembra 6.3mm + conector de 3 pines (GND / +12V / señal) para que sea completamente desmontable y el coche quede stock al desconectar.

**Criterio de éxito:** el coche funciona igual que antes en modo transparente; los comandos CAN activan los circuitos correctamente.

---

### Fase 4 — Gateway + UCE

**Estado:** 🔴 Detenida sin completar — UCE nunca llegó a compilar

**Gateway:**
1. ✅ Driver MCP2515 completo implementado en `Gateway/src/CAN.cpp`.
2. Añadir OTA (Over-The-Air) para actualizar firmware sin desmontar.
3. Pruebas en ruta: estabilidad del bus a distintas temperaturas con motor en marcha.

**UCE** (solo cuando todo lo anterior funciona — toca el motor):
- La UCE tiene errores de compilación pendientes (congelada; ver `docs/CHANGELOG.md`).
- Nunca conectar la UCE al motor sin verificar el timing con osciloscopio primero.
- Probar primero con LED en salidas de ignición antes de conectar bobinas reales.

---

## 3. Instalación física por nodo

### BCM — Body Control Module

**Ubicación:** bajo el salpicadero, junto al panel de relés (Sec 87).

| Pin | Señal | Conexión coche |
|-----|-------|----------------|
| PB8/PB9 | CAN RX/TX | Bus CAN via TJA1050 |
| PA0 | Relé alarma/sirena | Bocina original |
| PA3 | Relé bocina | Panel relés pos. 9 |
| PB0–PB5 | Relés luces | Panel relés pos. 6/8/11/12 |
| PA8 | Flames PWM | Módulo pirotécnico externo |

Circuito driver por cada relé:
```
GPIO 3.3V → R 1kΩ → Base BC547
                     Colector → Bobina SRD-05VDC → 5V
                     Diodo Schottky en paralelo (rueda libre)
                     Emisor → GND
```

Flashear: `cd BCM && pio run --target upload`
Verificar: LED PC13 parpadeando a 1Hz → scheduler FreeRTOS corriendo.

---

### ICM — Instrumentation Control Module

**Ubicación:** junto al cuadro de instrumentos.

| Pin | Señal | Sensor |
|-----|-------|--------|
| PA0 | RPM | Borne (−) bobina via PC817 |
| PA1 | MAP | MPX4250AP → toma vacío múltiple admisión |
| PA2 | CLT | NTC 10K → culata (rosca M14) |
| PA3 | IAT | NTC 10K → caja filtro de aire |
| PB6/PB7 | I2C SDA/SCL | INA219 (tensión batería, dirección 0x40) |

**Calibración RPM:** `RPM_PULSES_PER_REV = 2` (4 cilindros / 2 = 2 pulsos/vuelta en el distribuidor G-C1J).

**Calibración MAP:**
- Motor parado, llave en contacto: MAP ≈ 1000 mbar (presión atmosférica)
- Motor al ralentí: MAP ≈ 350–450 mbar

**Nota:** el ICM solo se puede calibrar con el motor y sensores físicos conectados. No hay datos útiles sin ellos.

---

### UCE — Engine Control Unit

**Ubicación:** compartimento motor, zona protegida de calor.

⚠️ **El G-C1J lleva carburador.** La UCE gestiona encendido (IGN_1–4) y ralentí (válvula IAC). Los canales de inyección (INJ_1–4) están reservados para futura conversión EFI — no conectar por ahora.

⚠️ **No flashear con el motor en marcha.**

Pasos de instalación (Fase 4+):
1. Instalar rueda fónica 36-1 en cigüeñal + sensor inductivo → PA0.
2. Calibrar TDC: poner pistón cil. 1 en TDC, contar dientes hasta el gap → actualizar `TDC_CYL1_TOOTH` en Config.h.
3. Verificar timing con osciloscopio antes de arrancar.
4. Conectar TunerStudio via USB al STM32F405 → abrir `UCE/Super5_UCE.ini`.

---

### Gateway — WiFi

**Ubicación:** salpicadero, con antena WiFi accesible.

1. Verificar cristal del módulo MCP2515 (8MHz → `MCP_8MHZ` en CAN.cpp).
2. Flashear firmware + filesystem:
   ```bash
   cd Gateway
   pio run --target upload
   pio run --target uploadfs
   ```
3. Conectar al WiFi definido en `WIFI_SSID`/`WIFI_PASSWORD` (`Gateway/src/Config.h`).
4. Abrir `http://192.168.4.1` → dashboard.

---

### Bus CAN — Cableado físico

| Señal | Color sugerido |
|-------|---------------|
| CAN_H | Amarillo |
| CAN_L | Verde |
| GND   | Negro |

- Par trenzado para CAN_H y CAN_L.
- **120 Ω en cada extremo del bus** (2 resistencias en total). Sin terminación → tramas corruptas intermitentes a 500 kbps.
- Longitud máxima ~40m a 500 kbps — suficiente en el habitáculo.

---

## 4. Sistema de flames

### Cómo funciona

Cada patrón es una tabla de frames. Cada frame tiene dos valores: `duty` (intensidad 0–255) y `duration` (ms). La tarea `Task_Flames` se ejecuta cada 33ms (30fps) y avanza al siguiente frame aplicando PWM en PA8.

El parámetro `speed` escala la duración de cada frame. A speed=255 → mínima duración. A speed=50 → 5× más lento.

### Módulo pirotécnico externo

La salida PA8 controla el módulo externo. Opción recomendada: **bobina de motillo + bujía NGK pequeña**, montada a 5–15cm de la salida del escape. Circuito:

```
PA8 → BC547 → bobina de motillo → bujía → chispa
              Diodo Schottky en paralelo (kickback)
```

Conexión **desmontable** mediante conector de 3 pines (GND / +12V / señal PA8). Desconectar = coche stock.

### Activación automática en corte de gas

Implementado en v3.1.0 (PRO-009.4). `Task_Sensors` del ICM detecta la condición y la envía al BCM:
```
ICM: rpm > 3500 && map < 300 mbar → CAN_CMD_BCM_FLAME patrón 12 (Rapid fire)
BCM: onCANReceived → Flames_Start(12, 200)
```

### Añadir un patrón nuevo

En `BCM/src/Flames.cpp`, añadir una entrada en `g_patterns[]`:

```cpp
// 13 - Mi patrón
{ "Mi patron", 5, {
    {255, 30}, {100, 20}, {255, 30}, {50, 15}, {0, 200}
}},
```

Actualizar `FLAMES_NUM_PATTERNS 14` en `BCM/src/Flames.h` y añadir la opción en el `<select>` de `Gateway/data/index.html`.

### Flujo completo del comando

```
Usuario pulsa "🔥 DISPARAR" en el navegador
  → app.js: POST /api/v1/command {target:"bcm", cmd:"flame", pattern:3, speed:200}
  → Web.cpp handleCommand() → CAN_SendCommand(0x182, [3, 200], 2)
  → MCP2515 → bus CAN → BCM Task_CAN_RX → Flames_Start(3, 200)
  → Task_Flames ejecuta frames en PA8
```
Latencia total: < 200ms.

---

## 5. Flujo de datos extremo a extremo

```
Sensor físico (RPM en el distribuidor)
    │
    ▼
ISR_RPM en ICM (STM32F103) — calcula período → RPM
    │
    ▼
Task_CAN_TX (ICM, cada 50ms) — empaqueta MSG_ICM_EngineData_t
    │
    ▼
Bus CAN físico (500 kbps)
    │
    ▼
MCP2515 en Gateway (ESP32) detecta trama
    │
    ▼
CAN_Task_RX (Core 0 del ESP32) — desempaqueta → g_eng.rpm
    │
    ▼
Task_WebSocket (Core 1) cada 100ms — JSON: {"rpm":2500,...}
    │
    ▼
WebSocket broadcast → cliente WiFi → app.js → DOM
    │
    ▼
Usuario ve "2500" en el gauge
```

**Latencia total worst-case: ~160ms** — perfectamente aceptable para un dashboard.

### Decodificar una trama manualmente

Si el analizador muestra `311 [7] C4 09 11 05 54 1C 85`:

```
MSG_ICM_EngineData_t (Protocol.h ID 0x311):
  rpm      = 0x09C4 = 2500 RPM    (bytes 0-1, little-endian)
  map_mbar = 0x0511 = 1297 mbar   (bytes 2-3)
  clt_c    = 0x54   = 84°C        (byte 4, signed)
  iat_c    = 0x1C   = 28°C        (byte 5)
  batt_dv  = 0x85   = 133 → 13.3V (byte 6)
```

---

## 6. Herramientas útiles

### Monitor serie múltiple

```bash
# Abrir en terminales separadas
pio device monitor --port /dev/ttyUSB0 --baud 115200   # BCM
pio device monitor --port /dev/ttyUSB1 --baud 115200   # ICM
pio device monitor --port /dev/ttyACM0 --baud 115200   # UCE
```

### Analizador CAN barato

Módulo USB-CAN basado en CH341 (~10€) + `candump` (Linux) o SavvyCAN:

```bash
candump can0
# Ejemplo: can0  311   [7]  C4 09 11 05 54 1C 85
```

### Simular datos sin STM32

Para probar el dashboard sin hardware, añadir en `Gateway/src/main.cpp`:

```cpp
static uint32_t last_sim = 0;
if (millis() - last_sim > 50) {
    last_sim = millis();
    MSG_ICM_EngineData_t fake = {};
    fake.rpm      = 1500 + random(-100, 100);
    fake.map_mbar = 4500;
    fake.clt_c    = 85;
    fake.iat_c    = 30;
    fake.batt_dv  = 132;
    // Inyectar en cola interna de recepción
}
```

---

## 7. Verificación y resolución de problemas

### Secuencia de encendido esperada

1. Contacto → todos los LEDs parpadean
2. ~2s → LEDs estables a 1Hz (FreeRTOS corriendo)
3. Gateway disponible en WiFi SUPER5_GTL
4. Dashboard muestra: MAP ~1000 mbar, CLT temperatura ambiente, RPM 0
5. Arranque motor: RPM aparece, MAP baja a ~400 mbar, CLT sube gradualmente
6. Flames desde dashboard → efectos visibles en PA8

### Tabla de síntomas

| Síntoma | Causa probable | Solución |
|---------|---------------|----------|
| LED no parpadea | Firmware no cargado | `pio run --target upload` |
| LED parpadea rápido | CAN init fallido (loopback no pasa) | Verificar que STM32_CAN_Init() retorna OK; revisar STM32_CAN.cpp |
| No hay comunicación CAN | Terminación incorrecta | Verificar 120Ω **entre** CANH y CANL en cada extremo — no en serie con la línea |
| No hay comunicación CAN | TJA1050 sin 5V | VCC del TJA1050 debe ir a VIN/5V, nunca a 3V3 |
| Nodo no aparece en el bus | Mal contacto en conector | Verificar soldadura o conectores vampiro; probar con cable directo |
| ST-Link: `init mode failed` | Cables SWCLK/SWDIO cruzados | Intercambiar los dos cables; si persiste, usar truco del botón RESET |
| RPM siempre 0 | PC817 mal conectado o ISR no enlazada | Verificar PA0 con osciloscopio; confirmar `attachInterrupt` en Sensors_Init() |
| MAP siempre máximo | Circuito abierto en MPX4250AP | Verificar conexión sensor |
| Dashboard no aparece | SPIFFS no subido | `pio run --target uploadfs` |
| TunerStudio no conecta | Puerto incorrecto | Verificar `/dev/ttyACM0` en Linux |
| Bus-off en CAN | Nodo con BTR incorrecto | Verificar que todos usan 500kbps; revisar prescaler en STM32_CAN.cpp |

### Cómo saber si el STM32 está colgado

LED PC13 parpadeando a 1Hz → FreeRTOS corriendo y `Task_Heartbeat` vivo.
LED fijo o apagado → scheduler no arrancó o `configASSERT` disparado por falta de RAM de tareas.

---

## 8. Preguntas frecuentes

**¿Por qué FreeRTOS y no un loop() normal?**
Con un loop normal, si una operación tarda más (ej: leer ADC lentamente), todo lo demás se retrasa. FreeRTOS garantiza que cada tarea se ejecute en su período exacto. Para el timing de ignición, cualquier retraso produce avance incorrecto.

**¿Por qué `vTaskDelayUntil` y no `vTaskDelay`?**
`vTaskDelay(50)` espera 50ms desde que termina la ejecución. Si la tarea tardó 3ms, el período real es 53ms. `vTaskDelayUntil` espera hasta un instante absoluto — el período es siempre exactamente 50ms independientemente de cuánto tarde la tarea.

**¿Por qué `__attribute__((packed))` en los structs?**
Sin `packed`, el compilador añade padding para alinear campos en memoria. Con `packed`, el struct ocupa exactamente los bytes que necesitamos para que `memcpy` hacia/desde el payload CAN funcione correctamente.

**¿El bus CAN necesita los 120Ω obligatoriamente?**
Para distancias cortas (< 1m en banco) puede funcionar sin ellos. En el coche real, los reflejos de señal sin terminación causan tramas corruptas de forma intermitente a 500kbps. Siempre usar terminación en producción.

**¿Puedo cambiar la velocidad del bus CAN?**
Sí, pero hay que cambiarla en los 4 nodos a la vez. El valor está en `CAN_SPEED` en el `Config.h` de cada nodo. También hay que recalcular el BTR register según el clock APB1 del MCU. Un error en el BTR de un solo nodo hace que ese nodo no pueda comunicarse.

**¿Cómo actualizo el firmware estando el coche montado?**
STM32: conectar ST-Link con el coche apagado → `pio run --target upload`. ESP32: se puede añadir OTA en Fase 4 para actualizar por WiFi sin desmontar nada.
