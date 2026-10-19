# Super5 RTOS v3

Sistema de gestión electrónica distribuido para un Renault 5 clásico: 4 nodos FreeRTOS que se hablan por bus CAN, con firmware en PlatformIO y una PCB diseñada en KiCad.

> 🔴 **Proyecto archivado.** El vehículo sufrió un incendio durante una marcha normal, antes de llegar a
> instalar el BCM. El desarrollo se detuvo ahí. Este repositorio queda como referencia técnica y cierre
> del proyecto — ver [`docs/`](docs/) para el contexto completo.

Sistema CAN distribuido de gestión y carrocería para un **Renault 5 GTL 1989** (G-C1J).
4 nodos FreeRTOS · Bus CAN 500 kbps · STM32F103 × 2 + STM32F405 + ESP32.

---

## Estado final

| Fase | Descripción | Estado |
|------|-------------|--------|
| Fase 1 | Validación bus CAN en banco | ✅ Completada |
| Fase 2 | PCB del BCM+Gateway (KiCad) | ✅ Diseño terminado — Gerbers listos, nunca fabricado |
| Fase 3 | Instalación física en el vehículo | 🔴 No iniciada — vehículo perdido |
| Fase 4 | Gateway completo + UCE (motor) | 🔴 No iniciada |

| Nodo | MCU | Estado |
|------|-----|--------|
| BCM | STM32F103C8 | ✅ Firmware v3.2.0 compilado — nunca instalado en el vehículo |
| ICM | STM32F103C8 | ✅ Firmware v3.1.0 compilado — nunca instalado en el vehículo |
| Gateway | ESP32 + MCP2515 | ✅ Firmware v3.2.0 compilado — nunca instalado en el vehículo |
| UCE | STM32F405RG | ❌ Nunca llegó a compilar — congelada |

---

## Qué hay en este repo

- `BCM/`, `ICM/`, `UCE/`, `Gateway/` — firmware por nodo (PlatformIO)
- `shared/` — protocolo y structs CAN comunes a todos los nodos
- `Archivos Placa/`, `Super5.pretty/` — diseño KiCad del PCB BCM+Gateway (Gerbers incluidos)
- `docs/` — protocolo CAN, pinout, hardware, BOM, instalación, e histórico de decisiones

## Colaboración

Desarrollado con **Claude Code** como implementador, dirigido por el usuario. `docs/PROPOSALS.md`
conserva el histórico de propuestas de la fase de desarrollo activo (incluidas las de Gemini CLI,
usado entonces como auditor de arquitectura).

## Licencia

MIT — ver [`LICENSE`](LICENSE).
