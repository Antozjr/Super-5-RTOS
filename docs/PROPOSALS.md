# PROPOSALS — Histórico de propuestas

Registro de todas las propuestas de mejora al proyecto, independientemente de su origen.  
El README.md refleja solo el estado actual aprobado. Este documento conserva el historial completo.

**Formato de estado:** `Pendiente` | `Aprobada` | `Rechazada` | `Implementada`

> **Nota (desarrollo detenido):** las referencias `Gemini/...` y `Claude/...` de este documento apuntan a
> directorios de trabajo de la fase de desarrollo activo que no forman parte de este repositorio.
> Se conservan tal cual por fidelidad histórica.

---

## [PRO-001] Watchdog independiente (IWDG) en BCM e ICM
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada en BCM — pendiente en ICM
- **Resumen:** Configurar IWDG con timeout ~500ms. El kick lo ejecuta una tarea de baja prioridad solo si todas las tareas críticas han actualizado sus flags.
- **Decisión:** Aprobada con matiz de Claude: el kick debe depender de flags de todas las tareas críticas, no de una sola tarea watchdog.
- **Implementación BCM (20/05/2026):** `Task_Watchdog` (prio 1) kika IWDG de 1s solo si `WDG_FLAG_CAN_RX` y `WDG_FLAG_ALARM` están ambos activos. Timeout ajustado a 1s (margen de boot). Pendiente aplicar el mismo patrón al ICM en Fase 4.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §2.1 · `docs/CHANGELOG.md` v1.2.0

---

## [PRO-002] Modo fail-safe ante pérdida de bus CAN
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada en BCM
- **Resumen:** Si no se recibe heartbeat en >5s, el BCM entra en modo seguro.
- **Decisión:** Aprobada con matiz: no forzar luces indefinidamente (riesgo batería). Encender luces de posición máximo N minutos, luego apagar todo salvo LED alarma parpadeando a 5Hz.
- **Implementación BCM (20/05/2026):** `g_last_can_rx_ms` actualizado en cada RX. `Task_Alarm` detecta timeout de 5s → `g_failsafe_active=true` → luces posición ON, flames OFF, LED PA1 5Hz. Tras 5min → luces OFF. Recovery automático cuando CAN vuelve. Comandos CAN ignorados mientras fail-safe activo.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §2.2 · `docs/CHANGELOG.md` v1.2.0

---

## [PRO-003] Filtro de media móvil en lecturas ADC del ICM
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada
- **Resumen:** Implementar moving average en MAP, CLT e IAT para evitar saltos por EMI del alternador y bobina de encendido en el nodo ICM.
- **Implementación (20/05/2026):** `ICM/src/Config.h`: `#define SENSORS_AVG_SAMPLES 8`. `ICM/src/Sensors.cpp`: buffers circulares estáticos `g_map_buffer/clt/iat[8]`, índice `g_avg_index`, helper `get_average()`. `Sensors_ReadAll()` acumula en buffers, calcula medias y las convierte antes de la detección de errores. ICM compila: RAM 11.9%, Flash 56.8%.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §2.3 · `docs/CHANGELOG.md` v3.1.0

---

## [PRO-004] Condensadores 100nF de desacoplo en VCC de STM32 y TJA1050
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada
- **Resumen:** Condensadores cerámicos 100nF lo más cerca posible de los pines VCC.
- **Decisión:** Ya incluido en `Claude/hardware/BCM_wiring.md` (C4). Implementada en la guía de hardware de Fase 2.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §2.3

---

## [PRO-009] Consolidación Final de Arquitectura - Fase 2
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada
- **Resumen:** Conjunto de mejoras críticas para cerrar la Fase 2: Robustez ICM, CAN dinámico y automatización de llamas.
- **Implementación (20/05/2026):**
    1. **IWDG ICM:** `Task_Watchdog` (prio 1, stack 80) en `ICM/src/Tasks.cpp`. Flags `WDG_FLAG_SENSORS` (Task_Sensors) y `WDG_FLAG_CAN_RX` (Task_CAN_RX). `IWatchdog.begin(1000000)` en `setup()` antes de crear tareas.
    2. **RPM ISR:** `attachInterrupt` descomentado en `ICM/src/Sensors.cpp:57`. `Sensors_RPM_ISR` y `Sensors_RPM_Update` operativas. (Resuelve TODO #10 de CLAUDE.md.)
    3. **CAN Dinámico:** `BCM/lib/STM32_CAN/STM32_CAN.cpp` e `ICM/lib/STM32_CAN/STM32_CAN.cpp`: `CAN_PRESCALER` fijo eliminado → `prescaler = CAN_APB1_HZ / (baud_rate × 18)` calculado en `STM32_CAN_Init()`. Resuelve TODO #6.
    4. **Auto-Flames overrun:** En `ICM/src/Tasks.cpp:Task_Sensors`: si `rpm > 3500 && map < 300 mbar` → envía `CAN_CMD_BCM_FLAME` patrón 12 (Rapid fire, índice 0-based). BCM ya procesa este comando en `onCANReceived`.
    5. **Status CAN:** `CAN_SendStatus()` (0x312) integrado en `Task_CAN_TX` cada 10 ciclos (500ms), sin tarea extra para preservar RAM.
- **BCM compila:** RAM 9.8%, Flash 50.9% — **ICM compila:** RAM 11.9%, Flash 56.8%
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §5.0 · `docs/CHANGELOG.md` v3.1.0

---

## [PRO-005] PWM real por hardware en el sistema de Flames (PA8)
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada
- **Resumen:** Migrar PA8 de digitalWrite a Timer1 CH1 PWM para usar el campo `duty` de los patrones de flames.
- **Decisión:** Aprobada. PA8 ya está asignado a Timer1 CH1 en Config.h. Es el TODO #11 de CLAUDE.md. Se implementa en Fase 2 o antes de Fase 3.
- **Implementación (20/05/2026):** `Flames_Init()` llama a `analogWrite(PIN_FLAME_OUT, 0)` — STM32duino configura Timer1 CH1 automáticamente. `Flames_Update()` y `Flames_Stop()` usan `analogWrite` con el duty real del patrón (0–255). Eliminados todos los `TODO` y `digitalWrite` de Flames.cpp.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §3.1 · `CLAUDE.md` TODO #11 · `docs/CHANGELOG.md` v1.2.0

---

## [PRO-006] Binary WebSockets en Gateway (sustituir JSON)
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Pendiente — baja prioridad
- **Resumen:** Usar protocolo binario en lugar de JSON para reducir payload un ~60% y bajar latencia del dashboard.
- **Decisión:** Pendiente. La latencia actual (~160ms) es aceptable. Revisar tras Fase 3 si se detectan problemas reales de rendimiento.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §3.2

---

## [PRO-007] Creación de docs/CHANGELOG.md
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada
- **Resumen:** Centralizar hitos y cambios técnicos en un CHANGELOG siguiendo el estándar Keep a Changelog.
- **Decisión:** Aprobada e implementada en esta misma sesión.
- **Ref:** `Gemini/Propuesta_Mejoras_Tecnicas.md` §4.2

---

## [PRO-010] Gateway Power Management (WiFi Shutdown on Ignition Loss)
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Implementada (20/05/2026)
- **Resumen:** Apagar la interfaz WiFi y entrar en Deep Sleep en el ESP32 (Gateway) 30 segundos después de perder el tráfico CAN (proxy del contacto +15), para evitar drenaje de batería.
- **Comportamiento implementado:**
  - Si no se recibe ningún frame CAN durante `POWER_SLEEP_TIMEOUT_MS` (30s):
    1. `detachInterrupt(GPIO4)` — desactiva ISR para evitar activación espuria.
    2. `CAN_DrainBuffer()` — vacía el buffer del MCP2515 para que INT vuelva a HIGH antes de configurar el wake-up (evita wake-up inmediato si INT ya estaba LOW).
    3. `WiFi.mode(WIFI_OFF)` — apaga la radio (~160 mA libres).
    4. `esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0)` — wake-up por nivel bajo en GPIO4 (MCP2515 INT activo bajo). GPIO4 = RTC_GPIO10 en ESP32, compatible con ext0.
    5. `esp_deep_sleep_start()` — deep sleep.
  - Al recibir una nueva trama CAN (BCM/ICM despiertan al poner el contacto), INT baja → ESP32 resetea y ejecuta `setup()` de nuevo. El mensaje `[PWR] Wake-up desde deep sleep por actividad CAN` se imprime por serie.
- **Implementación (Claude Code, 20/05/2026):**
  - `Gateway/src/Config.h`: `#define POWER_SLEEP_TIMEOUT_MS 30000`. Versión → 3.2.0.
  - `Gateway/src/CAN.h` + `CAN.cpp`: `g_last_can_rx_ms` (uint32_t, atómico en Xtensa LX6), actualizado en cada frame recibido. `CAN_GetLastRxMs()` y `CAN_DrainBuffer()` nuevas funciones.
  - `Gateway/src/main.cpp`: `GW_EnterDeepSleep()` (secuencia completa), `Task_PowerMgr` (Core 1, prio 1, verifica cada 5s), log de wakeup cause en setup().
- **Compilación:** RAM 14.4%, Flash 44.2% ✅
- **Nota:** Funciona en conjunción con PRO-011 (BCM STOP mode). Hasta que PRO-011 esté implementado, BCM/ICM seguirán transmitiendo si se alimentan desde +30, por lo que el sleep del Gateway solo se activará si se desconectan físicamente los nodos STM32.
- **Ref:** `Gemini/GUIA_FABRICACION_BCM.md` · `docs/PINOUT.md` (Gateway GPIO4 = MCP2515_INT)

---

## [PRO-011] BCM Smart Power Management (PA10 Sense & STOP Mode)
- **Fecha:** 20/05/2026
- **Autor:** Gemini CLI
- **Estado:** Pendiente de implementación (Blueprint)
- **Resumen:** Implementar gestión de energía nativa en el BCM aprovechando la nueva conexión a +30 (permanente) y la detección de +15 (contacto) en el pin PA10.
- **Diseño del Comportamiento:**
  - **Detección de Caída de Contacto:**
    1. En `Task_Alarm` (o una nueva tarea de sistema), monitorizar el estado de `PA10` (vía optoacoplador).
    2. Si `PA10 == LOW` durante > 1 segundo:
       - Ejecutar `apply_lights(0x00)` (apagar todo).
       - Detener animaciones de flames.
       - Enviar último heartbeat con flag de "nodo entrando en sueño" (opcional).
       - Entrar en modo de bajo consumo (STM32 **STOP mode**).
  - **Configuración de Wake-up:**
    - Antes de dormir, configurar `PA10` como interrupción externa (**EXTI**) con flanco de subida (`RISING`).
    - El despertar debe ser instantáneo al recibir +12V en el borne de contacto.
  - **Detección de Seguridad:** Si el bus CAN recibe actividad mientras el coche está sin contacto (ej: comando de alarma o ráfagas de bienvenida), el nodo debe ser capaz de despertar, procesar la orden y volver a dormir tras un timeout.
- **Implementación requerida (Claude Code):**
  - Modificar `BCM/src/main.cpp` para configurar el pin `PA10` y la interrupción EXTI.
  - Implementar la lógica de transición a STOP mode usando las librerías de bajo consumo de STM32duino (`LowPower.stop()`).
  - Actualizar `BCM/src/Tasks.cpp` para integrar la lectura de `PA10` en el bucle principal.
- **Ref:** `Gemini/GUIA_FABRICACION_BCM.md` · `docs/PINOUT.md`
