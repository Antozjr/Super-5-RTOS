/**
 * app.js - Super5 GTL Dashboard
 *
 * Conexión WebSocket al Gateway (192.168.4.1:81).
 * Actualiza gauges en tiempo real y envía comandos CAN.
 */

'use strict';

// =============================================================================
// CONFIG
// =============================================================================

const GW_HOST = window.location.hostname || '192.168.4.1';
const WS_URL  = `ws://${GW_HOST}:81`;
const API_URL = `http://${GW_HOST}`;
const WS_RECONNECT_MS = 3000;

// =============================================================================
// ESTADO
// =============================================================================

let ws        = null;
let connected = false;
let reconnectTimer = null;

// =============================================================================
// LOG
// =============================================================================

function log(msg, type = '') {
    const el  = document.getElementById('log');
    const div = document.createElement('div');
    div.className = 'log-line' + (type ? ` log-${type}` : '');
    div.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
    el.prepend(div);
    // Limitar a 50 líneas
    while (el.children.length > 50) el.removeChild(el.lastChild);
}

// =============================================================================
// WEBSOCKET
// =============================================================================

function connect() {
    log(`Conectando a ${WS_URL}...`);
    ws = new WebSocket(WS_URL);

    ws.onopen = () => {
        connected = true;
        setStatus(true);
        log('WebSocket conectado', 'ok');
        clearTimeout(reconnectTimer);
    };

    ws.onclose = () => {
        connected = false;
        setStatus(false);
        log('WebSocket desconectado', 'err');
        reconnectTimer = setTimeout(connect, WS_RECONNECT_MS);
    };

    ws.onerror = (e) => {
        log('Error WS: ' + e.type, 'err');
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            updateGauges(data);
        } catch (e) {
            log('JSON inválido: ' + event.data, 'err');
        }
    };
}

// =============================================================================
// UI - ACTUALIZAR GAUGES
// =============================================================================

function setStatus(ok) {
    const el = document.getElementById('connection-status');
    el.className = 'status-badge ' + (ok ? 'connected' : 'disconnected');
    el.textContent = ok ? 'Conectado' : 'Desconectado';
}

function setGauge(id, value, unit = '') {
    const el = document.getElementById(id);
    if (el) el.textContent = value !== null ? `${value}${unit}` : '—';
}

function setBar(id, pct) {
    const el = document.getElementById(id);
    if (el) el.style.width = Math.min(100, Math.max(0, pct)) + '%';
}

function updateGauges(d) {
    setGauge('rpm',  d.rpm  ?? '—');
    setGauge('map',  d.map  !== undefined ? Math.round(d.map / 10) : '—');  // mbar → kPa
    setGauge('clt',  d.clt  ?? '—');
    setGauge('iat',  d.iat  ?? '—');
    setGauge('batt', d.batt !== undefined ? d.batt.toFixed(1) : '—');

    // Flames
    if (d.flame !== undefined) {
        document.getElementById('flame-status').textContent =
            d.flame ? `🔥 #${d.pattern}` : 'Off';
    }

    // Barra RPM (0-8000)
    if (d.rpm !== undefined) setBar('rpm-bar', d.rpm / 80);

    // Alertas visuales
    if (d.clt !== undefined && d.clt > 105) {
        document.querySelector('.gauge-card:nth-child(3)').style.borderColor = 'var(--red)';
    }
    if (d.batt !== undefined && d.batt < 11.5) {
        document.querySelector('.gauge-card:nth-child(5)').style.borderColor = 'var(--red)';
    }
}

// =============================================================================
// COMANDOS → API REST
// =============================================================================

async function sendCommand(payload) {
    try {
        const res = await fetch(`${API_URL}/api/v1/command`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        const data = await res.json();
        if (data.ok) {
            log(`CMD OK: ${JSON.stringify(payload)}`, 'ok');
        } else {
            log(`CMD ERR: ${data.error}`, 'err');
        }
    } catch (e) {
        log('Error enviando comando: ' + e.message, 'err');
    }
}

// =============================================================================
// EVENTOS UI
// =============================================================================

document.addEventListener('DOMContentLoaded', () => {

    // Slider velocidad flames
    const speedSlider = document.getElementById('flame-speed');
    const speedVal    = document.getElementById('flame-speed-val');
    speedSlider.addEventListener('input', () => {
        speedVal.textContent = speedSlider.value;
    });

    // Botón disparar flames
    document.getElementById('btn-fire').addEventListener('click', () => {
        const pattern = parseInt(document.getElementById('flame-pattern').value);
        const speed   = parseInt(speedSlider.value);
        sendCommand({ target: 'bcm', cmd: 'flame', pattern, speed });
    });

    // Botón armar alarma
    document.getElementById('btn-arm').addEventListener('click', () => {
        sendCommand({ target: 'bcm', cmd: 'alarm', state: 1 });
    });

    // Botón desarmar alarma
    document.getElementById('btn-disarm').addEventListener('click', () => {
        sendCommand({ target: 'bcm', cmd: 'alarm', state: 0 });
    });

    // Iniciar WS
    connect();
    log('Dashboard iniciado');
});
