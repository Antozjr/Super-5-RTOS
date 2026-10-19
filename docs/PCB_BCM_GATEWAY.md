# PCB BCM + Gateway — Diseño de la placa (Fase 2)

**Nodo:** placa combinada BCM (STM32F103C8) + Gateway (ESP32 + MCP2515)
**Objetivo:** sustituir de forma **reversible** los relés originales del coche (interceptando
zócalos existentes) y ofrecer el puente CAN↔WiFi, en un único PCB fabricado (KiCad → PCBWay).
**Vehículo:** Renault 5 GTL 1989 · G-C1J.

> Decisiones registradas en `docs/CHANGELOG.md` (01/08/2026). Este documento es la fuente de
> verdad del diseño físico de la placa.

---

## 1. Principios de diseño

- **No modificar el arnés original.** Solo se interceptan relés existentes (instalación reversible,
  "modo transparente"). Los canales sin relé de fábrica van en la placa pero sin conectar al coche.
- **Módulos enchufables** sobre zócalos hembra: Blue Pill, ESP32 DevKit y MCP2515 → reemplazo fácil.
- **Un riel de 5V común** (LM2596) alimenta ambos nodos y las bobinas de relé.
- **Un único par CANH/CANL** en placa: dos nodos (STM32 vía TJA1050, ESP32 vía MCP2515) sobre el
  mismo bus, terminación 120 Ω por jumper. El CAN sale por un conector hacia el resto del coche.

---

## 2. Alcance de funciones

| Función | Entrada mando | Relé salida | ¿Relé de fábrica? | Se conecta al coche |
|---------|---------------|-------------|-------------------|---------------------|
| Antiniebla trasera | PB10 | PB5 | Sí (Sec 87 pos. 9) | **Ahora** |
| Luces de posición | PA5 | PB0 | Según versión (pos. 6) | Ahora (verificar zócalo) |
| Contacto +15 (sense) | PA10 | — | — (solo lectura) | Ahora |
| Bocina | PA4 | PA3 | No (palanca) | Diferido |
| Luces cruce/largas | PA6 | PB1 | No (palanca) | Diferido |
| Antiniebla delantera | PA7 | PB4 | No (interruptor) | Diferido |
| Flames | — | PA8/PA9 | — (módulo externo) | Cuando se monte |
| Reserva libre | header | header | — | Futuro por código |

Descartado: alarma/sirena (no existe dispositivo de fábrica que interceptar).

---

## 3. Protección de entrada (12 V)

Orden desde el +12 permanente (+30) robado del zócalo:

1. **Fusible de cuchilla (ATO/mini) 3 A** — portafusibles en placa, lo más cerca posible de la entrada.
2. **Protección de polaridad inversa** — MOSFET de canal P (mínima caída frente a un diodo en serie).
3. **Supresor de transitorios** — TVS tipo SMBJ (standoff ~16–18 V) + condensador de bulk.
4. **LM2596** ajustable → riel **5,00 V**. Condensadores C1 100 µF (in), C2 100 µF + C3 10 µF + C4 100 nF (out).

Potencia de los relés: **cada COM toma su +12 del propio zócalo interceptado** (ya fusiado por el
coche). No se distribuye un +12 común en la placa → se mantiene la protección por circuito del vehículo.

---

## 4. Conectores externos

| # | Conector | Señales | Tipo | Se cablea ahora |
|---|----------|---------|------|-----------------|
| J1 | Alimentación lógica | +12V perm (+30), GND | Faston 6.3mm → fusible 3A en placa | Sí |
| J2 | Bus CAN | CANH, CANL, GND | JST XH 3p + jumper 120 Ω | Sí |
| J3 | Salidas de relé (potencia) | Por relé: COM (+12 zócalo) + NO (→carga); 5 relés + 1 reserva | Faston 6.3mm (×2 por relé) | Antiniebla tras. (y posición) |
| J4 | Entradas de mando (señal) | IN_HORN, IN_POS, IN_DEL, IN_FOG_F, IN_FOG_R (+reserva), IGN_SENSE(+15), GND | Faston 2.8mm / header | Posición, antiniebla tras., ignición |
| J5 | Flames (módulo externo) | FLAME_OUT (PWM), FLAME_TRIGGER, GND | JST XH 3p | Al montar el módulo |
| J6 | Programación STM32 (ST-Link) | 3V3, SWDIO, SWCLK, GND, RST | Header 2.54mm 5p (SWD) | Sí |
| J7 | I2C / expansión | SDA, SCL, 3V3, GND | Header 4p + footprint AT24C32 en placa (DNP) | No (reserva) |
| — | USB ESP32 (Gateway) | Flasheo del ESP32 | USB-C del DevKit, accesible | Sí |

### Zócalos internos (módulos enchufables)

- **Blue Pill** (STM32F103C8): 2× header hembra 20p.
- **ESP32 DevKit**: 2× header hembra 19p (ancho).
- **MCP2515**: header según el módulo. SPI interno: SCK=18, MISO=19, MOSI=23, CS=5, INT=4.

---

## 5. Notas de componentes

- **Driver de relé:** BC337 (Ic 800 mA) en lugar de BC547 (100 mA) — la bobina SRD-05VDC tira ~72 mA;
  el BC337 da margen. Ambos están en el kit. R base 1 kΩ. Diodo Schottky de rueda libre por bobina.
- **Entrada opto (mando):** PC817, R 1 kΩ lado LED, R 10 kΩ pulldown lado STM32.
- **TJA1050:** VCC obligatoriamente a **5 V** (con 3,3 V no transmite).
- **Consumo estimado riel 5V:** ~1 A pico (5 bobinas ~360 mA + Blue Pill + ESP32 WiFi + MCP2515).
  LM2596 (2–3 A) sobrado.

---

## 6. Avisos de diseño abiertos

- **PA9/PA10** (USART1 del Blue Pill) chocan con FLAME_TRIGGER/IGN_SENSE → flasheo y depuración por
  **SWD (ST-Link)**, no por serie en esos pines. Consola serie futura por otro USART si se necesita.
- **Verificar físicamente** qué zócalos del panel (pos. 6 y 9) están poblados en el coche real y qué
  conmuta cada uno antes de cablear J3/J4.

---

## 7. Herramienta y fabricación

- **EDA:** KiCad (proyecto versionable).
- **Fabricación:** PCBWay u otra, 2 capas, exportación de Gerbers.

---

## 8. Estado final del PCB — 02/08/2026

Proyecto KiCad: `Archivos Placa/` · Gerbers: `Archivos Placa/gerbers/`.

- **Placa:** 2 capas, **159,5 × 142,5 mm** (~16 × 14,3 cm). 79 componentes. Plano de masa GND en ambas capas.
- **Ruteo:** Freerouting (100%) reimportado a KiCad. **DRC limpio** (0 errores, 0 sin conectar). 4 agujeros **M3** de fijación.
- **Clases de red (ancho de pista):** Default 0,3 mm · **Power** (+5V/+3V3/VBAT_*) 0,6 mm · **Load** (COM*/NO*) 1,5 mm.

### Cambios de componentes respecto al plan
- **Q1** protección polaridad inversa → **IRF9540N** (P-MOSFET TO-220, pinout G-D-S; símbolo `Transistor_FET:IRF9540N`).
- **D2** TVS → **P6KE18A** axial THT (el SMBJ18A era SMD).
- **F1** → portafusibles **mini-blade** (`Fuse:Fuseholder_Blade_Mini_Keystone_3568`) + fusible **3 A**.

### Montaje de módulos (medidas reales)
| Módulo | Cómo se monta | Dato |
|--------|---------------|------|
| Blue Pill | 2 zócalos hembra 1×20 | filas a **15,24 mm** · cuerpo 52×23 mm (+4 mm pines SWD) |
| ESP32 DevKit (30p) | 2 zócalos hembra 1×15 | filas a **25,4 mm** · cuerpo 51×38 mm · USB-C al borde |
| MCP2515 | zócalo 1×7 (lógica) + **CANH/CANL cableados** a `J_MCPCAN` | cuerpo ~50×40 mm |
| TJA1050 | zócalo 1×4 (VCC-TX-RX-GND) + **CANH/CANL cableados** a `J_TJACAN` | cuerpo 32×20 mm |
| Buck LM2596 | 2 conectores con footprint a medida `Super5.pretty/LM2596_pin_pair` | par a **17,5 mm** · IN↔OUT **40 mm** · cuerpo 44×21 mm |

> El buck se enchufa por sus 4 pines (agujeros holgados de 2 mm para tolerancia). Los CANH/CANL del
> MCP y el TJA se llevan con 2 hilos cortos cada uno a sus pads, evitando alinear la 2ª fila del módulo.

### Presupuesto (PCBWay, 02/08/2026)
- 5 uds, 2 capas, 1,6 mm: **~€39,84** + envío. Envío recomendado **Global Direct ~€6,29** (6–10 días). Total ~€46. Nunca pedido — desarrollo detenido antes de fabricar la placa.

### Sin completar (desarrollo detenido)
- **Revisión CAN (decidido 02/08/2026, nunca aplicada):** pasar CANH/CANL del **MCP2515 y TJA1050** de cableado a **enchufe completo** — ambos módulos sacan el CAN por pines (el MCP por header además de la bornera). Faltaba medir la separación de filas y el offset de los 2 pines de CAN, reposicionar `J_MCPCAN`/`J_TJACAN` como zócalos hembra y reenrutar esas pistas.
- Pedido en PCBWay · recuento de componentes vs BOM · caja/alojamiento — ninguno llegó a hacerse.
