# Pinout consolidado — Super5_RTOS_v3

**Vehículo:** RENAULT-5 GTL 5P · G-C1J · 1989

---

## Nodo BCM — STM32F103C8 (Blue Pill)

Bus CAN remapeado a PB8/PB9 (AFIO).

| GPIO | Función firmware | Señal del coche | Conector coche | Notas |
|------|-----------------|-----------------|----------------|-------|
| PC13 | LED_STATUS | — | — | LED integrado Blue Pill, activo en bajo |
| PB8  | CAN_RX | Bus CAN (via TJA1050) | Par trenzado bus | Remapeado (AFIO) |
| PB9  | CAN_TX | Bus CAN (via TJA1050) | Par trenzado bus | Remapeado (AFIO) |
| PA0  | ALARM_OUT | Relé sirena | — | BC547 + SRD-05VDC + 1N4007 |
| PA1  | ALARM_LED | LED indicador alarma exterior | — | |
| PA2  | ALARM_BTN | Botón armar/desarmar | — | INPUT_PULLUP |
| PA3  | HORN_OUT | Relé bocina | ⚠️ Sin relé de fábrica — señal en palanca (conector int.-bocina vía 1); relé propio del BCM | BC547 + SRD-05VDC + 1N4007 |
| PB0  | LIGHT_POS | Relé luces posición | Panel relés Sec 87, pos. 6 (según versión: relé de potencia o chivato olvido luces — verificar) | BC547 + SRD-05VDC + 1N4007 |
| PB1  | LIGHT_LOW | Relé luces cruce | ⚠️ Sin relé de fábrica — palanca conector iluminación vía 2 (relé pos.11 solo Suecia/Noruega) | BC547 + SRD-05VDC + 1N4007 |
| PB3  | LIGHT_HIGH | Relé luces carretera | ⚠️ Sin relé de fábrica — palanca conector iluminación vía 1 (relé pos.10 solo Suecia/Noruega) | BC547 + SRD-05VDC + 1N4007 |
| PB4  | LIGHT_FOG_F | Relé antiniebla delantero | ⚠️ Sin relé de fábrica en el panel (interruptor directo) | BC547 + SRD-05VDC + 1N4007 |
| PB5  | LIGHT_FOG_R | Relé antiniebla trasero | Panel relés Sec 87, pos. 9 (antiniebla trasera) → conector dorado 7v Sec 81, vía 6 | BC547 + SRD-05VDC + 1N4007 |
| PA8  | FLAME_OUT | Salida flames PWM | — | Timer1 CH1 |
| PA9  | FLAME_TRIGGER | Trigger disparo flames | — | |
| PA10 | IGNITION_SENSE| Detección contacto +15 | Pin 86 zócalo master | EXTI wake-up |
| PB6  | I2C_SDA | EEPROM AT24C32 | — | I2C1 |
| PB7  | I2C_SCL | EEPROM AT24C32 | — | I2C1 |

**Circuito driver por cada salida de relé:**
```
GPIO (3.3V) → R 1kΩ → Base BC547 → Colector → Bobina relé SRD-05VDC → 5V
                                                 Diodo Schottky en paralelo (rueda libre)
                        Emisor → GND
```

**Conectores del coche (Sec 87) — acceso:**
- Panel de relés detrás del tablero: quitar guantera + 2 tornillos
- Conexión al arnés: Faston hembra 6.3mm sobre los terminales del relé original

---

## Nodo ICM — STM32F103C8 (Blue Pill)

| GPIO | Función firmware | Señal del coche | Conector coche | Notas |
|------|-----------------|-----------------|----------------|-------|
| PC13 | LED_STATUS | — | — | Activo en bajo |
| PB8  | CAN_RX | Bus CAN | Par trenzado bus | Remapeado (AFIO) |
| PB9  | CAN_TX | Bus CAN | Par trenzado bus | Remapeado (AFIO) |
| PB6  | I2C_SDA | INA219 (tensión batería) | — | I2C1 — Wire por defecto STM32F103 |
| PB7  | I2C_SCL | INA219 (tensión batería) | — | I2C1 |
| PA0  | RPM_INPUT | Señal borne (−) bobina | Via PC817 optoacoplador | EXTI0, flanco RISING |
| PA1  | ADC_MAP | MPX4250AP presión múltiple | Sensor MAP (vacío 8mm) | ADC1 CH1, 0.25–4.75V |
| PA2  | ADC_CLT | NTC 10K temperatura refrigerante | Sensor CLT rosca M14 bloque | ADC1 CH2 |
| PA3  | ADC_IAT | NTC 10K temperatura aire admisión | Sensor IAT colector admisión | ADC1 CH3 |
| PA4  | — | Libre | — | Antes divisor batería, sustituido por INA219 |

**INA219 — sensor tensión batería:**
- Dirección I2C: `0x40` (pin ADDR a GND)
- Rango: 0–26V bus, resolución 1.6mV
- Calibración: `setCalibration_32V_2A()` cubre 10.5–14.8V batería coche
- Librería PlatformIO: `adafruit/Adafruit INA219 @ ^1.2.3`
- También mide corriente si se añade shunt (opcional, no implementado)

**RPM — G-C1J (distribuidor):**
- 4 cilindros, 4 tiempos → `RPM_PULSES_PER_REV = 2`
- Señal tomada del borne (−) de la bobina, aislada via PC817
- Timeout sin pulso → 0 RPM (`RPM_TIMEOUT_MS = 500`)

**NTC — divisor de referencia:**
- R_pull = 10 kΩ a 3.3V
- Beta = 3950, R0 = 10 kΩ a 25°C
- Fórmula: Steinhart-Hart simplificada (ver `adcToNTC_cdeg()` en Sensors.cpp)

---

## Nodo UCE — STM32F405RG

> ⚠️ **Motor G-C1J = carburador.** La UCE gestiona encendido y ralentí.
> Inyectores (INJ_1–4) y lambda wideband no se usan hasta conversión EFI futura.

| GPIO | Función firmware | Señal del coche | Notas |
|------|-----------------|-----------------|-------|
| PC13 | LED_STATUS | — | |
| PB8  | CAN_RX | Bus CAN | CAN1 AF9 — sin conflicto |
| PB9  | CAN_TX | Bus CAN | CAN1 AF9 — sin conflicto |
| PA0  | CRANK_INPUT | Señal distribuidor / rueda fónica | EXTI0 alta prioridad |
| PA1  | CAM_INPUT | Sensor fase árbol de levas | EXTI1 (si se monta sensor) |
| PC6  | INJ_1 | Inyector cil. 1 | Timer8 CH1 — reservado EFI futuro |
| PC7  | INJ_2 | Inyector cil. 2 | Timer8 CH2 — reservado EFI futuro |
| PC8  | INJ_3 | Inyector cil. 3 | Timer8 CH3 — reservado EFI futuro |
| PC9  | INJ_4 | Inyector cil. 4 | Timer8 CH4 — reservado EFI futuro |
| PB6  | IGN_1 | Bobina cil. 1 | Timer4 CH1 |
| PB7  | IGN_2 | Bobina cil. 2 | Timer4 CH2 |
| PE9  | IGN_3 | Bobina cil. 3 | Timer1 CH1 — remapeado desde PB8 |
| PE10 | IGN_4 | Bobina cil. 4 | Timer1 CH2 — remapeado desde PB9 |
| PA2  | ADC_MAP | MPX4250AP | ADC1 CH2 |
| PA3  | ADC_TPS | Potenciómetro TPS | ADC1 CH3, 0–5V via divisor a 3.3V |
| PA4  | ADC_CLT | NTC 10K refrigerante | ADC1 CH4 |
| PA5  | ADC_IAT | NTC 10K aire | ADC1 CH5 |
| PA6  | ADC_LAMBDA | Sonda lambda banda estrecha | ADC1 CH6, 0–1V |
| PA7  | ADC_BATT | Tensión batería (divisor) | ADC1 CH7 |
| PD0  | FUEL_PUMP | Relé bomba combustible | Salida relé 12V |
| PD1  | IAC | Solenoide ralentí carburador | Válvula paso de aire |

---

## Nodo Gateway — ESP32 DevKit (Tipo-C)

| GPIO | Función firmware | Periférico |
|------|-----------------|-----------|
| 18 | SPI_SCK | MCP2515 SCK |
| 19 | SPI_MISO | MCP2515 SO |
| 23 | SPI_MOSI | MCP2515 SI |
| 5  | MCP2515_CS | MCP2515 CS (activo en bajo) |
| 4  | MCP2515_INT | MCP2515 INT (interrupción RX, FALLING) |
| 2  | LED_STATUS | LED integrado ESP32 |

**Red WiFi (modo AP):**
- SSID/contraseña definidos en `Gateway/src/Config.h` (`WIFI_SSID`/`WIFI_PASSWORD`) — no hardcodear credenciales reales en el repo
- IP: `192.168.4.1` · HTTP: puerto 80 · WebSocket: puerto 81

---

## Bus CAN — Cableado físico

| Señal | Color sugerido | Notas |
|-------|---------------|-------|
| CAN_H | Amarillo | Par trenzado con CAN_L |
| CAN_L | Verde | Par trenzado con CAN_H |
| GND   | Negro | Masa común todos los nodos |

- **Terminación:** 120 Ω en cada extremo del bus (2 resistencias en total)
- **Transceivers:** TJA1050 standalone para BCM, ICM y UCE · TJA1050 integrado en módulo MCP2515 para el ESP32
- **Conectores internos PCB:** JST XH 2.54mm (2 pines: CAN_H + CAN_L, o 3 pines añadiendo GND)
- **Velocidad:** 500 kbps · Longitud máxima ~40m (suficiente dentro del habitáculo)

---

## Conectores del coche — referencia cruzada (manual Sec 83/84/87)

### Panel de relés (Sec 87) — detrás del tablero

> ⚠️ **Corregido 01/08/2026** contra el manual oficial (Sec 87, pág. 575). La tabla
> anterior asignaba mal las posiciones 8/9/10/11/12 y suponía relés inexistentes.
> Mapa real del panel de 12 posiciones:

| Pos. | Función relé original (manual) | ¿Relé de potencia en GTL español? | GPIO BCM |
|------|--------------------------------|-----------------------------------|----------|
| 5  | Cadenciador limpiaparabrisas | Sí — no se interviene | — |
| 6  | Relé luces posición / chivato olvido luces | Según versión — verificar | PB0 (si es relé de potencia) |
| 7  | Temporización condenación puertas (CPE) | No — GTL sin CPE, zócalo vacío | — |
| 8  | Central intermitencia (flasher) | Sí — no se interviene | — |
| 9  | Relé antiniebla trasera | Sí — **interceptable** | PB5 |
| 10 | Relé principal iluminación (Suecia/Noruega) | No — zócalo vacío en España | — |
| 11 | Relé luces de cruce (Suecia/Noruega) | No — zócalo vacío en España | — |
| 12 | Cadenciador limpiaparabrisas | Sí — no se interviene | — |

**Funciones SIN relé de fábrica en este coche** (las gobierna la palanca/interruptor
directamente, no hay zócalo que aprovechar):

- **Bocina** — palanca, conector intermitentes-bocina vía 1
- **Luces de cruce / carretera** — palanca, conector iluminación vías 2 (cortas) y 1 (largas)
- **Antiniebla delantera** — interruptor de salpicadero directo

Para controlarlas, el BCM tendría que interponer sus propios relés cortando el arnés →
descartado por decisión de **no modificar el cableado original** (ver CHANGELOG 01/08/2026).

### Cuadro VEGLIA (Sec 83)

| Conector | Vía | Señal | Nodo |
|----------|-----|-------|------|
| A (9v) | 1 | Testigo dirección | BCM |
| A | 2 | Testigo luces cruce | BCM |
| A | 3 | Testigo pastillas freno | ICM |
| A | 4 | Iluminación cuadro | BCM |
| A | 5 | Luces carretera | BCM |
| A | 6 | Nivel carburante | ICM |
| A | 7 | Presión aceite | ICM |
| A | 8 | Freno de mano / Nivosco | ICM |
| A | 9 | Testigo carga | ICM |
| C (7v) | 1 | +después contacto | — |
| C | 7 | Masa cuadro | — |

### Iluminación trasera (Sec 81)

| Conector | Vía | Señal | GPIO BCM |
|----------|-----|-------|----------|
| Negro 7v (dcho.) | 2 | Stop derecho | — (lectura) |
| Negro 7v (dcho.) | 3 | Posición trasera derecha | PB0 |
| Negro 7v (dcho.) | 4 | Intermitente trasero derecho | — |
| Negro 7v (dcho.) | 6 | Marcha atrás | — |
| Negro 7v (dcho.) | 7 | Masa | GND |
| Dorado 7v (izdo.) | 2 | Stop izquierdo | — (lectura) |
| Dorado 7v (izdo.) | 3 | Posición trasera izquierda | PB0 |
| Dorado 7v (izdo.) | 4 | Intermitente trasero izquierdo | — |
| Dorado 7v (izdo.) | 7 | Masa | GND |

### Plafonnier (Sec 81 — GTL sin TIM, conector B 3 vías)

| Vía | Señal |
|-----|-------|
| 1 | Masa |
| 2 | +antes contacto |
| 3 | Información cierre puerta |

---

## Notas sobre el manual de taller

El PDF `manual-de-taller-oficial-renault-super-5-espaol-pdf_compress.pdf` está en la raíz del proyecto.

**Secciones eléctricas clave (páginas ~541–580 del PDF):**
- Sec 80: Batería + Faros delanteros
- Sec 81: Iluminación trasera + Plafonnier ← datos de conectores usados en PINOUT
- Sec 83: Cuadro VEGLIA (conectores A/B/C) ← datos de conectores usados en PINOUT
- Sec 84: Palancas señalización (limpiaparabrisas, luces, intermitentes, bocina)
- Sec 87: Panel de relés (12 posiciones) ← ubicación del BCM validada aquí
- Sec 88: Cableado (luneta térmica, luces de cruce)

**Ausentes del PDF:** Sec 82 y Sec 86 (probablemente caja de fusibles con amperajes). Para la tabla de fusibles buscar el suplemento eléctrico Renault **NT 8052**.

**Secciones del motor (G-C1J, carburador):**
- Sec 12–14: Carburación, antipolucion, ralentí (SOLEX/ZENITH/WEBER)
- Sec 17: Encendido transistorizado (la parte EFI Bendix/Fenix no aplica a este motor)

**No aplica a este vehículo GTL:**
- Sec 36: Dirección asistida (este GTL lleva dirección mecánica simple)
- Sec 62: Aire acondicionado
- Conector TIM-E (4 vías): solo existe en versiones con CPE — este GTL no lo tiene
