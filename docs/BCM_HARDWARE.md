# BCM — Hardware y Fabricación

**Nodo:** BCM · STM32F103C8 (Blue Pill) · Fase 2

---

## 1. Alimentación

```
Batería / fuente 12V
    │
    ├── (+12V permanente) ──► LM2596 IN+
    │                              │
    │                        LM2596 OUT+ ──► 5V RAIL ──┬── Blue Pill VIN
    │                                                    ├── TJA1050   VCC  ← OBLIGATORIO 5V
    │                                                    └── Relay A1(+) × 5
    │
    └── (GND) ──► LM2596 IN-
                  LM2596 OUT- ──► GND RAIL ──┬── Blue Pill GND
                                              ├── TJA1050  GND
                                              └── BC547 Emitter × 5
```

**Condensadores (soldar junto al LM2596):**

| Ref | Valor | Ubicación | Nota |
|-----|-------|-----------|------|
| C1 | 100 µF electrolítico | Entrada LM2596 (IN+/IN-) | + a IN+ |
| C2 | 100 µF electrolítico | Salida LM2596 (OUT+/OUT-) | + a OUT+ |
| C3 | 10 µF electrolítico | Salida LM2596 (OUT+/OUT-) | + a OUT+ |
| C4 | 100 nF cerámico | Salida LM2596 (OUT+/OUT-) | sin polaridad |

> Ajustar el potenciómetro del LM2596 hasta medir **5.00 V** antes de conectar cualquier componente.

---

## 2. Bus CAN — TJA1050

| TJA1050 pin | Conecta a | Nota |
|-------------|-----------|------|
| VCC | 5V RAIL | Nunca a 3V3 |
| GND | GND RAIL | |
| TXD | Blue Pill PB9 | CAN_TX (remapeado vía AFIO) |
| RXD | Blue Pill PB8 | CAN_RX (remapeado vía AFIO) |
| CANH | Bus CAN — hilo amarillo | Par trenzado |
| CANL | Bus CAN — hilo verde | Par trenzado |
| S | GND | Modo high-speed (S=LOW) |

**Terminación:** 120 Ω entre CANH y CANL solo si el BCM es el extremo del bus.

**Conector bus CAN (JST XH 3 pines):**

| Pin | Señal | Color |
|-----|-------|-------|
| 1 | CANH | Amarillo |
| 2 | CANL | Verde |
| 3 | GND | Negro |

---

## 3. Circuito driver de relé — repetir × 5

```
                    +5V RAIL
                       │
                    Relay A1(+)
                   [SRD-05VDC]
                    Relay A2(-) ──────── Colector BC547
                       │                      │
                   Schottky               BC547 Base ── R 1kΩ ── GPIO (3.3V)
               (cátodo→A1, ánodo→A2)     Emisor
                                              │
                                          GND RAIL
```

**Montaje BC547 TO-92** (lado plano hacia ti, izq→der): **C · B · E**

---

## 4. Asignación GPIO → Relé → Coche

### 4a. Salidas (relé)

> ⚠️ **Corregido 01/08/2026** (manual oficial Sec 87 pág. 575 / Sec 84 pág. 562). En el
> panel de relés real solo la **antiniebla trasera (pos. 9)** y quizá **posición (pos. 6)**
> son relés interceptables. Bocina, cruce/largas y antiniebla delantera **no llevan relé
> de fábrica** — la palanca/interruptor los gobierna directo. Ver CHANGELOG 01/08/2026.

| GPIO | Define | Relé | Función | Conector coche |
|------|--------|------|---------|----------------|
| PA3 | `PIN_HORN_OUT` | RL1 | Bocina | ⚠️ Sin relé de fábrica — señal en palanca (int.-bocina vía 1); relé propio del BCM |
| PB0 | `PIN_LIGHT_POS` | RL2 | Luces posición | Sec 87 pos. 6 (según versión: relé o chivato — verificar) — Faston 6.3mm |
| PB1 | `PIN_LIGHT_DEL` | RL3 | Luces delanteras (cruce/largas) | ⚠️ Sin relé de fábrica — palanca conector iluminación vías 1/2 |
| PB4 | `PIN_LIGHT_FOG_F` | RL4 | Antiniebla delantera | ⚠️ Sin relé de fábrica en el panel (interruptor directo) |
| PB5 | `PIN_LIGHT_FOG_R` | RL5 | Antiniebla trasera | Sec 87 pos. 9 (antiniebla trasera) → conector dorado Sec 81, vía 6 |

> Cruce/largas: la palanca gestiona la conmutación dentro del circuito habilitado por RL3.

### 4b. Entradas mando (12V coche → PC817 → STM32)

Mismo circuito que la detección de contacto (Sec. 6): R 1kΩ lado LED, R 10kΩ pulldown lado STM32.

| GPIO | Define | Señal | Origen |
|------|--------|-------|--------|
| PA10 | `PIN_IGN_SENSE` | +15 contacto | Arnés pin 86 |
| PA4 | `PIN_IN_HORN` | Botón bocina stalk | Pin 86 zócalo pos. 9 |
| PA5 | `PIN_IN_LIGHT_POS` | Mando luces posición | Pin 86 zócalo pos. 6 |
| PA6 | `PIN_IN_LIGHT_DEL` | Mando luces delanteras | Pin 86 zócalo pos. 10 |
| PA7 | `PIN_IN_FOG_F` | Interruptor antiniebla del. | Pin 86 zócalo pos. 8 |
| PB10 | `PIN_IN_FOG_R` | Interruptor antiniebla tras. | Cable interruptor |

**Contactos del relé en el arnés del coche:**
```
[+12V permanente coche] ──► COM relé BCM
                             NO  relé BCM ──► Carga (bombilla, bocina…)
```

---

## 5. Estrategia de alimentación — arnés ("el pulpo")

- **+30 (permanente):** Pin 30 del zócalo → LM2596 IN+. El BCM siempre tiene energía.
- **+15 (contacto):** Pin 86 del zócalo → circuito PC817 → PA10 (IGNITION_SENSE, PRO-011).
- **GND:** Pin 85 del zócalo → LM2596 IN- y GND de placa.
- **Bobinas relés:** todos los COM a +12V permanente (Pin 30).

---

## 6. Detección de contacto +15 (PA10) — PRO-011

```
+12V contacto (Pin 86) ──► R 1kΩ ──► Ánodo PC817
                                       Cátodo PC817 ──► GND coche

3.3V STM32 ──► Colector PC817
               Emisor PC817 ──► PA10 (+ R 10kΩ pulldown a GND STM32)
```

- PA10 = HIGH → contacto ON → BCM operativo
- PA10 = LOW  → contacto OFF → BCM entra en STOP mode (PRO-011, blueprint — nunca implementado)

---

## 7. Otras conexiones del Blue Pill

| GPIO | Función | Conecta a |
|------|---------|-----------|
| PC13 | LED estado | Integrado (no soldar nada) |
| PA8 | `PIN_FLAME_OUT` | Módulo pirotécnico (Timer1 CH1 PWM) |
| PA9 | `PIN_FLAME_TRIGGER` | Trigger disparo flames |
| PB6 | I2C SDA | EEPROM AT24C32 (reservado Fase 2+) |
| PB7 | I2C SCL | EEPROM AT24C32 (reservado Fase 2+) |

**GPIO libres (reserva Fase 3+):** PB3, PB11, PB12, PB13, PB14, PB15, PA0, PA1, PA2, PA15, PC14, PC15

---

## 8. Mapa de pines Blue Pill

```
  Left (pin 1 arriba)              Right (pin 1 arriba)
  GND                              GND
  GND                              3V3
  3V3                              RST
  RST                              PB11 ── libre
  PB12 ── libre                    PB10 ── IN_FOG_R   (PC817)
  PB13 ── libre                    PB1  ── LIGHT_DEL  → RL3
  PB14 ── libre                    PB0  ── LIGHT_POS  → RL2
  PB15 ── libre                    PA7  ── IN_FOG_F   (PC817)
  PA8  ── FLAME_OUT (PWM T1CH1)    PA6  ── IN_LIGHT_DEL (PC817)
  PA9  ── FLAME_TRIGGER            PA5  ── IN_LIGHT_POS (PC817)
  PA10 ── IN_IGNITION (PC817)      PA4  ── IN_HORN    (PC817)
  PA11 ── USB DM (no usar)         PA3  ── HORN_OUT   → RL1
  PA12 ── USB DP (no usar)         PA2  ── libre
  PA15 ── libre                    PA1  ── libre
  PB3  ── libre                    PA0  ── libre
  PB4  ── LIGHT_FOG_F → RL4        PC15 ── libre
  PB5  ── LIGHT_FOG_R → RL5        PC14 ── libre
  PB6  ── I2C SDA                  PC13 ── LED STATUS
  PB7  ── I2C SCL                  VCC  → 5V RAIL
  PB8  ── CAN_RX → TJA1050         GND  → GND RAIL
  PB9  ── CAN_TX → TJA1050
```

---

## 9. Conector inter-placas (carrier ↔ relay board, JST XH 7 pines)

| Pin | Señal | Descripción |
|-----|-------|-------------|
| 1 | +5V | Alimentación bobinas relés |
| 2 | GND | Masa común |
| 3 | PA3 | Driver RL1 — Bocina |
| 4 | PB0 | Driver RL2 — Luces posición |
| 5 | PB1 | Driver RL3 — Luces delanteras |
| 6 | PB4 | Driver RL4 — Antiniebla del. |
| 7 | PB5 | Driver RL5 — Antiniebla tras. |

---

## 10. Verificación antes de dar tensión

1. Multímetro en modo diodo: verificar los 5 Schottky (0.2–0.4 V directo; OL inverso).
2. Continuidad: GND Blue Pill ↔ GND BC547 ↔ GND LM2596.
3. Resistencia 5V–GND con todo desconectado: debe ser > 1 kΩ.
4. Ajustar LM2596 → medir 5.00 V → conectar Blue Pill → verificar LED PC13 a 1 Hz.
5. Monitor serie: confirmar `[BCM] Heartbeat` cada segundo.
6. Con Gateway en el bus: verificar trama 0x100 en monitor ESP32.
7. Comando CAN `0x181` con bitmask → escuchar "clic" de cada relé.

---

## 11. Instalación en el vehículo

**Acceso al panel de relés (Sec 87):** quitar guantera + 2 tornillos de ¼ vuelta.

1. Fotografiar el arnés original antes de tocar nada.
2. Extraer relés originales → insertar conectores Faston BCM en sus posiciones.
3. **Modo transparente:** con BCM sin alimentar, el circuito original debe seguir funcionando.
4. Dar contacto → verificar LED PC13 a 1 Hz → probar bocina desde dashboard WiFi.
5. Usar termorretráctil en todos los terminales Faston expuestos.
6. Alojar el BCM en zona fresca del salpicadero, lejos de la calefacción.
