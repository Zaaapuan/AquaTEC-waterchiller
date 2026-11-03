#ifndef DASHBOARD_HTML_H
#define DASHBOARD_HTML_H

#include <Arduino.h>

// Web Dashboard AquaTEC dalam PROGMEM
// Mengadopsi Desain AI Generatif Minimalis (design.md)
// - Warna: Dark Surface #202124, Accent AI Teal #00A67E, Kartu #2a2b2f
// - Tipografi: Sans-serif (Inter) & Monospace (JetBrains Mono)
// - Native HTML5 Canvas (0 KB external dependency, 100% mandiri lokal)

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AquaTEC Control System</title>
<style>
  :root {
    --bg-dark: #16171a;
    --card-bg: #202124;
    --card-border: #2e3035;
    --text-primary: #f0f0f0;
    --text-muted: #9aa0a6;
    --accent: #00A67E;
    --accent-hover: #008766;
    --green: #00E676;
    --amber: #FFB300;
    --red: #FF5252;
    --fault: #D50000;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background-color: var(--bg-dark);
    color: var(--text-primary);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Inter", sans-serif;
    padding: 16px;
    min-height: 100vh;
  }
  .container { max-width: 960px; margin: 0 auto; }
  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding-bottom: 16px;
    border-bottom: 1px solid var(--card-border);
    margin-bottom: 20px;
  }
  .logo { display: flex; align-items: center; gap: 10px; }
  .logo h1 { font-size: 1.4rem; font-weight: 700; letter-spacing: -0.5px; }
  .tag {
    background: rgba(0, 166, 126, 0.15);
    color: var(--accent);
    padding: 2px 8px;
    border-radius: 6px;
    font-size: 0.75rem;
    font-weight: 600;
  }
  .badge {
    padding: 6px 14px;
    border-radius: 20px;
    font-size: 0.8rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }
  .badge-normal { background: rgba(0, 230, 118, 0.15); color: var(--green); border: 1px solid var(--green); }
  .badge-warning { background: rgba(255, 179, 0, 0.15); color: var(--amber); border: 1px solid var(--amber); }
  .badge-critical { background: rgba(255, 82, 82, 0.15); color: var(--red); border: 1px solid var(--red); }
  .badge-fault { background: rgba(213, 0, 0, 0.25); color: var(--fault); border: 1px solid var(--fault); animation: pulse 1.5s infinite; }
  @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }

  .alert-banner {
    background: rgba(255, 82, 82, 0.2);
    border: 1px solid var(--red);
    color: var(--red);
    padding: 12px 16px;
    border-radius: 8px;
    margin-bottom: 20px;
    display: none;
    font-size: 0.9rem;
    font-weight: 600;
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 16px;
    margin-bottom: 20px;
  }
  .card {
    background: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 12px;
    padding: 16px;
  }
  .card-title {
    font-size: 0.8rem;
    text-transform: uppercase;
    color: var(--text-muted);
    letter-spacing: 0.5px;
    margin-bottom: 8px;
  }
  .card-value {
    font-family: "JetBrains Mono", Consolas, monospace;
    font-size: 1.9rem;
    font-weight: 700;
  }
  .card-unit { font-size: 1rem; color: var(--text-muted); font-weight: 400; margin-left: 2px; }
  .card-sub { font-size: 0.78rem; color: var(--text-muted); margin-top: 6px; }

  /* Baris Kontrol Utama (a. Card Set Point & b. Kontrol Sistem) */
  .control-row {
    display: grid;
    grid-template-columns: 1.5fr 1fr;
    gap: 16px;
    margin-bottom: 16px;
  }
  @media (max-width: 768px) {
    .control-row {
      grid-template-columns: 1fr;
    }
  }

  .card-main {
    display: flex;
    flex-direction: column;
    justify-content: center;
  }
  .sp-layout {
    display: flex;
    align-items: center;
    gap: 20px;
    flex-wrap: wrap;
  }
  .sp-target {
    min-width: 130px;
  }
  .sp-divider {
    width: 1px;
    height: 48px;
    background: var(--card-border);
  }
  .sp-input-box {
    flex: 1;
    min-width: 180px;
  }
  @media (max-width: 480px) {
    .sp-divider { display: none; }
  }

  .power-btn-row {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    margin-top: 8px;
  }
  .power-btn-row button {
    flex: 1;
    min-width: 120px;
  }

  /* Card Suhu Bersebelahan (Suhu Air & Suhu Heatsink) */
  .temp-card {
    margin-bottom: 16px;
    padding: 18px 24px;
  }
  .temp-grid {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    text-align: center;
    gap: 20px;
  }
  .temp-divider {
    width: 1px;
    height: 52px;
    background: var(--card-border);
  }
  @media (max-width: 480px) {
    .temp-grid {
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    .temp-divider {
      display: none;
    }
  }

  /* Grid Aktuator (c. Daya Peltier & d. Pompa) */
  .actuator-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    margin-bottom: 20px;
  }
  @media (max-width: 600px) {
    .actuator-grid {
      grid-template-columns: 1fr;
    }
  }

  .form-group { margin-bottom: 14px; }
  label { display: block; font-size: 0.85rem; color: var(--text-muted); margin-bottom: 6px; }
  .input-row { display: flex; gap: 8px; align-items: center; }
  input[type="number"] {
    flex: 1;
    background: #121315;
    border: 1px solid var(--card-border);
    border-radius: 8px;
    color: var(--text-primary);
    padding: 10px 12px;
    font-size: 1.1rem;
    font-family: "JetBrains Mono", monospace;
  }
  input[type="number"]:focus {
    outline: none;
    border-color: var(--accent);
  }
  button {
    background: var(--accent);
    color: #fff;
    border: none;
    border-radius: 8px;
    padding: 10px 18px;
    font-size: 0.9rem;
    font-weight: 600;
    cursor: pointer;
    transition: background 0.2s;
  }
  button:hover { background: var(--accent-hover); }
  button:disabled { background: #3a3b40; color: #777; cursor: not-allowed; }
  .btn-danger { background: var(--red); }
  .btn-danger:hover { background: #e04040; }
  .btn-outline {
    background: transparent;
    border: 1px solid var(--card-border);
    color: var(--text-primary);
  }
  .btn-outline:hover { background: #2e3035; }
  .btn-off { background: #444; }

  /* Tombol Kontrol Sistem: Aktif (Hijau) & Nonaktif (Merah) */
  .btn-power-on {
    background: #00B368;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 8px rgba(0, 179, 104, 0.25);
  }
  .btn-power-on:hover {
    background: #009959;
  }
  .btn-power-off {
    background: #E53935;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 8px rgba(229, 57, 53, 0.25);
  }
  .btn-power-off:hover {
    background: #C62828;
  }

  .chart-card {
    background: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 12px;
    padding: 16px;
    margin-bottom: 20px;
  }
  .chart-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 12px;
    margin-bottom: 10px;
  }
  .chart-legend {
    display: flex;
    gap: 14px;
    align-items: center;
    font-size: 0.8rem;
    color: var(--text-muted);
    margin-top: 4px;
  }
  .legend-item {
    display: flex;
    align-items: center;
    gap: 6px;
    font-weight: 500;
  }
  .legend-color {
    width: 12px;
    height: 12px;
    border-radius: 3px;
    display: inline-block;
  }
  .legend-water {
    background-color: #00A67E;
    box-shadow: 0 0 6px rgba(0, 166, 126, 0.4);
  }
  .legend-hs {
    background-color: #FF5252;
    box-shadow: 0 0 6px rgba(255, 82, 82, 0.4);
  }
  .zoom-controls {
    display: flex;
    gap: 6px;
    align-items: center;
    flex-wrap: wrap;
  }
  .btn-zoom {
    background: #2a2b2f;
    border: 1px solid var(--card-border);
    color: var(--text-primary);
    padding: 6px 11px;
    border-radius: 6px;
    font-size: 0.78rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
  }
  .btn-zoom:hover {
    background: #36383e;
    border-color: #4c4f57;
  }
  .btn-zoom-reset {
    color: var(--amber);
  }
  .zoom-badge {
    background: rgba(255, 255, 255, 0.08);
    color: var(--text-muted);
    padding: 5px 8px;
    border-radius: 6px;
    font-size: 0.75rem;
    font-family: "JetBrains Mono", monospace;
  }
  .chart-info-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 8px;
    padding: 8px 12px;
    background: #191a1d;
    border: 1px solid #28292d;
    border-radius: 8px;
    margin-bottom: 10px;
    font-family: "JetBrains Mono", monospace;
    font-size: 0.76rem;
    color: var(--text-muted);
  }
  .chart-info-bar .val-highlight {
    color: var(--text-primary);
    font-weight: 600;
  }
  .canvas-wrapper {
    position: relative;
    width: 100%;
    user-select: none;
    -webkit-user-select: none;
    touch-action: pan-y;
  }
  canvas {
    width: 100%;
    height: 250px;
    display: block;
    cursor: crosshair;
  }
  .chart-hint {
    font-size: 0.72rem;
    color: #7b8089;
    margin-top: 8px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 6px;
  }
  footer {
    text-align: center;
    color: var(--text-muted);
    font-size: 0.75rem;
    padding: 16px 0;
  }
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="logo">
      <h1>AquaTEC</h1>
      <span class="tag">V.1.4</span>
    </div>
    <div id="safetyBadge" class="badge badge-normal">NORMAL</div>
  </header>

  <div id="alertBanner" class="alert-banner"></div>

  <!-- Baris Kontrol Utama: a. Card Set Point (Main Card) & b. Kontrol Sistem (Second) -->
  <div class="control-row">
    <!-- a. Card Set Point (main card): Target & Input Set Point jadi 1 card -->
    <div class="card card-main">
      <div class="sp-layout">
        <div class="sp-target">
          <div class="card-title">Target (Set Point)</div>
          <div class="card-value"><span id="setpointVal">--.-</span><span class="card-unit">°C</span></div>
        </div>
        <div class="sp-divider"></div>
        <div class="sp-input-box">
          <div class="card-title">Input Set Point</div>
          <form class="input-row" onsubmit="event.preventDefault(); saveSetpoint();">
            <input type="number" id="spInput" min="20.0" max="30.0" step="0.5" placeholder="28.0" enterkeyhint="done">
            <button type="submit" id="btnSaveSp">Save</button>
          </form>
        </div>
      </div>
    </div>

    <!-- b. Kontrol Sistem (aktif nonaktif) (second) -->
    <div class="card card-second">
      <div class="card-title">Kontrol Sistem</div>
      <div class="power-btn-row">
        <button id="btnPower" onclick="togglePower()" class="btn-power-on">Sistem: AKTIF</button>
        <button id="btnResetFault" onclick="resetFault()" class="btn-danger" style="display:none;">Reset FAULT</button>
      </div>
      <div class="card-sub" id="uptimeText" style="margin-top:10px;">Uptime: 0m</div>
    </div>
  </div>

  <!-- 2. Card Suhu Air dan Suhu Heatsink (1 Card Bersebelahan) -->
  <div class="card temp-card">
    <div class="temp-grid">
      <div class="temp-col">
        <div class="card-title">Suhu Air</div>
        <div class="card-value"><span id="waterTemp">--.-</span><span class="card-unit">°C</span></div>
      </div>
      <div class="temp-divider"></div>
      <div class="temp-col">
        <div class="card-title">Suhu Heatsink</div>
        <div class="card-value"><span id="hsTemp">--.-</span><span class="card-unit">°C</span></div>
      </div>
    </div>
  </div>

  <!-- Aktuator Grid: c. Daya Peltier & d. Pompa -->
  <div class="actuator-grid">
    <!-- c. Daya Peltier -->
    <div class="card">
      <div class="card-title">Daya Peltier (PWM)</div>
      <div class="card-value"><span id="peltierPct">0</span><span class="card-unit">%</span></div>
      <div class="card-sub" id="peltierDetail">PID: 0%</div>
    </div>

    <!-- d. Pompa -->
    <div class="card">
      <div class="card-title">Sirkulasi Pompa</div>
      <div class="card-value"><span id="pumpStatus">OFF</span></div>
      <div class="card-sub">Otomatis saat sistem aktif</div>
    </div>
  </div>

  <!-- e. Live Canvas Chart (Grafik Perbandingan Suhu Air vs Heatsink) -->
  <div class="chart-card">
    <div class="chart-header">
      <div>
        <div class="card-title" style="margin-bottom:2px;">Perbandingan Suhu Air & Panas Heatsink</div>
        <div class="chart-legend">
          <span class="legend-item"><span class="legend-color legend-water"></span>Suhu Air (Dingin)</span>
          <span class="legend-item"><span class="legend-color legend-hs"></span>Suhu Heatsink (Panas)</span>
        </div>
      </div>
      <div class="zoom-controls">
        <button type="button" class="btn-zoom" id="btnZoomIn" title="Perbesar Grafik">+ Zoom In</button>
        <button type="button" class="btn-zoom" id="btnZoomOut" title="Perkecil Grafik">− Zoom Out</button>
        <button type="button" class="btn-zoom btn-zoom-reset" id="btnZoomReset" title="Reset Tampilan Penuh">⟲ Reset</button>
        <span class="zoom-badge" id="zoomBadge">1.0x</span>
      </div>
    </div>

    <!-- Live Readout Koordinat Kursor (X & Y) -->
    <div class="chart-info-bar" id="chartInfoBar">
      <span>Waktu: <span class="val-highlight" id="readoutTime">--:--:--</span></span>
      <span>Air: <span class="val-highlight" style="color:var(--accent);" id="readoutWater">--.- °C</span></span>
      <span>Heatsink: <span class="val-highlight" style="color:var(--red);" id="readoutHs">--.- °C</span></span>
      <span>ΔT: <span class="val-highlight" style="color:var(--amber);" id="readoutDiff">--.- °C</span></span>
    </div>

    <div class="canvas-wrapper">
      <canvas id="historyCanvas"></canvas>
    </div>
  </div>

  <footer>
    AquaTEC Embedded System
  </footer>
</div>

<script>
let currentEnabled = true;
let chartWaterData = [];
let chartHsData = [];
let chartTimes = [];
let initialSetpointLoaded = false;

// State Zoom, Pan, dan Interaksi Kursor
let zoomFactor = 1.0;
let panOffset = 0.0;
let hoverIdx = -1;
let isDragging = false;
let dragStartX = 0;
let dragStartPan = 0;

function updateStatus() {
  fetch('/api/status')
    .then(res => res.json())
    .then(data => {
      document.getElementById('waterTemp').innerText = data.water_temp.toFixed(1);
      document.getElementById('hsTemp').innerText = data.heatsink_temp.toFixed(1);
      document.getElementById('setpointVal').innerText = data.setpoint.toFixed(1);

      // Nilai awal pada INPUT adalah nilai terakhir dari set point yang ditetapkan
      const spInputEl = document.getElementById('spInput');
      if (!initialSetpointLoaded && data.setpoint !== undefined) {
        spInputEl.value = data.setpoint.toFixed(1);
        initialSetpointLoaded = true;
      }

      document.getElementById('peltierPct').innerText = Math.round(data.peltier_pct);
      document.getElementById('peltierDetail').innerText = 'PID: ' + Math.round(data.pid_output || 0) + '%';
      
      const pumpEl = document.getElementById('pumpStatus');
      pumpEl.innerText = data.pump ? 'ON' : 'OFF';
      pumpEl.style.color = data.pump ? 'var(--green)' : 'var(--text-muted)';

      // Safety Badge
      const badge = document.getElementById('safetyBadge');
      badge.innerText = data.safety;
      badge.className = 'badge badge-' + data.safety.toLowerCase();

      // Tombol Power
      currentEnabled = data.enabled;
      const btnPower = document.getElementById('btnPower');
      if (currentEnabled) {
        btnPower.innerText = 'AKTIF';
        btnPower.className = 'btn-power-on';
      } else {
        btnPower.innerText = 'NONAKTIF';
        btnPower.className = 'btn-power-off';
      }

      // Tombol Reset Fault
      const btnReset = document.getElementById('btnResetFault');
      btnReset.style.display = (data.safety === 'FAULT') ? 'block' : 'none';

      // Alert Banner
      const banner = document.getElementById('alertBanner');
      if (!data.sensor_ok) {
        banner.style.display = 'block';
        banner.innerText = 'PERINGATAN: Salah satu sensor DS18B20 terputus atau mengalami gangguan!';
      } else if (data.water_temp > 32.0) {
        banner.style.display = 'block';
        banner.innerText = 'ALARM: Suhu air melebihi 32.0 °C! Periksa sistem pendingin.';
      } else {
        banner.style.display = 'none';
      }

      // Uptime
      const mins = Math.floor(data.uptime_s / 60);
      const hours = Math.floor(mins / 60);
      document.getElementById('uptimeText').innerText = 'Uptime: ' + (hours > 0 ? hours + 'j ' : '') + (mins % 60) + 'm';
    })
    .catch(err => console.error('Gagal mengambil status:', err));
}

function saveSetpoint() {
  const spInputEl = document.getElementById('spInput');
  spInputEl.blur();
  const val = parseFloat(spInputEl.value);
  if (val < 20.0 || val > 30.0 || isNaN(val)) {
    alert('Set point harus berada dalam rentang 20.0 - 30.0 °C');
    return;
  }
  fetch('/api/setpoint', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ value: val })
  })
  .then(res => {
    if (res.ok) {
      spInputEl.value = val.toFixed(1);
      updateStatus();
    } else {
      alert('Gagal menyimpan setpoint. Nilai di luar rentang.');
    }
  });
}

document.getElementById('spInput').addEventListener('keydown', function(e) {
  if (e.key === 'Enter') {
    e.preventDefault();
    saveSetpoint();
  }
});

function togglePower() {
  fetch('/api/power', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: !currentEnabled })
  })
  .then(res => updateStatus());
}

function resetFault() {
  fetch('/api/reset-fault', { method: 'POST' })
    .then(res => res.json())
    .then(data => {
      if (!data.success) {
        alert('Gagal reset FAULT: ' + (data.message || 'Heatsink masih terlalu panas!'));
      }
      updateStatus();
    });
}

function updateTimestamps(len) {
  const now = Date.now();
  if (chartTimes.length === len && len > 0) return;
  chartTimes = [];
  for (let i = 0; i < len; i++) {
    // Tiap titik disampling tiap 10 detik
    chartTimes.push(new Date(now - (len - 1 - i) * 10000));
  }
}

function formatTime(d) {
  if (!d) return '--:--:--';
  const pad = n => (n < 10 ? '0' + n : n);
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
}

function updateInfoBarDefault() {
  if (hoverIdx >= 0) return;
  const N = chartWaterData.length;
  if (N > 0) {
    const lastIdx = N - 1;
    const w = chartWaterData[lastIdx];
    const h = (chartHsData && chartHsData.length > lastIdx) ? chartHsData[lastIdx] : null;
    const t = chartTimes[lastIdx];

    document.getElementById('readoutTime').innerText = formatTime(t) + ' (Terbaru)';
    document.getElementById('readoutWater').innerText = (typeof w === 'number' && !isNaN(w)) ? w.toFixed(1) + ' °C' : '--.- °C';
    document.getElementById('readoutHs').innerText = (typeof h === 'number' && !isNaN(h)) ? h.toFixed(1) + ' °C' : '--.- °C';
    if (typeof w === 'number' && typeof h === 'number' && !isNaN(w) && !isNaN(h)) {
      const diff = h - w;
      document.getElementById('readoutDiff').innerText = (diff >= 0 ? '+' : '') + diff.toFixed(1) + ' °C';
    } else {
      document.getElementById('readoutDiff').innerText = '--.- °C';
    }
  }
}

function loadHistory() {
  fetch('/api/history')
    .then(res => res.json())
    .then(data => {
      if (Array.isArray(data)) {
        chartWaterData = data;
        chartHsData = [];
      } else if (data && typeof data === 'object') {
        chartWaterData = data.water || [];
        chartHsData = data.heatsink || [];
      }
      const len = chartWaterData.length;
      updateTimestamps(len);
      updateInfoBarDefault();
      drawChart();
    })
    .catch(err => console.error('Gagal mengambil riwayat suhu:', err));
}

function setZoom(newZoom, focalRatio = 0.5) {
  const N = chartWaterData.length;
  if (N < 4) return;
  const clampedZoom = Math.min(10.0, Math.max(1.0, newZoom));
  if (Math.abs(clampedZoom - zoomFactor) < 0.01) return;

  const currentVisible = Math.max(4, Math.floor(N / zoomFactor));
  const maxPanOld = N - currentVisible;
  const startOld = maxPanOld > 0 ? Math.round(panOffset * maxPanOld) : 0;
  const focalIndex = startOld + focalRatio * (currentVisible - 1);

  zoomFactor = clampedZoom;
  if (zoomFactor <= 1.0) {
    zoomFactor = 1.0;
    panOffset = 0.0;
  } else {
    const newVisible = Math.max(4, Math.floor(N / zoomFactor));
    const newStart = focalIndex - focalRatio * (newVisible - 1);
    const maxPanNew = N - newVisible;
    panOffset = maxPanNew > 0 ? Math.max(0, Math.min(1, newStart / maxPanNew)) : 0.0;
  }
  updateZoomUI();
  drawChart();
}

function updateZoomUI() {
  const badge = document.getElementById('zoomBadge');
  if (badge) {
    badge.innerText = zoomFactor.toFixed(1) + 'x';
    badge.style.color = (zoomFactor > 1.0) ? 'var(--accent)' : 'var(--text-muted)';
  }
  const canvas = document.getElementById('historyCanvas');
  if (canvas) {
    canvas.style.cursor = (zoomFactor > 1.0) ? (isDragging ? 'grabbing' : 'grab') : 'crosshair';
  }
}

function drawChart() {
  const canvas = document.getElementById('historyCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  const w = rect.width;
  const h = rect.height;
  const padding = { top: 20, right: 25, bottom: 35, left: 45 };

  ctx.clearRect(0, 0, w, h);

  const N = chartWaterData.length;
  if (N < 2) {
    ctx.fillStyle = '#9aa0a6';
    ctx.font = '12px Inter, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Mengumpulkan data riwayat suhu...', w / 2, h / 2);
    return;
  }

  // Hitung jendela data yang tampil saat Zoom & Pan
  const visibleCount = Math.max(4, Math.min(N, Math.floor(N / zoomFactor)));
  const maxPan = N - visibleCount;
  const startIdx = maxPan > 0 ? Math.round(panOffset * maxPan) : 0;
  const endIdx = startIdx + visibleCount - 1;

  const sliceWater = chartWaterData.slice(startIdx, endIdx + 1);
  const sliceHs = (chartHsData && chartHsData.length >= endIdx + 1) ? chartHsData.slice(startIdx, endIdx + 1) : [];
  const sliceTimes = (chartTimes && chartTimes.length >= endIdx + 1) ? chartTimes.slice(startIdx, endIdx + 1) : [];

  // Tentukan batas min & max sumbu Y dari data air dan heatsink
  let minT = 100, maxT = -100;
  for (let i = 0; i < sliceWater.length; i++) {
    const val = sliceWater[i];
    if (typeof val === 'number' && !isNaN(val)) {
      if (val < minT) minT = val;
      if (val > maxT) maxT = val;
    }
  }
  for (let i = 0; i < sliceHs.length; i++) {
    const val = sliceHs[i];
    if (typeof val === 'number' && !isNaN(val)) {
      if (val < minT) minT = val;
      if (val > maxT) maxT = val;
    }
  }
  if (minT > maxT) { minT = 20; maxT = 35; }
  minT = Math.floor(minT) - 1;
  maxT = Math.ceil(maxT) + 1;
  if (maxT - minT < 4) { maxT = minT + 4; }

  const chartW = w - padding.left - padding.right;
  const chartH = h - padding.top - padding.bottom;

  // 1. Grid Horizontal & Label Sumbu Y
  ctx.strokeStyle = '#282a2e';
  ctx.lineWidth = 1;
  ctx.setLineDash([]);
  ctx.fillStyle = '#9aa0a6';
  ctx.font = '10px "JetBrains Mono", monospace';
  ctx.textAlign = 'right';

  const stepsY = 4;
  for (let i = 0; i <= stepsY; i++) {
    const yVal = minT + ((maxT - minT) * i / stepsY);
    const y = padding.top + chartH - (i * chartH / stepsY);
    ctx.beginPath();
    ctx.moveTo(padding.left, y);
    ctx.lineTo(w - padding.right, y);
    ctx.stroke();
    ctx.fillText(yVal.toFixed(1) + '°C', padding.left - 6, y + 3);
  }

  // 2. Rentetan Waktu Sampling pada Sumbu X (Point 5)
  const numXTicks = Math.min(6, Math.max(3, visibleCount));
  ctx.fillStyle = '#9aa0a6';
  ctx.font = '10px "JetBrains Mono", monospace';

  for (let k = 0; k < numXTicks; k++) {
    const ratio = k / (numXTicks - 1);
    const x = padding.left + ratio * chartW;
    const relIdx = Math.round(ratio * (visibleCount - 1));
    const pointIdx = startIdx + relIdx;
    const tObj = chartTimes[pointIdx];
    const timeLabel = formatTime(tObj);

    // Garis grid vertikal putus-putus
    ctx.beginPath();
    ctx.setLineDash([3, 4]);
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
    ctx.moveTo(x, padding.top);
    ctx.lineTo(x, padding.top + chartH);
    ctx.stroke();

    // Tick mark sumbu X
    ctx.beginPath();
    ctx.setLineDash([]);
    ctx.strokeStyle = '#2e3035';
    ctx.moveTo(x, padding.top + chartH);
    ctx.lineTo(x, padding.top + chartH + 4);
    ctx.stroke();

    // Teks Label Waktu Sampling
    if (k === 0) ctx.textAlign = 'left';
    else if (k === numXTicks - 1) ctx.textAlign = 'right';
    else ctx.textAlign = 'center';

    ctx.fillText(timeLabel, x, padding.top + chartH + 18);
  }
  ctx.setLineDash([]);

  // 3. Garis & Area Suhu Heatsink (Merah Color Tone Terkalibrasi - Point 3)
  if (sliceHs.length >= 2) {
    // Area Gradien Bawah Heatsink
    ctx.beginPath();
    for (let i = 0; i < sliceHs.length; i++) {
      const x = padding.left + (i * chartW / (sliceHs.length - 1));
      const val = sliceHs[i];
      const y = padding.top + chartH - ((val - minT) / (maxT - minT) * chartH);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    const lastX = padding.left + chartW;
    const bottomY = padding.top + chartH;
    ctx.lineTo(lastX, bottomY);
    ctx.lineTo(padding.left, bottomY);
    ctx.closePath();
    const gradHs = ctx.createLinearGradient(0, padding.top, 0, bottomY);
    gradHs.addColorStop(0, 'rgba(255, 82, 82, 0.16)');
    gradHs.addColorStop(1, 'rgba(255, 82, 82, 0.0)');
    ctx.fillStyle = gradHs;
    ctx.fill();

    // Garis Kurva Suhu Heatsink
    ctx.beginPath();
    ctx.strokeStyle = '#FF5252';
    ctx.lineWidth = 2;
    for (let i = 0; i < sliceHs.length; i++) {
      const x = padding.left + (i * chartW / (sliceHs.length - 1));
      const val = sliceHs[i];
      const y = padding.top + chartH - ((val - minT) / (maxT - minT) * chartH);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // 4. Garis & Area Suhu Air (Teal)
  ctx.beginPath();
  for (let i = 0; i < sliceWater.length; i++) {
    const x = padding.left + (i * chartW / (sliceWater.length - 1));
    const val = sliceWater[i];
    const y = padding.top + chartH - ((val - minT) / (maxT - minT) * chartH);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  const lastX = padding.left + chartW;
  const bottomY = padding.top + chartH;
  ctx.lineTo(lastX, bottomY);
  ctx.lineTo(padding.left, bottomY);
  ctx.closePath();
  const gradWater = ctx.createLinearGradient(0, padding.top, 0, bottomY);
  gradWater.addColorStop(0, 'rgba(0, 166, 126, 0.22)');
  gradWater.addColorStop(1, 'rgba(0, 166, 126, 0.0)');
  ctx.fillStyle = gradWater;
  ctx.fill();

  // Garis Kurva Suhu Air
  ctx.beginPath();
  ctx.strokeStyle = '#00A67E';
  ctx.lineWidth = 2;
  for (let i = 0; i < sliceWater.length; i++) {
    const x = padding.left + (i * chartW / (sliceWater.length - 1));
    const val = sliceWater[i];
    const y = padding.top + chartH - ((val - minT) / (maxT - minT) * chartH);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // 5. Interaktivitas Hover Kursor Koordinat X dan Y (Point 4)
  if (hoverIdx >= startIdx && hoverIdx <= endIdx) {
    const relIdx = hoverIdx - startIdx;
    const xHover = padding.left + (relIdx * chartW / (visibleCount - 1));
    const wVal = chartWaterData[hoverIdx];
    const hVal = (chartHsData && chartHsData.length > hoverIdx) ? chartHsData[hoverIdx] : null;
    const tVal = chartTimes[hoverIdx];

    const yWater = padding.top + chartH - ((wVal - minT) / (maxT - minT) * chartH);
    const yHs = (hVal !== null) ? (padding.top + chartH - ((hVal - minT) / (maxT - minT) * chartH)) : null;

    // Garis Crosshair Vertikal
    ctx.beginPath();
    ctx.setLineDash([3, 3]);
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.35)';
    ctx.lineWidth = 1;
    ctx.moveTo(xHover, padding.top);
    ctx.lineTo(xHover, padding.top + chartH);
    ctx.stroke();
    ctx.setLineDash([]);

    // Titik Sorot Suhu Air (Teal)
    ctx.beginPath();
    ctx.arc(xHover, yWater, 6, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(0, 166, 126, 0.4)';
    ctx.fill();
    ctx.beginPath();
    ctx.arc(xHover, yWater, 4, 0, Math.PI * 2);
    ctx.fillStyle = '#00A67E';
    ctx.fill();
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1.5;
    ctx.stroke();

    // Titik Sorot Suhu Heatsink (Merah)
    if (yHs !== null) {
      ctx.beginPath();
      ctx.arc(xHover, yHs, 6, 0, Math.PI * 2);
      ctx.fillStyle = 'rgba(255, 82, 82, 0.4)';
      ctx.fill();
      ctx.beginPath();
      ctx.arc(xHover, yHs, 4, 0, Math.PI * 2);
      ctx.fillStyle = '#FF5252';
      ctx.fill();
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 1.5;
      ctx.stroke();
    }

    // Kotak Tooltip Floating (Koordinat X dan Y)
    const timeStr = formatTime(tVal);
    const boxW = 190;
    const boxH = 82;
    let boxX = xHover + 12;
    if (boxX + boxW > w - padding.right) {
      boxX = xHover - boxW - 12;
    }
    let boxY = padding.top + 8;

    ctx.save();
    ctx.fillStyle = 'rgba(20, 21, 24, 0.94)';
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.16)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    const r = 8;
    ctx.moveTo(boxX + r, boxY);
    ctx.lineTo(boxX + boxW - r, boxY);
    ctx.quadraticCurveTo(boxX + boxW, boxY, boxX + boxW, boxY + r);
    ctx.lineTo(boxX + boxW, boxY + boxH - r);
    ctx.quadraticCurveTo(boxX + boxW, boxY + boxH, boxX + boxW - r, boxY + boxH);
    ctx.lineTo(boxX + r, boxY + boxH);
    ctx.quadraticCurveTo(boxX, boxY + boxH, boxX, boxY + boxH - r);
    ctx.lineTo(boxX, boxY + r);
    ctx.quadraticCurveTo(boxX, boxY, boxX + r, boxY);
    ctx.closePath();
    ctx.fill();
    ctx.stroke();

    ctx.textAlign = 'left';
    ctx.font = '10px "JetBrains Mono", monospace';

    // Koordinat X: Waktu sampling & nomor sampel
    ctx.fillStyle = '#9aa0a6';
    ctx.fillText('⏱ Waktu: ' + timeStr + ' (#' + (hoverIdx + 1) + ')', boxX + 10, boxY + 18);

    // Koordinat Y: Suhu Air
    ctx.fillStyle = '#00E676';
    ctx.fillText('💧 Suhu Air: ' + wVal.toFixed(1) + ' °C', boxX + 10, boxY + 36);

    // Koordinat Y: Suhu Heatsink
    ctx.fillStyle = '#FF5252';
    const hsStr = (hVal !== null) ? hVal.toFixed(1) + ' °C' : '--.- °C';
    ctx.fillText('🔥 Suhu Heatsink: ' + hsStr, boxX + 10, boxY + 54);

    // Perbedaan / Selisih Suhu ΔT
    if (hVal !== null) {
      const diff = hVal - wVal;
      ctx.fillStyle = '#FFB300';
      ctx.fillText('⚡ ΔT Selisih: ' + (diff >= 0 ? '+' : '') + diff.toFixed(1) + ' °C', boxX + 10, boxY + 72);
    }
    ctx.restore();

    // Perbarui Info Bar di atas grafik
    document.getElementById('readoutTime').innerText = timeStr + ' (#' + (hoverIdx + 1) + ')';
    document.getElementById('readoutWater').innerText = wVal.toFixed(1) + ' °C';
    document.getElementById('readoutHs').innerText = (hVal !== null) ? hVal.toFixed(1) + ' °C' : '--.- °C';
    if (hVal !== null) {
      const diff = hVal - wVal;
      document.getElementById('readoutDiff').innerText = (diff >= 0 ? '+' : '') + diff.toFixed(1) + ' °C';
    }
  } else {
    updateInfoBarDefault();
  }
}

// Inisialisasi Event Listener Interaktivitas & Zoom (Point 4 & 6)
const canvasEl = document.getElementById('historyCanvas');

function getCanvasRelativeCoords(e) {
  const rect = canvasEl.getBoundingClientRect();
  const clientX = e.touches ? e.touches[0].clientX : e.clientX;
  const clientY = e.touches ? e.touches[0].clientY : e.clientY;
  return {
    x: clientX - rect.left,
    y: clientY - rect.top,
    rect: rect
  };
}

function handlePointerMove(e) {
  if (isDragging) return;
  const { x, y, rect } = getCanvasRelativeCoords(e);
  const padding = { top: 20, right: 25, bottom: 35, left: 45 };
  const chartW = rect.width - padding.left - padding.right;

  if (x >= padding.left && x <= rect.width - padding.right && y >= padding.top && y <= rect.height - padding.bottom) {
    const N = chartWaterData.length;
    if (N > 1) {
      const visibleCount = Math.max(4, Math.min(N, Math.floor(N / zoomFactor)));
      const maxPan = N - visibleCount;
      const startIdx = maxPan > 0 ? Math.round(panOffset * maxPan) : 0;
      const ratio = Math.max(0, Math.min(1, (x - padding.left) / chartW));
      const relIdx = Math.round(ratio * (visibleCount - 1));
      hoverIdx = Math.max(0, Math.min(N - 1, startIdx + relIdx));
    }
  } else {
    hoverIdx = -1;
  }
  drawChart();
}

canvasEl.addEventListener('mousemove', handlePointerMove);
canvasEl.addEventListener('mouseleave', () => {
  if (!isDragging) {
    hoverIdx = -1;
    drawChart();
  }
});

// Drag to Pan saat Zoom > 1.0x
canvasEl.addEventListener('mousedown', (e) => {
  if (zoomFactor > 1.0) {
    isDragging = true;
    dragStartX = e.clientX;
    dragStartPan = panOffset;
    canvasEl.style.cursor = 'grabbing';
  }
});

window.addEventListener('mousemove', (e) => {
  if (isDragging) {
    const dx = e.clientX - dragStartX;
    const rect = canvasEl.getBoundingClientRect();
    const padding = { top: 20, right: 25, bottom: 35, left: 45 };
    const chartW = rect.width - padding.left - padding.right;
    const dPan = -dx / chartW;
    panOffset = Math.max(0, Math.min(1, dragStartPan + dPan));
    drawChart();
  }
});

window.addEventListener('mouseup', () => {
  if (isDragging) {
    isDragging = false;
    updateZoomUI();
    drawChart();
  }
});

// Mouse Wheel Zoom
canvasEl.addEventListener('wheel', (e) => {
  e.preventDefault();
  const rect = canvasEl.getBoundingClientRect();
  const padding = { top: 20, right: 25, bottom: 35, left: 45 };
  const chartW = rect.width - padding.left - padding.right;
  const ratio = Math.max(0, Math.min(1, (e.clientX - rect.left - padding.left) / chartW));

  if (e.deltaY < 0) {
    setZoom(zoomFactor * 1.3, ratio);
  } else {
    setZoom(zoomFactor / 1.3, ratio);
  }
}, { passive: false });

// Touch Drag & Pinch Support untuk Perangkat Layar Sentuh
let touchStartDist = 0;
let touchStartZoom = 1.0;

canvasEl.addEventListener('touchstart', (e) => {
  if (e.touches.length === 1) {
    if (zoomFactor > 1.0) {
      isDragging = true;
      dragStartX = e.touches[0].clientX;
      dragStartPan = panOffset;
    }
    handlePointerMove(e);
  } else if (e.touches.length === 2) {
    isDragging = false;
    touchStartDist = Math.hypot(
      e.touches[0].clientX - e.touches[1].clientX,
      e.touches[0].clientY - e.touches[1].clientY
    );
    touchStartZoom = zoomFactor;
  }
}, { passive: true });

canvasEl.addEventListener('touchmove', (e) => {
  if (e.touches.length === 1) {
    if (isDragging) {
      const dx = e.touches[0].clientX - dragStartX;
      const rect = canvasEl.getBoundingClientRect();
      const padding = { top: 20, right: 25, bottom: 35, left: 45 };
      const chartW = rect.width - padding.left - padding.right;
      panOffset = Math.max(0, Math.min(1, dragStartPan - (dx / chartW)));
      drawChart();
    } else {
      handlePointerMove(e);
    }
  } else if (e.touches.length === 2 && touchStartDist > 0) {
    const dist = Math.hypot(
      e.touches[0].clientX - e.touches[1].clientX,
      e.touches[0].clientY - e.touches[1].clientY
    );
    const scale = dist / touchStartDist;
    setZoom(touchStartZoom * scale, 0.5);
  }
}, { passive: true });

canvasEl.addEventListener('touchend', () => {
  isDragging = false;
  touchStartDist = 0;
  updateZoomUI();
});

// Tombol Zoom In, Zoom Out, dan Reset (Point 6)
document.getElementById('btnZoomIn').addEventListener('click', () => setZoom(zoomFactor * 1.5, 0.5));
document.getElementById('btnZoomOut').addEventListener('click', () => setZoom(zoomFactor / 1.5, 0.5));
document.getElementById('btnZoomReset').addEventListener('click', () => {
  zoomFactor = 1.0;
  panOffset = 0.0;
  updateZoomUI();
  drawChart();
});

window.addEventListener('resize', drawChart);

// Mulai Polling API Berkala
updateStatus();
loadHistory();
setInterval(updateStatus, 2000);
setInterval(loadHistory, 10000);
</script>
</body>
</html>
)rawliteral";

#endif // DASHBOARD_HTML_H

