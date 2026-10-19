# Protocolo CAN - Super5 RTOS v3

Bus CAN a **500 kbps**, tramas estándar 11 bits.

## Asignación de IDs

| Nodo    | Rango       |
|---------|-------------|
| BCM     | 0x100–0x1FF |
| ICM     | 0x310–0x31F |
| UCE     | 0x400–0x41F |
| Gateway | 0x700       |
| Broadcast | 0x7FF    |

---

## BCM (Body Control Module)

### 0x100 — HEARTBEAT (1000 ms)
| Byte | Contenido        |
|------|-----------------|
| 0    | Status flags     |
| 1–4  | Uptime (s, u32) |
| 5–7  | Reservado        |

### 0x110 — ALARM_STATE (500 ms)
| Byte | Contenido                     |
|------|------------------------------|
| 0    | armed (0=no, 1=sí)           |
| 1    | triggered (0=ok, 1=alarma)   |

### 0x115 — LIGHTS_STATE (500 ms)
| Byte | Contenido                     |
|------|------------------------------|
| 0    | bitmask luces activas        |

### 0x120 — FLAME_CONFIG (on-demand)
| Byte | Contenido                  |
|------|---------------------------|
| 0    | pattern (0–12)            |
| 1    | speed (0–255)             |
| 2    | color (si aplica)         |

### 0x121 — FLAME_STATE (100 ms)
| Byte | Contenido         |
|------|------------------|
| 0    | active (0/1)     |
| 1    | pattern actual   |
| 2    | frame actual     |

### Comandos hacia BCM
| ID    | Función        | data[0]                    |
|-------|---------------|----------------------------|
| 0x180 | CMD_ALARM     | 0=off, 1=arm, 2=trigger   |
| 0x181 | CMD_LIGHTS    | bitmask luces              |
| 0x182 | CMD_FLAME     | [0]=pattern, [1]=speed     |
| 0x183 | CMD_HORN      | 0=off, 1=on                |

---

## Modos Especiales (BCM)

### Fail-Safe CAN (PRO-002)
Si el bus CAN no recibe mensajes por más de **5 segundos**:
1. El BCM activa automáticamente las **luces de posición**.
2. El LED de alarma parpadea a **5 Hz**.
3. El sistema de Flames se detiene y bloquea.
4. Tras **5 minutos** en este estado, las luces se apagan para proteger la batería.
5. El sistema vuelve a la normalidad automáticamente al detectar actividad en el bus.

### Watchdog (PRO-001)
El IWDG está configurado a **1 segundo**. Se reinicia (kick) solo si las tareas de `CAN_RX` y `ALARM` están operativas. Si alguna se bloquea, el nodo se reinicia físicamente.

---

## ICM (Instrumentation Control Module)

### 0x310 — HEARTBEAT (1000 ms)
Igual estructura que BCM.

### 0x311 — ENGINE_DATA (50 ms)
| Byte | Contenido                          |
|------|------------------------------------|
| 0–1  | RPM (u16, 0–9000)                 |
| 2–3  | MAP en mbar (u16)                 |
| 4    | CLT en °C (i8, -40..+150)        |
| 5    | IAT en °C (i8, -40..+125)        |
| 6    | Batería en deciVoltios (u8)       |
| 7    | Reservado                          |

### 0x312 — STATUS (500 ms)
| Byte | Contenido            |
|------|---------------------|
| 0    | flags estado        |
| 1    | bitmask errores     |

---

## UCE (Engine Control Unit)

### 0x400 — HEARTBEAT (1000 ms)

### 0x410 — ENGINE (20 ms)
| Byte | Contenido                    |
|------|------------------------------|
| 0–1  | RPM (u16)                   |
| 2–3  | MAP en mbar (u16)           |
| 4    | TPS % (u8, 0–100)           |
| 5    | Lambda × 10 (u8)            |

### 0x411 — TEMPS (100 ms)
| Byte | Contenido                      |
|------|-------------------------------|
| 0–1  | CLT °C × 10 (i16)            |
| 2–3  | IAT °C × 10 (i16)            |

### 0x412 — IGNITION (50 ms)
| Byte | Contenido                        |
|------|----------------------------------|
| 0–1  | Avance × 10 grados BTDC (i16)  |
| 2–3  | Dwell (µs, u16)                 |
| 4    | Knock (0–255)                   |

### 0x413 — INJECTION (50 ms)
| Byte | Contenido               |
|------|------------------------|
| 0–1  | PW inyección µs (u16) |
| 2    | VE % (u8)             |
| 3    | Correcciones bitmask  |

---

## Broadcast

### 0x7FF — RESET_ALL
data[0] = 0xFF → todos los nodos ejecutan NVIC_SystemReset()

---

## Frecuencias de transmisión

| Mensaje          | Frecuencia |
|-----------------|------------|
| HEARTBEAT       | 1 Hz       |
| ENGINE_DATA/ICM | 20 Hz      |
| UCE_ENGINE      | 50 Hz      |
| UCE_IGNITION    | 20 Hz      |
| FLAME_STATE     | 10 Hz      |
| WS broadcast    | 10 Hz      |

## Carga de bus estimada

Con todos los mensajes activos (~15 tramas/s × 125 bits/trama):
~1875 bits/s sobre 500kbps → **carga < 0.4%** (muy holgado)
