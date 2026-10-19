# CHANGELOG — Super5 RTOS v3

Formato: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

---

## [Desarrollo detenido] — 23/09/2026

El vehículo sufrió un incendio durante una marcha normal, antes de llegar a instalar el BCM. El
desarrollo se detiene aquí. Reorganización final del repositorio para dejarlo ordenado:

### Changed
- Flujo de colaboración actualizado: único colaborador Claude Code (antes Gemini CLI + Claude Code).
  `AGENTS.md`, `README.md` y `CLAUDE.md` reflejan el desarrollo detenido.
- `docs/PROPOSALS.md` y `docs/CHANGELOG.md` se conservan sin reescribir como registro histórico.
- `Gateway/src/Config.h`: credenciales WiFi reales sustituidas por placeholders antes de publicar el repo.
- `docs/PINOUT.md`, `docs/INSTALL.md`: referencias a la contraseña WiFi real eliminadas.
- `docs/INSTALL.md`: Fases 2–4 marcadas como detenidas sin completar (en vez de "pendiente").

### Removed
- `docs/INDICE_MANUAL.md` — índice de 1000+ líneas del manual de taller en papel, ya sin utilidad
  fuera del trabajo mecánico sobre el vehículo. El resumen de secciones clave se conserva en `AGENTS.md`.

---

## [Unreleased] — Fase 2 (nunca continuada — ver [Desarrollo detenido] arriba)

### Fixed (docs) — 01/08/2026
- **Corregido el mapa del panel de relés Sec 87** en `docs/PINOUT.md` y `docs/BCM_HARDWARE.md`,
  verificado contra el manual oficial (Sec 87 pág. 575 y Sec 84 pág. 562):
  - Mapa real: 5/12 = cadenciador limpia · 6 = luces posición o chivato olvido luces ·
    7 = temporización CPE (GTL sin CPE) · 8 = central intermitencia (flasher) ·
    9 = **antiniebla trasera** · 10/11 = principal iluminación / cruce **solo Suecia-Noruega**.
  - La doc anterior decía por error: 8 = antiniebla, 9 = bocina, 10 = delanteras, 11/12 = cruce/cadenciador.
  - **Bocina, cruce/largas y antiniebla delantera NO llevan relé de fábrica** en este coche:
    los gobierna la palanca (conector iluminación vías 1/2, conector int.-bocina vía 1) o un
    interruptor directo. Solo antiniebla trasera (pos. 9) y quizá posición (pos. 6) son relés
    interceptables robando el zócalo.

### Decisiones de alcance Fase 2 PCB — 01/08/2026
- **No modificar el arnés original**: solo se interceptan relés existentes (instalación reversible,
  "modo transparente"). Las funciones sin relé de fábrica quedan cableadas en la PCB pero sin conectar
  al coche hasta decidir un método reversible (adaptador inline).
- **Alarma/sirena descartadas** del alcance del BCM (no existe dispositivo de fábrica que interceptar).
- **Placa combinada BCM + Gateway** en un solo PCB: comparten riel de 5V y conector de bus CAN. Dos
  nodos CAN (STM32 vía TJA1050 · ESP32 vía MCP2515) sobre el mismo par CANH/CANL, terminación 120Ω por
  jumper. Módulos enchufables (Blue Pill, ESP32, MCP2515) sobre headers para reemplazo fácil.
- **Canales de reserva**: la placa lleva los 5 relés + 6 optos del firmware, más 1 canal libre extra
  (relé + opto) sacado a header para uso futuro por código.
- **Protección de entrada 12V**: portafusibles de cuchilla (ATO/mini) con fusible **3A** en la placa +
  protección de polaridad inversa (MOSFET canal P) + TVS (SMBJ ~16–18V) + condensador de bulk, antes del LM2596.
- **Potencia de los relés**: cada contacto toma su +12 del zócalo original interceptado (ya fusiado por
  el coche). No se distribuye un +12 común en la placa → se mantiene la protección por circuito del vehículo.
- **Herramienta EDA**: KiCad (proyecto versionable, Gerbers para PCBWay).

### PCB BCM+Gateway diseñado en KiCad — 02/08/2026
- **Placa única BCM + Gateway** terminada: 2 capas, **159,5 × 142,5 mm**, 79 componentes THT + módulos enchufables.
- **Ruteo automático con Freerouting** (100% ruteado) + **plano de masa GND** en ambas capas. **DRC limpio** (0 errores, 0 sin conectar). **Gerbers generados** en `Archivos Placa/gerbers/` (7 capas + taladros).
- **Anchos de pista por clase:** señal 0,3 mm · Power 0,6 mm · Load (relés) 1,5 mm.
- **Componentes clave definidos:** MOSFET protección inversa **IRF9540N** (TO-220) · TVS **P6KE18A** (THT, sustituye al SMBJ18A SMD) · portafusibles **mini-blade** + fusible **3 A**.
- **Módulos como zócalos hembra:** Blue Pill (filas a 15,24 mm) y ESP32 30-pin (filas a 25,4 mm). MCP2515 y TJA1050 se enchufan por su fila lógica y **CANH/CANL van cableados** (2 hilos c/u) a pads `J_MCPCAN`/`J_TJACAN`.
- **Buck LM2596:** no sigue paso estándar → **footprint a medida** `Super5.pretty/LM2596_pin_pair` (paso 17,5 mm, agujeros holgados), IN y OUT separados 40 mm.
- **4 agujeros M3** de fijación. Conectores de potencia con bornera de tornillo 5 mm; CAN/flames JST-XH; entradas de mando JST-XH 8p.
- **Presupuesto PCBWay:** ~€39,84 / 5 uds + envío (Global Direct ~€6,29). Pendiente de pedido.

### Nunca realizado (desarrollo detenido antes de continuar)
- Pedir el PCB a PCBWay
- Recontar componentes vs BOM y comprar faltantes · decidir caja/alojamiento
- Verificar físicamente qué zócalos del panel (pos. 6 y 9) están poblados en el coche real
- Montaje: soldar THT + zócalos, enchufar módulos, cablear buck y CANH/CANL

### Validated (21/05/2026)
- Bus CAN 500 kbps con BCM v3.2.0 + ICM v3.1.0 + Gateway v3.2.0 — comunicación OK

---

## [BCM 3.2.0 / ICM 3.1.0 / Gateway 3.2.0] — 21/05/2026

### Added (BCM)
- PRO-011 `Task_PowerMgr`: monitoriza PA10 (PC817 optoacoplador, +15 contacto)
  - PA10 HIGH → operación normal; PA10 LOW estable 1s → apaga salidas y suspende TX CAN
  - Al volver contacto: reactiva todo y resetea el temporizador de fail-safe
  - El IWDG sigue corriendo vía `Task_Watchdog` (sin STOP mode hardware — LSI no para en STM32F1 STOP)
- `PIN_IGN_SENSE PA10`, `IGN_OFF_DEBOUNCE_MS 1000`, `STACK_POWER_MGR`, `PRIO_POWER_MGR` en `Config.h`

### Fixed
- `platformio.ini` de los tres nodos: `-DFIRMWARE_VERSION` anulaba el `#ifndef` de `Config.h`
  → versiones compiladas reales eran siempre `3.0.0` independientemente de `Config.h`
  → corregido: BCM `3.2.0`, ICM `3.1.0`, Gateway `3.2.0`
- ICM `Tasks.cpp`: auto-flames enviaba patrón 13 (fuera de rango 0–12) → corregido a 12 (Rapid fire)

### Memory (post PRO-011)
- BCM: RAM 9.8%, Flash 51.4%
- ICM: RAM 11.9%, Flash 56.8% (sin cambios)
- Gateway: RAM 14.4%, Flash 44.2% (sin cambios)

---

## [3.2.0] — 20/05/2026

### Added (Gateway)
- Power management deep sleep (PRO-010): `Task_PowerMgr` (Core 1, prio 1) verifica inactividad CAN cada 5s
- `GW_EnterDeepSleep()`: detach ISR → drain buffer MCP2515 → WiFi OFF → `ext0_wakeup(GPIO4, LOW)` → `esp_deep_sleep_start()`
- `CAN_GetLastRxMs()`: getter del timestamp del último frame recibido
- `CAN_DrainBuffer()`: vacía buffer RX del MCP2515 antes de sleep (evita wake-up inmediato si INT ya está LOW)
- Log de wakeup cause en `setup()`: imprime mensaje al despertar desde deep sleep por actividad CAN
- `#define POWER_SLEEP_TIMEOUT_MS 30000` en `Config.h`

---

## [3.1.0] — 20/05/2026

### Added (ICM)
- IWDG 1s con patrón de flags por tarea (PRO-009.1): `Task_Watchdog` (prio 1) kika solo si `Task_Sensors` y `Task_CAN_RX` están vivas
- Media móvil ADC 8 muestras en MAP/CLT/IAT (PRO-003): buffers circulares `g_map/clt/iat_buffer[8]`, helper `get_average()`, conversión sobre valores promediados
- `attachInterrupt` RPM activo (PRO-009.2): `Sensors_RPM_ISR` enlazada en `Sensors_Init()` — resuelve TODO #10
- Auto-Flames en overrun (PRO-009.4): `Task_Sensors` detecta `rpm > 3500 && map < 300 mbar` → envía `CAN_CMD_BCM_FLAME` patrón 13
- `CAN_ICM_STATUS` (0x312) enviado cada 500ms desde `Task_CAN_TX` con contador (PRO-009.5) — sin tarea extra

### Changed (BCM + ICM)
- `STM32_CAN.cpp` (BCM e ICM): prescaler CAN calculado dinámicamente en `STM32_CAN_Init()` según `baud_rate` (PRO-009.3) — resuelve TODO #6
  - Fórmula: `prescaler = 36MHz / (baud_rate × 18tq)` — BS1=13tq, BS2=4tq, punto de muestra 77%
  - `CAN_PRESCALER` constante eliminado; `CAN_APB1_HZ`, `CAN_BS1_TICKS`, `CAN_BS2_TICKS` como nuevos defines

### Fixed (PROPOSALS.md)
- PRO-009 aparecía duplicado — eliminado duplicado, entrada unificada

---

## [3.0.0] — 20/05/2026

### Added (BCM)
- IWDG 1s con patrón de flags por tarea (PRO-001): `Task_Watchdog` kika solo si `Task_CAN_RX` y `Task_Alarm` están vivas
- Modo fail-safe ante pérdida de bus CAN (PRO-002): 5s sin CAN → luces posición ON + LED alarma 5Hz; 5min → apagar luces para no agotar batería; recovery automático al restaurar CAN
- PWM real en Flames vía `analogWrite` en PA8/Timer1 CH1 (PRO-005): los 13 patrones usan duty cycle real en lugar de HIGH/LOW
- `Task_Horn` implementada con cola FreeRTOS (`g_horn_queue`, 4 slots)
- `onCANReceived` completo: alarm state machine, bitmask luces → GPIO, horn → queue, flames bloqueadas en fail-safe
- Botón físico PA2 con debounce 100ms para armar/desarmar alarma manualmente
- LED PA1 parpadea a 5Hz durante alarma triggered o modo fail-safe
- `CAN_CMD_BCM_HORN 0x183` añadido a `shared/Protocol.h`

### Changed (BCM)
- `main.cpp`: IWDG arranca antes de crear tareas; cola bocina creada antes del scheduler
- `Flames.cpp`: eliminados todos los `digitalWrite` y `TODO`; PWM activo desde `Flames_Init()`

---

## [1.1.0] — 20/05/2026

### Added
- `Claude/hardware/BCM_wiring.md` — guía de conexiones detallada para montaje del BCM en Fase 2
- `README.md` — documento de estado compartido entre Claude y Gemini
- `PROPOSALS.md` — histórico de propuestas de mejora con estado y decisión
- `docs/CHANGELOG.md` — este archivo
- Estructura de colaboración IA: `Claude/` y `Gemini/` como directorios independientes por agente

### Changed
- `BCM/hardware/BCM_wiring.md` movido a `Claude/hardware/BCM_wiring.md`

---

## [1.0.0] — 18/05/2026 — Hito 1: Primera comunicación CAN real

### Added
- Bus CAN físico a 500 kbps operativo con 3 nodos (BCM + ICM + Gateway)
- `Gateway/src/CAN.cpp` — driver MCP2515 completo con IRQ + polling fallback 20ms
- `Gateway/src/CAN.cpp` — array `g_rx_callbacks[4]` + `CAN_RegisterRxCallback()` (resuelve TODO #3)
- `ICM/src/Config.h` — flag `INA219_ENABLED` para arrancar sin hardware físico (resuelve TODO #13)
- `shared/Protocol.h`, `shared/Messages.h` — protocolo CAN completo
- Todos los nodos: FreeRTOS operativo, heartbeat 1Hz, LED PC13 parpadeando

### Fixed
- TJA1050 requiere 5V obligatoriamente — documentado en INSTALL.md y BCM_wiring.md
- STM32 bxCAN arranca en modo SLEEP tras reset — añadido `CAN_MCR_SLEEP` clear en STM32_CAN_Init()
- `CAN_BTR_TS1_6` no declarado en CMSIS F1 — sustituido por valores directos
- `IRAM_ATTR` eliminado de ISRs en STM32 (atributo solo válido en ESP32)
- `ADC_RESOLUTION` renombrado a `ADC_COUNTS` (conflicto con Arduino.h)
- `autowp/mcp2515` — lib_deps cambiado a URL directa GitHub (registro PlatformIO sin disponibilidad)
- `CAN_SPEED` renombrado a `CAN_BUS_SPEED` (colisión con enum de mcp2515.h)

### Known issues
- UCE: errores de compilación pendientes (ver CLAUDE.md TODO #8) — congelada hasta Fase 4
- STM32_CAN.cpp: prescaler CAN fijo a 4, pendiente cálculo dinámico (TODO #6)
- BCM Flames: usa digitalWrite, sin PWM real (TODO #11) — aprobado en PRO-005
