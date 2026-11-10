#ifndef DEVMODE_HTML_H
#define DEVMODE_HTML_H

#include <Arduino.h>

// Halaman Developer Mode AquaTEC
// Didesain mengikuti estetika profesional MATLAB Publication Figures:
// - Area Figure & Axes: Background putih murni #FFFFFF
// - Sumbu & Box: MATLAB standard 'box on' dengan inward tick marks dan grid putus-putus
// - Color Order: MATLAB default color palette (#0072BD, #D95319, #EDB120, #7E2F8E, #77AC30, #4DBEEE, #A2142F)
// - Legend Box: Kotak legend khas MATLAB dengan border halus
// - Ekspor PNG: Resolusi tinggi (High-DPI) standar publikasi jurnal/laporan teknik

const char DEVMODE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AquaTEC Developer Mode & MATLAB Telemetry</title>
<style>
  :root {
    --bg-dark: #121316;
    --card-bg: #1c1d22;
    --card-secondary: #23252b;
    --card-border: #2c2e36;
    --text-primary: #f0f0f2;
    --text-muted: #8e929b;
    --accent: #00A67E;
    --accent-hover: #008766;
    --green: #00E676;
    --amber: #FFB300;
    --red: #FF5252;
    --fault: #D50000;
    
    /* MATLAB Color Palette */
    --matlab-blue: #0072BD;
    --matlab-orange: #D95319;
    --matlab-yellow: #EDB120;
    --matlab-purple: #7E2F8E;
    --matlab-green: #77AC30;
    --matlab-cyan: #4DBEEE;
    --matlab-darkred: #A2142F;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background-color: var(--bg-dark);
    color: var(--text-primary);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Inter", sans-serif;
    padding: 18px;
    min-height: 100vh;
  }
  .container { max-width: 1140px; margin: 0 auto; }
  
  /* Header */
  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding-bottom: 14px;
    border-bottom: 1px solid var(--card-border);
    margin-bottom: 18px;
    flex-wrap: wrap;
    gap: 12px;
  }
  .header-left { display: flex; align-items: center; gap: 12px; }
  .logo-title { font-size: 1.3rem; font-weight: 700; letter-spacing: -0.4px; }
  .dev-tag {
    background: rgba(0, 166, 126, 0.12);
    color: var(--accent);
    padding: 3px 8px;
    border-radius: 6px;
    font-size: 0.72rem;
    font-weight: 700;
    border: 1px solid rgba(0, 166, 126, 0.28);
    font-family: "JetBrains Mono", monospace;
  }
  .btn-back {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    background: var(--card-bg);
    color: var(--text-primary);
    border: 1px solid var(--card-border);
    padding: 7px 14px;
    border-radius: 6px;
    font-size: 0.84rem;
    font-weight: 600;
    text-decoration: none;
    transition: all 0.2s;
  }
  .btn-back:hover {
    background: var(--card-secondary);
    border-color: var(--accent);
    color: var(--accent);
  }

  /* Grid Layout */
  .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 16px; margin-bottom: 16px; }

  /* Cards */
  .card {
    background-color: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 8px;
    padding: 16px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.18);
  }
  .card-title {
    font-size: 0.85rem;
    font-weight: 600;
    color: var(--text-muted);
    text-transform: uppercase;
    letter-spacing: 0.6px;
    margin-bottom: 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  /* Status Badges */
  .badge {
    padding: 3px 9px;
    border-radius: 4px;
    font-size: 0.72rem;
    font-weight: 700;
    text-transform: uppercase;
    font-family: "JetBrains Mono", monospace;
  }
  .badge-normal { background: rgba(0, 230, 118, 0.12); color: var(--green); border: 1px solid var(--green); }
  .badge-warning { background: rgba(255, 179, 0, 0.12); color: var(--amber); border: 1px solid var(--amber); }
  .badge-critical { background: rgba(255, 82, 82, 0.12); color: var(--red); border: 1px solid var(--red); }
  .badge-fault { background: rgba(213, 0, 0, 0.2); color: var(--fault); border: 1px solid var(--fault); animation: pulse 1.5s infinite; }
  @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }

  /* Info Lists */
  .info-row {
    display: flex;
    justify-content: space-between;
    padding: 6px 0;
    border-bottom: 1px solid rgba(255, 255, 255, 0.035);
    font-size: 0.86rem;
  }
  .info-row:last-child { border-bottom: none; }
  .info-val { font-family: "JetBrains Mono", monospace; font-weight: 600; color: #FFFFFF; }

  /* Buttons & Controls */
  button, .btn {
    background-color: var(--accent);
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 7px 13px;
    font-size: 0.82rem;
    font-weight: 600;
    cursor: pointer;
    transition: background-color 0.2s, transform 0.08s;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
  }
  button:hover { background-color: var(--accent-hover); }
  button:active { transform: scale(0.98); }
  button.btn-secondary {
    background-color: var(--card-secondary);
    border: 1px solid var(--card-border);
    color: var(--text-primary);
  }
  button.btn-secondary:hover { background-color: #2b2e37; border-color: #3e424f; }
  button.btn-matlab {
    background-color: #0072BD;
    color: #ffffff;
    border: 1px solid #005a96;
  }
  button.btn-matlab:hover { background-color: #005a96; }
  button.btn-fault, .btn-fault {
    background-color: var(--fault);
    color: #ffffff;
    border: 1px solid #990000;
    font-weight: 700;
    box-shadow: 0 2px 8px rgba(213, 0, 0, 0.35);
  }
  button.btn-fault:hover, .btn-fault:hover {
    background-color: #b70000;
  }
  button.btn-danger {
    background-color: rgba(255, 82, 82, 0.12);
    color: var(--red);
    border: 1px solid var(--red);
  }
  button.btn-danger:hover { background-color: var(--red); color: #fff; }
  button.btn-warning {
    background-color: rgba(255, 179, 0, 0.12);
    color: var(--amber);
    border: 1px solid var(--amber);
  }
  button.btn-warning:hover { background-color: var(--amber); color: #111; }

  /* Inputs */
  input[type="number"], select {
    background: #141518;
    border: 1px solid var(--card-border);
    border-radius: 5px;
    color: var(--text-primary);
    padding: 6px 9px;
    font-size: 0.85rem;
    font-family: "JetBrains Mono", monospace;
  }
  input[type="number"]:focus, select:focus {
    border-color: var(--accent);
    outline: none;
  }

  .form-row {
    display: flex;
    gap: 8px;
    align-items: center;
    margin-bottom: 9px;
    flex-wrap: wrap;
  }
  .form-label {
    min-width: 135px;
    font-size: 0.83rem;
    color: var(--text-muted);
  }

  /* =========================================================================
     MATLAB FIGURE SECTION (Clean White Background Publication Style)
     ========================================================================= */
  .matlab-figure-card {
    background-color: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 8px;
    padding: 14px;
    margin-bottom: 16px;
    box-shadow: 0 4px 16px rgba(0,0,0,0.25);
  }
  .matlab-toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 10px;
    margin-bottom: 12px;
    padding-bottom: 10px;
    border-bottom: 1px solid rgba(255,255,255,0.05);
  }
  .series-toggles {
    display: flex;
    flex-wrap: wrap;
    gap: 12px;
    font-size: 0.8rem;
  }
  .series-toggle-item {
    display: flex;
    align-items: center;
    gap: 6px;
    cursor: pointer;
    user-select: none;
  }
  .matlab-color-legend {
    width: 14px;
    height: 3px;
    display: inline-block;
    border-radius: 2px;
  }
  
  /* MATLAB Window Canvas Wrapper */
  .matlab-window-frame {
    background: #FFFFFF;
    border: 1px solid #c8c8c8;
    border-radius: 6px;
    overflow: hidden;
    box-shadow: 0 2px 10px rgba(0,0,0,0.3);
  }
  .matlab-window-header {
    background: #EBEBEB;
    border-bottom: 1px solid #D0D0D0;
    padding: 6px 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    font-size: 0.75rem;
    color: #333333;
    font-weight: 500;
  }
  .matlab-window-header .dots {
    display: flex;
    gap: 5px;
  }
  .matlab-window-header .dot {
    width: 9px;
    height: 9px;
    border-radius: 50%;
    background: #CCCCCC;
  }
  .chart-container {
    width: 100%;
    position: relative;
    background: #FFFFFF;
    padding: 0;
    display: block;
    cursor: crosshair;
  }
  #chartCanvas {
    width: 100%;
    height: 420px;
    display: block;
  }

  /* Quick Test Grid */
  .quick-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(210px, 1fr));
    gap: 10px;
  }
  .quick-btn {
    width: 100%;
    padding: 9px 11px;
    text-align: left;
    font-size: 0.82rem;
    line-height: 1.3;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  .quick-btn span.subtext {
    font-size: 0.71rem;
    opacity: 0.8;
    font-weight: 400;
  }

  /* Toast Notification */
  #toast {
    position: fixed;
    bottom: 20px;
    right: 20px;
    padding: 9px 15px;
    background: #23252b;
    border: 1px solid var(--accent);
    color: #ffffff;
    border-radius: 6px;
    font-size: 0.82rem;
    box-shadow: 0 4px 14px rgba(0,0,0,0.4);
    opacity: 0;
    transition: opacity 0.25s;
    pointer-events: none;
    z-index: 999;
    font-family: "JetBrains Mono", monospace;
  }
  #toast.show { opacity: 1; }
</style>
</head>
<body>
<div class="container">
  <!-- Header -->
  <header>
    <div class="header-left">
      <div class="logo-title">AquaTEC Control</div>
      <div class="dev-tag">V.1.6 DEV / MATLAB</div>
    </div>
    <div style="display:flex; gap:10px; align-items:center;">
      <a href="/" class="btn-back">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
        Dashboard Utama
      </a>
    </div>
  </header>

  <!-- Live Status & Safety State Diagnostics -->
  <div class="grid-2">
    <!-- Card 1: Safety State Machine & Timers -->
    <div class="card">
      <div class="card-title">
        <span>State Machine Proteksi</span>
        <span id="badgeSafety" class="badge badge-normal">NORMAL</span>
      </div>
      <div class="info-row">
        <span>Kritis dalam Window 30 Min:</span>
        <span id="txtCritCount" class="info-val">0 / 3</span>
      </div>
      <div class="info-row">
        <span>Durasi di WARNING:</span>
        <span id="txtWarnDur" class="info-val">0 s</span>
      </div>
      <div class="info-row">
        <span>Durasi di CRITICAL:</span>
        <span id="txtCritDur" class="info-val">0 s</span>
      </div>
      <div class="info-row">
        <span>Grace Period Heatsink:</span>
        <span id="txtHsGrace" class="info-val">NON-AKTIF</span>
      </div>
      <div class="info-row">
        <span>Grace Period Suhu Air:</span>
        <span id="txtWaterGrace" class="info-val">NON-AKTIF</span>
      </div>
      <div class="info-row">
        <span>Status Simulasi:</span>
        <span id="txtSimStatus" class="info-val">SENSOR FISIK</span>
      </div>
    </div>

    <!-- Card 2: Live Sensor & Actuator Diagnostics (Clean White Text) -->
    <div class="card">
      <div class="card-title">
        <span>Telemetri Sistem Aktif</span>
        <span id="txtUptime" style="font-size:0.75rem; font-family:'JetBrains Mono'; color:var(--text-muted);">Uptime: 0s</span>
      </div>
      <div class="info-row">
        <span>Suhu Air (DS18B20 #1):</span>
        <span id="txtWaterTemp" class="info-val">--.- °C</span>
      </div>
      <div class="info-row">
        <span>Suhu Heatsink (DS18B20 #2):</span>
        <span id="txtHsTemp" class="info-val">--.- °C</span>
      </div>
      <div class="info-row">
        <span>Setpoint Suhu Target:</span>
        <span id="txtSetpoint" class="info-val">--.- °C</span>
      </div>
      <div class="info-row">
        <span>Kalkulasi PID murni:</span>
        <span id="txtPid" class="info-val">0.0 %</span>
      </div>
      <div class="info-row">
        <span>Daya Peltier PWM:</span>
        <span id="txtPeltier" class="info-val">0.0 %</span>
      </div>
      <div class="info-row">
        <span>Sirkulasi Pompa 5V:</span>
        <span id="txtPump" class="info-val">OFF</span>
      </div>
    </div>
  </div>

  <!-- MATLAB Figure Section (Pure White Canvas, MATLAB Publication Styling) -->
  <div class="matlab-figure-card">
    <div class="matlab-toolbar">
      <div class="series-toggles">
        <label class="series-toggle-item">
          <input type="checkbox" id="chkWater" checked onchange="renderChart()">
          <span class="matlab-color-legend" style="background:var(--matlab-blue);"></span> Water Temp
        </label>
        <label class="series-toggle-item">
          <input type="checkbox" id="chkHs" checked onchange="renderChart()">
          <span class="matlab-color-legend" style="background:var(--matlab-orange);"></span> Heatsink Temp
        </label>
        <label class="series-toggle-item">
          <input type="checkbox" id="chkSp" checked onchange="renderChart()">
          <span class="matlab-color-legend" style="background:var(--matlab-green); height:2px; border-bottom:1px dashed #000;"></span> Setpoint
        </label>
        <label class="series-toggle-item">
          <input type="checkbox" id="chkPeltier" checked onchange="renderChart()">
          <span class="matlab-color-legend" style="background:var(--matlab-cyan);"></span> Peltier Power (%)
        </label>
        <label class="series-toggle-item">
          <input type="checkbox" id="chkPid" onchange="renderChart()">
          <span class="matlab-color-legend" style="background:var(--matlab-purple);"></span> PID Output (%)
        </label>
        <label class="series-toggle-item">
          <input type="checkbox" id="chkThresh" checked onchange="renderChart()">
          <span class="matlab-color-legend" style="background:var(--matlab-darkred); height:2px; border-bottom:1px dashed #000;"></span> Safety Limits
        </label>
      </div>

      <div style="display:flex; gap:8px; align-items:center;">
        <select id="selInterval" onchange="updateAutoRefresh()" style="padding:4px 8px;">
          <option value="5000">Auto: 5s</option>
          <option value="10000" selected>Auto: 10s</option>
          <option value="30000">Auto: 30s</option>
          <option value="0">Manual</option>
        </select>
        <button class="btn-secondary" onclick="fetchData()" title="Segarkan Data Sekarang">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M23 4v6h-6M1 20v-6h6"/><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/></svg>
        </button>
        <button class="btn-matlab" onclick="downloadMatlabPNG()" title="Ekspor Figure MATLAB Resolusi Tinggi (PNG)">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>
          Export Figure (PNG)
        </button>
        <button class="btn-secondary" onclick="downloadCSV()" title="Unduh Data Mentah Telemetri (.csv)">
          Export CSV
        </button>
      </div>
    </div>

    <!-- MATLAB Figure Window Frame -->
    <div class="matlab-window-frame">
      <div class="matlab-window-header">
        <div class="dots">
          <span class="dot" style="background:#ff5f56;"></span>
          <span class="dot" style="background:#ffbd2e;"></span>
          <span class="dot" style="background:#27c93f;"></span>
        </div>
        <div style="font-weight:600; color:#333;">Figure 1: AquaTEC System Telemetry & Protection Analysis</div>
        <div style="font-family:'JetBrains Mono'; font-size:0.7rem; color:#777;">MATLAB R2024b Style</div>
      </div>
      <div class="chart-container" id="chartWrapper">
        <canvas id="chartCanvas"></canvas>
      </div>
    </div>

    <div style="display:flex; justify-content:space-between; margin-top:8px; font-size:0.75rem; color:var(--text-muted); font-family:'JetBrains Mono';">
      <span>Rendering Style: MATLAB 2D Line Plot ('box on', 'grid on', TickDir 'in')</span>
      <span id="txtPointsCount">360 Data Points (1-Hour History Buffer)</span>
    </div>
  </div>

  <!-- Simulation Workbench & Quick Presets -->
  <div class="grid-2">
    <!-- Panel 1: Injeksi Parameter Simulasi Bebas -->
    <div class="card">
      <div class="card-title">Workbench Simulasi Sensor</div>

      <!-- Heatsink simulation -->
      <div class="form-row">
        <div class="form-label">Simulasi Suhu HS:</div>
        <input type="number" id="inpSimHs" step="0.5" placeholder="Contoh: 66.0" style="width:110px;">
        <button onclick="applySimHs(true)">Injeksi</button>
        <button class="btn-secondary" onclick="applySimHs(false)">Batal</button>
      </div>

      <!-- Water simulation (Bebas tanpa batasan 20-30°C) -->
      <div class="form-row">
        <div class="form-label">Simulasi Suhu Air:</div>
        <input type="number" id="inpSimWater" step="0.5" placeholder="Contoh: 18.0 / 35.0" style="width:110px;">
        <button onclick="applySimWater(true)">Injeksi</button>
        <button class="btn-secondary" onclick="applySimWater(false)">Batal</button>
      </div>

      <!-- Sensor Failure Injection -->
      <div style="margin-top:14px; padding-top:10px; border-top:1px solid rgba(255,255,255,0.05);">
        <div style="font-size:0.8rem; color:var(--text-muted); margin-bottom:8px;">Simulasi Kerusakan / Lepas Kabel Sensor:</div>
        <div style="display:flex; gap:16px; margin-bottom:10px; font-size:0.84rem;">
          <label style="display:flex; align-items:center; gap:6px;">
            <input type="checkbox" id="chkFailHs"> Putus Heatsink (-127°C)
          </label>
          <label style="display:flex; align-items:center; gap:6px;">
            <input type="checkbox" id="chkFailWater"> Putus Air (-127°C)
          </label>
        </div>
        <div style="display:flex; gap:8px;">
          <button class="btn-secondary" onclick="applySensorFail()">Terapkan Error Sensor</button>
          <button class="btn-fault" onclick="resetAllSim()">Reset Semua Simulasi</button>
        </div>
      </div>
    </div>

    <!-- Panel 2: Quick Test Skenario Proteksi (1-Click, Netral kecuali FAULT) -->
    <div class="card">
      <div class="card-title">Quick Test Skenario Proteksi</div>
      <div class="quick-grid">
        <button class="quick-btn btn-secondary" onclick="quickTest(1)">
          <span>Uji WARNING (HS 66°C)</span>
          <span class="subtext">Peltier max 70%, Pompa OFF</span>
        </button>

        <button class="quick-btn btn-secondary" onclick="quickTest(2)">
          <span>Uji CRITICAL (HS 72°C)</span>
          <span class="subtext">Peltier OFF, Pompa OFF</span>
        </button>

        <button class="quick-btn btn-fault" onclick="quickTest(3)">
          <span>Force FAULT Instant</span>
          <span class="subtext">Latching trip seketika</span>
        </button>

        <button class="quick-btn btn-secondary" onclick="quickTest(4)">
          <span>Reset Safety & Sensor</span>
          <span class="subtext">Pulihkan ke sensor fisik & NORMAL</span>
        </button>

        <button class="quick-btn btn-secondary" onclick="quickTest(5)">
          <span>Uji Air Dingin (18°C)</span>
          <span class="subtext">Fail-safe Peltier OFF beku</span>
        </button>

        <button class="quick-btn btn-secondary" onclick="quickTest(6)">
          <span>Uji Air Panas (34°C)</span>
          <span class="subtext">Alarm suhu air tinggi</span>
        </button>
      </div>
    </div>
  </div>
</div>

<div id="toast">Operasi Berhasil</div>

<script>
// Global Data State
let historyData = {
  water: [],
  heatsink: [],
  peltier: [],
  pid: [],
  safety: []
};
let liveStatus = {};
let autoRefreshTimer = null;
let hoveredIndex = -1;

function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2500);
}

function formatTimeAgo(secondsAgo) {
  if (secondsAgo === 0) return '0s';
  if (secondsAgo < 60) return `-${secondsAgo}s`;
  const m = Math.floor(secondsAgo / 60);
  const s = secondsAgo % 60;
  return s > 0 ? `-${m}m ${s}s` : `-${m}m`;
}

async function fetchData() {
  try {
    const resHist = await fetch('/api/dev/history/full');
    if (resHist.ok) {
      historyData = await resHist.json();
      document.getElementById('txtPointsCount').textContent = `${historyData.water.length} Data Points (Sampling 10s)`;
      renderChart();
    }

    const resState = await fetch('/api/dev/state');
    if (resState.ok) {
      liveStatus = await resState.json();
      updateDiagnosticsUI();
    }
  } catch (e) {
    console.error('Fetch error:', e);
  }
}

function updateDiagnosticsUI() {
  const s = liveStatus;
  if (!s) return;

  document.getElementById('txtWaterTemp').textContent = (s.water_temp != null) ? `${s.water_temp.toFixed(1)} °C` : '--.- °C';
  document.getElementById('txtHsTemp').textContent = (s.heatsink_temp != null) ? `${s.heatsink_temp.toFixed(1)} °C` : '--.- °C';
  document.getElementById('txtSetpoint').textContent = (s.setpoint != null) ? `${s.setpoint.toFixed(1)} °C` : '--.- °C';
  document.getElementById('txtPid').textContent = (s.pid_output != null) ? `${s.pid_output.toFixed(1)} %` : '0.0 %';
  document.getElementById('txtPeltier').textContent = (s.peltier_pct != null) ? `${s.peltier_pct.toFixed(1)} %` : '0.0 %';
  document.getElementById('txtPump').textContent = s.pump ? 'AKTIF (ON)' : 'MATI (OFF)';
  document.getElementById('txtUptime').textContent = `Uptime: ${s.uptime_s || 0}s`;

  const b = document.getElementById('badgeSafety');
  const lvl = s.safety || 'NORMAL';
  b.textContent = lvl;
  b.className = 'badge ' + (
    lvl === 'NORMAL' ? 'badge-normal' :
    lvl === 'WARNING' ? 'badge-warning' :
    lvl === 'CRITICAL' ? 'badge-critical' : 'badge-fault'
  );

  document.getElementById('txtCritCount').textContent = `${s.critical_count_window || 0} / 3`;
  document.getElementById('txtWarnDur').textContent = `${s.warning_dur_s || 0} s`;
  document.getElementById('txtCritDur').textContent = `${s.critical_dur_s || 0} s`;
  document.getElementById('txtHsGrace').textContent = s.heatsink_grace ? `AKTIF (${s.heatsink_grace_rem}s)` : 'NON-AKTIF';
  document.getElementById('txtWaterGrace').textContent = s.water_grace ? `AKTIF (${s.water_grace_rem}s)` : 'NON-AKTIF';

  let simTxt = [];
  if (s.sim_hs_enabled) simTxt.push(`HS=${s.sim_hs_temp.toFixed(1)}°C`);
  if (s.sim_water_enabled) simTxt.push(`Air=${s.sim_water_temp.toFixed(1)}°C`);
  if (s.sim_hs_fail) simTxt.push('HS Fail');
  if (s.sim_water_fail) simTxt.push('Air Fail');
  
  const elSim = document.getElementById('txtSimStatus');
  if (simTxt.length > 0) {
    elSim.textContent = 'SIMULASI: ' + simTxt.join(' | ');
  } else {
    elSim.textContent = 'SENSOR FISIK';
  }
}

// =============================================================================
// MATLAB ENGINE 2D LINE PLOT RENDERER
// Clean White Background, Box On, Inward Ticks, Dotted Grid, Legend Box
// =============================================================================
function drawMatlabPlot(ctx, W, H, isExport) {
  // 1. Clear & Background Figure (Pure MATLAB White #FFFFFF)
  ctx.fillStyle = '#FFFFFF';
  ctx.fillRect(0, 0, W, H);

  // Padding proporsional khas MATLAB
  const padLeft = isExport ? 80 : 64;
  const padRight = isExport ? 80 : 64;
  const padTop = isExport ? 60 : 42;
  const padBottom = isExport ? 64 : 48;
  const plotW = W - padLeft - padRight;
  const plotH = H - padTop - padBottom;

  // 2. Axes Background (Pure White)
  ctx.fillStyle = '#FFFFFF';
  ctx.fillRect(padLeft, padTop, plotW, plotH);

  // Range Sumbu
  const minTemp = 10.0;
  const maxTemp = 80.0;
  const minPct = 0.0;
  const maxPct = 100.0;

  // 3. Gridlines (MATLAB style 'grid on': subtle dotted lines #D0D0D0)
  ctx.save();
  ctx.strokeStyle = '#D8D8D8';
  ctx.lineWidth = isExport ? 1.0 : 0.8;
  ctx.setLineDash([2, 3]);

  const numYDivs = 7;
  for (let i = 0; i <= numYDivs; i++) {
    const yPx = padTop + plotH - (i / numYDivs) * plotH;
    ctx.beginPath();
    ctx.moveTo(padLeft, yPx);
    ctx.lineTo(padLeft + plotW, yPx);
    ctx.stroke();
  }

  const numXDivs = 6;
  for (let j = 0; j <= numXDivs; j++) {
    const xPx = padLeft + (j / numXDivs) * plotW;
    ctx.beginPath();
    ctx.moveTo(xPx, padTop);
    ctx.lineTo(xPx, padTop + plotH);
    ctx.stroke();
  }
  ctx.restore();

  // 4. Threshold Lines (Safety Boundaries)
  if (document.getElementById('chkThresh').checked) {
    // HS Critical 70°C: MATLAB Dark Red #A2142F
    drawMatlabThreshold(ctx, 70.0, '#A2142F', [5, 4], 'Critical Threshold (70 °C)', padLeft, padTop, plotW, plotH, minTemp, maxTemp, isExport);
    // HS Warning 60°C: MATLAB Gold/Yellow #EDB120
    drawMatlabThreshold(ctx, 60.0, '#D95319', [4, 4], 'Warning Threshold (60 °C)', padLeft, padTop, plotW, plotH, minTemp, maxTemp, isExport);
    // Water Min Safe 20°C: MATLAB Blue #0072BD
    drawMatlabThreshold(ctx, 20.0, '#0072BD', [3, 4], 'Min Safe Water (20 °C)', padLeft, padTop, plotW, plotH, minTemp, maxTemp, isExport);
  }

  // Setpoint Line: MATLAB Green #77AC30
  if (document.getElementById('chkSp').checked && liveStatus.setpoint) {
    drawMatlabThreshold(ctx, liveStatus.setpoint, '#77AC30', [8, 3, 2, 3], `Setpoint (${liveStatus.setpoint.toFixed(1)} °C)`, padLeft, padTop, plotW, plotH, minTemp, maxTemp, isExport);
  }

  const n = historyData.water ? historyData.water.length : 0;

  // 5. Kurva Data (MATLAB Default Color Order)
  if (n >= 2) {
    // a. PID Output % (MATLAB Purple #7E2F8E)
    if (document.getElementById('chkPid').checked && historyData.pid) {
      drawMatlabSeries(ctx, historyData.pid, minPct, maxPct, '#7E2F8E', isExport ? 2.0 : 1.5, [], padLeft, padTop, plotW, plotH);
    }
    // b. Peltier Power % (MATLAB Cyan #4DBEEE)
    if (document.getElementById('chkPeltier').checked && historyData.peltier) {
      drawMatlabSeries(ctx, historyData.peltier, minPct, maxPct, '#4DBEEE', isExport ? 2.5 : 1.8, [], padLeft, padTop, plotW, plotH);
    }
    // c. Suhu Heatsink (MATLAB Orange #D95319)
    if (document.getElementById('chkHs').checked && historyData.heatsink) {
      drawMatlabSeries(ctx, historyData.heatsink, minTemp, maxTemp, '#D95319', isExport ? 3.0 : 2.2, [], padLeft, padTop, plotW, plotH);
    }
    // d. Suhu Air (MATLAB Blue #0072BD)
    if (document.getElementById('chkWater').checked && historyData.water) {
      drawMatlabSeries(ctx, historyData.water, minTemp, maxTemp, '#0072BD', isExport ? 3.2 : 2.4, [], padLeft, padTop, plotW, plotH);
    }
  } else {
    ctx.fillStyle = '#666666';
    ctx.textAlign = 'center';
    ctx.font = `${isExport ? 18 : 13}px "Helvetica Neue", Arial, sans-serif`;
    ctx.fillText('Waiting for telemetry buffer stream...', padLeft + plotW / 2, padTop + plotH / 2);
  }

  // 6. MATLAB 'box on': Solid Black Frame around Plot (Top, Bottom, Left, Right)
  ctx.strokeStyle = '#111111';
  ctx.lineWidth = isExport ? 1.5 : 1.2;
  ctx.strokeRect(padLeft, padTop, plotW, plotH);

  // 7. MATLAB Ticks (TickDir: 'in')
  const tickLen = isExport ? 7 : 5;
  ctx.strokeStyle = '#111111';
  ctx.lineWidth = isExport ? 1.4 : 1.0;
  ctx.fillStyle = '#111111';
  ctx.font = `${isExport ? 13 : 10.5}px "Helvetica Neue", Arial, sans-serif`;

  // Y-Ticks (Left & Right Ticks in)
  for (let i = 0; i <= numYDivs; i++) {
    const fraction = i / numYDivs;
    const yPx = padTop + plotH - fraction * plotH;
    const yTemp = minTemp + fraction * (maxTemp - minTemp);
    const yPct = minPct + fraction * (maxPct - minPct);

    // Left tick (inward)
    ctx.beginPath();
    ctx.moveTo(padLeft, yPx);
    ctx.lineTo(padLeft + tickLen, yPx);
    ctx.stroke();

    // Right tick (inward)
    ctx.beginPath();
    ctx.moveTo(padLeft + plotW, yPx);
    ctx.lineTo(padLeft + plotW - tickLen, yPx);
    ctx.stroke();

    // Left label: Temperature (°C)
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    ctx.fillText(`${yTemp.toFixed(0)}`, padLeft - 6, yPx);

    // Right label: Actuator (%)
    ctx.textAlign = 'left';
    ctx.fillText(`${yPct.toFixed(0)}%`, padLeft + plotW + 6, yPx);
  }

  // X-Ticks & Labels (Bottom & Top Ticks in)
  for (let j = 0; j <= numXDivs; j++) {
    const fraction = j / numXDivs;
    const xPx = padLeft + fraction * plotW;

    // Bottom tick (inward)
    ctx.beginPath();
    ctx.moveTo(xPx, padTop + plotH);
    ctx.lineTo(xPx, padTop + plotH - tickLen);
    ctx.stroke();

    // Top tick (inward)
    ctx.beginPath();
    ctx.moveTo(xPx, padTop);
    ctx.lineTo(xPx, padTop + tickLen);
    ctx.stroke();

    // X Label
    const totalSec = (n > 1) ? (n - 1) * 10 : 3600;
    const secAgo = Math.round((1 - fraction) * totalSec);
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    ctx.fillText(formatTimeAgo(secAgo), xPx, padTop + plotH + (isExport ? 7 : 5));
  }

  // 8. MATLAB Axis Labels & Title
  // Figure Title
  ctx.fillStyle = '#000000';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'bottom';
  ctx.font = `bold ${isExport ? 16 : 12.5}px "Helvetica Neue", Arial, sans-serif`;
  ctx.fillText('AquaTEC System Telemetry & Protection Analysis', padLeft + plotW / 2, padTop - (isExport ? 14 : 9));

  // X Axis Label
  ctx.font = `${isExport ? 13 : 11}px "Helvetica Neue", Arial, sans-serif`;
  ctx.fillText('Time (seconds ago)', padLeft + plotW / 2, padTop + plotH + (isExport ? 36 : 28));

  // Y Left Axis Label (Rotated 90 deg)
  ctx.save();
  ctx.translate(padLeft - (isExport ? 45 : 36), padTop + plotH / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = 'center';
  ctx.textBaseline = 'bottom';
  ctx.font = `${isExport ? 13 : 11}px "Helvetica Neue", Arial, sans-serif`;
  ctx.fillText('Temperature (°C)', 0, 0);
  ctx.restore();

  // Y Right Axis Label (Rotated 90 deg)
  ctx.save();
  ctx.translate(padLeft + plotW + (isExport ? 48 : 40), padTop + plotH / 2);
  ctx.rotate(Math.PI / 2);
  ctx.textAlign = 'center';
  ctx.textBaseline = 'bottom';
  ctx.font = `${isExport ? 13 : 11}px "Helvetica Neue", Arial, sans-serif`;
  ctx.fillText('Actuator Output (%)', 0, 0);
  ctx.restore();

  // 9. MATLAB Legend Box (Inside Plot, Upper Right, White box with border)
  drawMatlabLegend(ctx, padLeft, padTop, plotW, plotH, isExport);

  // 10. Data Cursor (Hover effect if user moves mouse over canvas)
  if (!isExport && hoveredIndex >= 0 && n > 0 && hoveredIndex < n) {
    const xPx = padLeft + (hoveredIndex / (n - 1)) * plotW;
    ctx.save();
    ctx.strokeStyle = '#333333';
    ctx.lineWidth = 1;
    ctx.setLineDash([2, 2]);
    ctx.beginPath();
    ctx.moveTo(xPx, padTop);
    ctx.lineTo(xPx, padTop + plotH);
    ctx.stroke();

    // Data Cursor Box
    const wVal = historyData.water[hoveredIndex];
    const hsVal = historyData.heatsink[hoveredIndex];
    const cursorTxt = `W: ${wVal.toFixed(1)}°C | HS: ${hsVal.toFixed(1)}°C`;
    ctx.font = '10px "JetBrains Mono", monospace';
    const txtW = ctx.measureText(cursorTxt).width + 12;
    const boxX = (xPx + txtW + 10 > padLeft + plotW) ? (xPx - txtW - 8) : (xPx + 8);
    ctx.fillStyle = 'rgba(255, 255, 255, 0.95)';
    ctx.strokeStyle = '#666666';
    ctx.setLineDash([]);
    ctx.fillRect(boxX, padTop + 10, txtW, 20);
    ctx.strokeRect(boxX, padTop + 10, txtW, 20);
    ctx.fillStyle = '#000000';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(cursorTxt, boxX + 6, padTop + 20);
    ctx.restore();
  }
}

function drawMatlabThreshold(ctx, val, color, dash, label, padLeft, padTop, plotW, plotH, minVal, maxVal, isExport) {
  if (val < minVal || val > maxVal) return;
  const yPx = padTop + plotH - ((val - minVal) / (maxVal - minVal)) * plotH;
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = isExport ? 1.5 : 1.1;
  ctx.setLineDash(dash);
  ctx.beginPath();
  ctx.moveTo(padLeft, yPx);
  ctx.lineTo(padLeft + plotW, yPx);
  ctx.stroke();

  // Small label near right edge
  ctx.fillStyle = color;
  ctx.font = `${isExport ? 11 : 9.5}px "Helvetica Neue", Arial, sans-serif`;
  ctx.textAlign = 'right';
  ctx.textBaseline = 'bottom';
  ctx.fillText(label, padLeft + plotW - 6, yPx - 2);
  ctx.restore();
}

function drawMatlabSeries(ctx, data, minVal, maxVal, color, lineWidth, dash, padLeft, padTop, plotW, plotH) {
  const n = data.length;
  if (n < 2) return;

  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth;
  ctx.setLineDash(dash);
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';

  ctx.beginPath();
  for (let i = 0; i < n; i++) {
    const x = padLeft + (i / (n - 1)) * plotW;
    const clampedVal = Math.max(minVal, Math.min(maxVal, data[i]));
    const y = padTop + plotH - ((clampedVal - minVal) / (maxVal - minVal)) * plotH;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
  ctx.restore();
}

// MATLAB Legend Box ('legend')
function drawMatlabLegend(ctx, padLeft, padTop, plotW, plotH, isExport) {
  const legendItems = [];
  if (document.getElementById('chkWater').checked) legendItems.push({ label: 'Water Temp', color: '#0072BD', dash: [] });
  if (document.getElementById('chkHs').checked) legendItems.push({ label: 'Heatsink Temp', color: '#D95319', dash: [] });
  if (document.getElementById('chkSp').checked) legendItems.push({ label: 'Setpoint', color: '#77AC30', dash: [4, 2] });
  if (document.getElementById('chkPeltier').checked) legendItems.push({ label: 'Peltier (%)', color: '#4DBEEE', dash: [] });
  if (document.getElementById('chkPid').checked) legendItems.push({ label: 'PID Output (%)', color: '#7E2F8E', dash: [] });

  if (legendItems.length === 0) return;

  const itemHeight = isExport ? 18 : 14;
  const lineLen = isExport ? 22 : 16;
  const boxPadding = isExport ? 10 : 7;
  const legendW = isExport ? 165 : 125;
  const legendH = legendItems.length * itemHeight + boxPadding * 2;
  const boxX = padLeft + plotW - legendW - 10;
  const boxY = padTop + 10;

  ctx.save();
  // Legend Background & Frame (White with subtle shadow)
  ctx.fillStyle = '#FFFFFF';
  ctx.strokeStyle = '#B0B0B0';
  ctx.lineWidth = 1;
  ctx.fillRect(boxX, boxY, legendW, legendH);
  ctx.strokeRect(boxX, boxY, legendW, legendH);

  ctx.font = `${isExport ? 11 : 9.5}px "Helvetica Neue", Arial, sans-serif`;
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';

  for (let i = 0; i < legendItems.length; i++) {
    const item = legendItems[i];
    const yCenter = boxY + boxPadding + i * itemHeight + itemHeight / 2;

    // Line sample
    ctx.strokeStyle = item.color;
    ctx.lineWidth = isExport ? 2.2 : 1.8;
    ctx.setLineDash(item.dash);
    ctx.beginPath();
    ctx.moveTo(boxX + boxPadding, yCenter);
    ctx.lineTo(boxX + boxPadding + lineLen, yCenter);
    ctx.stroke();

    // Text label
    ctx.fillStyle = '#111111';
    ctx.setLineDash([]);
    ctx.fillText(item.label, boxX + boxPadding + lineLen + 6, yCenter);
  }
  ctx.restore();
}

// Render on Screen Canvas
function renderChart() {
  const canvas = document.getElementById('chartCanvas');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();

  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;

  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);

  drawMatlabPlot(ctx, rect.width, rect.height, false);
}

// Ekspor Figure MATLAB Resolusi Tinggi (300 DPI Publication Quality)
function downloadMatlabPNG() {
  const exportCanvas = document.createElement('canvas');
  // Standar dimensi publikasi MATLAB (1920 x 1080 Full HD)
  const expW = 1920;
  const expH = 1080;
  exportCanvas.width = expW;
  exportCanvas.height = expH;

  const ctx = exportCanvas.getContext('2d');
  drawMatlabPlot(ctx, expW, expH, true);

  const link = document.createElement('a');
  link.download = `AquaTEC_MATLAB_Telemetry_${Date.now()}.png`;
  link.href = exportCanvas.toDataURL('image/png');
  link.click();
  showToast('Figure MATLAB PNG (300 DPI) berhasil diunduh');
}

function downloadCSV() {
  window.location.href = '/api/dev/export/csv';
}

function updateAutoRefresh() {
  if (autoRefreshTimer) clearInterval(autoRefreshTimer);
  const interval = parseInt(document.getElementById('selInterval').value, 10);
  if (interval > 0) {
    autoRefreshTimer = setInterval(fetchData, interval);
  }
}

// Mouse Interactivity (Data Cursor)
const chartCanvas = document.getElementById('chartCanvas');
chartCanvas.addEventListener('mousemove', (e) => {
  const rect = chartCanvas.getBoundingClientRect();
  const mouseX = e.clientX - rect.left;
  const padLeft = 64;
  const padRight = 64;
  const plotW = rect.width - padLeft - padRight;
  const n = historyData.water ? historyData.water.length : 0;

  if (n > 1 && mouseX >= padLeft && mouseX <= padLeft + plotW) {
    const fraction = (mouseX - padLeft) / plotW;
    hoveredIndex = Math.round(fraction * (n - 1));
  } else {
    hoveredIndex = -1;
  }
  renderChart();
});

chartCanvas.addEventListener('mouseleave', () => {
  hoveredIndex = -1;
  renderChart();
});

// API Actions
async function applySimHs(enable) {
  const inp = document.getElementById('inpSimHs');
  const temp = enable ? parseFloat(inp.value) : 25.0;
  if (enable && isNaN(temp)) {
    showToast('Masukkan nilai suhu valid!');
    return;
  }
  await fetch('/api/dev/sim/heatsink', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: enable, temp: temp })
  });
  showToast(enable ? `Simulasi Heatsink: ${temp}°C` : 'Simulasi Heatsink DIMATIKAN');
  fetchData();
}

async function applySimWater(enable) {
  const inp = document.getElementById('inpSimWater');
  const temp = enable ? parseFloat(inp.value) : 25.0;
  if (enable && isNaN(temp)) {
    showToast('Masukkan nilai suhu valid!');
    return;
  }
  await fetch('/api/dev/sim/water', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: enable, temp: temp })
  });
  showToast(enable ? `Simulasi Air: ${temp}°C` : 'Simulasi Air DIMATIKAN');
  fetchData();
}

async function applySensorFail() {
  const failHs = document.getElementById('chkFailHs').checked;
  const failWater = document.getElementById('chkFailWater').checked;
  await fetch('/api/dev/sim/sensor-fail', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ heatsink: failHs, water: failWater })
  });
  showToast(`Simulasi Error Sensor (HS:${failHs}, Air:${failWater})`);
  fetchData();
}

async function resetAllSim() {
  document.getElementById('chkFailHs').checked = false;
  document.getElementById('chkFailWater').checked = false;
  document.getElementById('inpSimHs').value = '';
  document.getElementById('inpSimWater').value = '';

  await fetch('/api/dev/sim/reset', { method: 'POST' });
  showToast('Semua simulasi dimatikan. Sensor fisik aktif.');
  fetchData();
}

async function quickTest(id) {
  switch (id) {
    case 1:
      await fetch('/api/dev/sim/heatsink', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: true, temp: 66.0 })
      });
      showToast('Uji WARNING (HS = 66.0°C)');
      break;
    case 2:
      await fetch('/api/dev/sim/heatsink', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: true, temp: 72.0 })
      });
      showToast('Uji CRITICAL (HS = 72.0°C)');
      break;
    case 3:
      await fetch('/api/dev/safety/force', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ level: 'FAULT' })
      });
      showToast('Status FAULT Instan Dipicu!');
      break;
    case 4:
      // Reset State Safety ke NORMAL dan matikan semua simulasi sensor (kembali ke sensor fisik)
      await fetch('/api/dev/sim/reset', { method: 'POST' });
      await fetch('/api/dev/safety/force', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ level: 'NORMAL' })
      });
      document.getElementById('chkFailHs').checked = false;
      document.getElementById('chkFailWater').checked = false;
      document.getElementById('inpSimHs').value = '';
      document.getElementById('inpSimWater').value = '';
      showToast('Safety & Sensor Simulasi Dipulihkan ke Sensor Fisik');
      break;
    case 5:
      await fetch('/api/dev/sim/water', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: true, temp: 18.0 })
      });
      showToast('Uji Air Dingin 18.0°C (Proteksi Beku)');
      break;
    case 6:
      await fetch('/api/dev/sim/water', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: true, temp: 34.0 })
      });
      showToast('Uji Air Panas 34.0°C (Alarm Suhu Air)');
      break;
  }
  fetchData();
}

window.addEventListener('DOMContentLoaded', () => {
  fetchData();
  updateAutoRefresh();
  window.addEventListener('resize', renderChart);
});
</script>
</body>
</html>
)rawliteral";

#endif // DEVMODE_HTML_H
