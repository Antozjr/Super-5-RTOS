# BOM — Bill of Materials

## Super5\_RTOS\_v3 · RENAULT-5 GTL 5P 1989

## Componentes — pedido realizado

> 🛒 **Pedido confirmado** · ~127 € (AliExpress, tras descuento) · Mayo 2026

| \# | Componente | Descripción | Cant. | Fase | Estado |
| - | - | - | - | - | - |
| 1 | Cable silicona flexible | 22AWG, kit 5 colores × 5m | 2 kit | 2–3 | ✅ Disponible |
| 2 | PCB doble cara prototipo | 2×8, 3×7, 4×6, 5×7 cm | 2 pack | 2 | ✅ Dispobible |
| 3 | Optoacoplador PC817 | DIP-4, 10 uds | 10 uds | 2–3 | ✅ Disponible |
| 4 | LEDs 5mm surtidos | 200 uds, varios colores | 200 uds | 2–4 | ✅ Disponible |
| 5 | Condensadores cerámicos | 100 pF–100 nF, 50V, 100 uds | 100 uds | 2 | ✅ Disponible |
| 6 | Pin headers 2.54mm | 1×40P, 2×40P, surtido 3 pitches | 1 kit | 2–3 | ✅ Disponible |
| 7 | Módulo MCP2515 + TJA1050 | CAN bus SPI, cristal 8 MHz integrado | 3 uds | 1 | ✅ Disponible |
| 8 | ESP32 DevKit (Tipo-C) | Dual-core 240 MHz, WiFi+BT | 2 uds | 1–4 | ✅ Disponible |
| 9 | CP2102 USB→UART | Módulo 3.3V/5V | 1 ud | 1 | ✅ Disponible |
| 10 | STM32F405RGT6 (WeAct) | ARM Cortex-M4 168 MHz | 2 uds | 1–4 | ✅ Disponible |
| 11 | STM32F103C8T6 (Blue Pill) | ARM Cortex-M3 72 MHz, CAN | 2 uds pedido + 1 en casa | 1–3 | ✅ Disponible |
| 12 | INA219 módulo I2C | Sensor tensión/corriente 0–26V | 5 uds | 1–3 | ✅  
Disponible |
| 13 | Kit transistores TO-92 | 10 valores: incluye BC547, BC337, etc. | 1 kit (200 uds) | 2 | ✅ Disponible |
| 14 | Kit diodos Schottky | Surtido switching, ≥30V inversa | 1 kit | 2 | ✅ Pedido |
| 15 | TJA1050 módulo CAN | Transceptor standalone, 5 uds | 5 uds | 1–2 | ✅ Disponible |
| 16 | Kit conectores Faston | 270 uds, 2.8/4.8/6.3mm macho+hembra | 1 kit | 3 | ✅ Disponible |
| 17 | Tubo termorretráctil | 127 pcs, negro, varios diámetros | 1 kit | 2–3 | ✅ Disponible |
| 18 | Relé SRD-05VDC-SL-C | Bobina 5V, contactos 10A | 10 uds | 2–3 | ✅ Disponible |
| 19 | Programador ST-Link V2 (clon) | Para Blue Pill + STM32F405 | 1 ud | 1 | ✅ En casa |


## Sin comprar (proyecto archivado antes de completar el pedido)

| \# | Componente | Referencia | Cant. | Fase | Notas |
| - | - | - | - | - | - |
| 20 | **Buck converter 12V→5V** | Módulo LM2596 ajustable | 4 uds | 2 | ✅ En casa |
| 21 | **Condensadores electrolíticos** | Kit 200 uds, 15 valores, 0.1µF–220µF, 10V–50V | 1 kit | 2 | ✅ Disponible |
| 22 | **Conectores JST XH 2.54mm** | Macho + hembra + terminales, 2–5 pines | — | 2–3 | ✅ Disponible |
| 23 | ~~Caja ABS~~ | — | — | — | Descartado |
| 24 | ~~Ferrita clip EMC~~ | — | — | — | Descartado |


## Descartado / no necesario

| Componente | Motivo |
| - | - |
| Cable UTP Cat5e | Prescindible de momento — improvisar con cable existente en Fase 1 |
| AMS1117 3.3V | Los módulos Blue Pill y ESP32 DevKit llevan regulador propio |
| Cristal 8 MHz | Incluido en el módulo MCP2515 |
| Resistencias 120Ω separadas | Cubiertas por el kit de resistencias surtidas |
| FT232RL | Sustituido por CP2102 (función equivalente) |


## Herramientas de banco

| Herramienta | Notas |
| - | - |
| Fuente de laboratorio 0–15V / 3A | Simular batería coche en Fases 1 y 2 |
| Multímetro | Tensiones y continuidad |
| Osciloscopio (mínimo 2 canales) | Bus CAN, señal RPM, timing encendido |
| Soldador temperatura controlada | Componentes SMD si aplica |
| Analizador lógico (8 ch, 24 MHz) | SPI (MCP2515), I2C (INA219), UART |
| Crimpadoras JST + Faston | Arnés en el coche |


## Coste estimado

| Grupo | Coste |
| - | - |
| AliExpress — carrito principal (mayo 2026) | **127 €** |
| **Total real** | **~130 €** |


> No incluye herramientas ni materiales del vehículo.

