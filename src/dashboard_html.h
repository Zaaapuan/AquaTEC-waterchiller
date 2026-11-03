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

  .control-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    margin-bottom: 20px;
  }
  @media (max-width: 600px) {
    .control-grid { grid-template-columns: 1fr; }
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

  .chart-card {
    background: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 12px;
    padding: 16px;
    margin-bottom: 20px;
  }
  canvas {
    width: 100%;
    height: 220px;
    display: block;
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
      <span class="tag">V.1.3</span>
    </div>
    <div id="safetyBadge" class="badge badge-normal">NORMAL</div>
  </header>

  <div id="alertBanner" class="alert-banner"></div>

  <!-- Metric Cards -->
  <div class="grid">
    <div class="card">
      <div class="card-title">Suhu Air Akuarium</div>
      <div class="card-value"><span id="waterTemp">--.-</span><span class="card-unit">°C</span></div>
    </div>
    <div class="card">
      <div class="card-title">Suhu Heatsink</div>
      <div class="card-value"><span id="hsTemp">--.-</span><span class="card-unit">°C</span></div>
    </div>
    <div class="card">
      <div class="card-title">Target (Set Point)</div>
      <div class="card-value"><span id="setpointVal">--.-</span><span class="card-unit">°C</span></div>
    </div>
    <div class="card">
      <div class="card-title">Daya Peltier (PWM)</div>
      <div class="card-value"><span id="peltierPct">0</span><span class="card-unit">%</span></div>
      <div class="card-sub" id="peltierDetail">PID: 0%</div>
    </div>
    <div class="card">
      <div class="card-title">Sirkulasi Pompa</div>
      <div class="card-value"><span id="pumpStatus">OFF</span></div>
    </div>
  </div>

  <!-- Control Cards -->
  <div class="control-grid">
    <div class="card">
      <div class="form-group">
        <label for="spInput">Set Point: Target Suhu Air (°C)</label>
        <div class="input-row">
          <input type="number" id="spInput" min="20.0" max="30.0" step="0.5" value="28.0">
          <button id="btnSaveSp" onclick="saveSetpoint()">Simpan</button>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Kontrol Sistem</div>
      <div style="display:flex; gap:10px; flex-wrap:wrap; margin-top:10px;">
        <button id="btnPower" onclick="togglePower()" style="flex:1;">Sistem: ON</button>
        <button id="btnResetFault" onclick="resetFault()" class="btn-danger" style="display:none; flex:1;">Reset FAULT</button>
      </div>
      <div class="card-sub" id="uptimeText" style="margin-top:12px;">Uptime: 0m</div>
    </div>
  </div>

  <!-- Live Canvas Chart -->
  <div class="chart-card">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px;">
      <div class="card-title" style="margin-bottom:0;">Grafik Suhu Air 1 Jam Terakhir</div>
      <span class="card-sub">Sampling tiap 10 detik (360 titik)</span>
    </div>
    <canvas id="historyCanvas"></canvas>
  </div>

  <footer>
    AquaTEC Embedded System
  </footer>
</div>

<script>
let currentEnabled = true;
let chartData = [];

function updateStatus() {
  fetch('/api/status')
    .then(res => res.json())
    .then(data => {
      document.getElementById('waterTemp').innerText = data.water_temp.toFixed(1);
      document.getElementById('hsTemp').innerText = data.heatsink_temp.toFixed(1);
      document.getElementById('setpointVal').innerText = data.setpoint.toFixed(1);
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
        btnPower.className = '';
      } else {
        btnPower.innerText = 'NONAKTIF';
        btnPower.className = 'btn-off';
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
  const val = parseFloat(document.getElementById('spInput').value);
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
      updateStatus();
    } else {
      alert('Gagal menyimpan setpoint. Nilai di luar rentang.');
    }
  });
}

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

function loadHistory() {
  fetch('/api/history')
    .then(res => res.json())
    .then(data => {
      chartData = data;
      drawChart();
    })
    .catch(err => console.error('Gagal mengambil riwayat suhu:', err));
}

function drawChart() {
  const canvas = document.getElementById('historyCanvas');
  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  const w = rect.width;
  const h = rect.height;
  const padding = { top: 20, right: 20, bottom: 30, left: 45 };

  ctx.clearRect(0, 0, 0, 0);

  if (!chartData || chartData.length < 2) {
    ctx.fillStyle = '#9aa0a6';
    ctx.font = '12px Inter, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Mengumpulkan data riwayat suhu...', w / 2, h / 2);
    return;
  }

  // Tentukan batas sumbu Y
  let minT = 18;
  let maxT = 34;
  for (let val of chartData) {
    if (val < minT) minT = Math.floor(val) - 1;
    if (val > maxT) maxT = Math.ceil(val) + 1;
  }

  const chartW = w - padding.left - padding.right;
  const chartH = h - padding.top - padding.bottom;

  // Garis grid & label Y
  ctx.strokeStyle = '#2e3035';
  ctx.lineWidth = 1;
  ctx.fillStyle = '#9aa0a6';
  ctx.font = '10px "JetBrains Mono", monospace';
  ctx.textAlign = 'right';

  const steps = 4;
  for (let i = 0; i <= steps; i++) {
    const yVal = minT + ((maxT - minT) * i / steps);
    const y = padding.top + chartH - (i * chartH / steps);
    ctx.beginPath();
    ctx.moveTo(padding.left, y);
    ctx.lineTo(w - padding.right, y);
    ctx.stroke();
    ctx.fillText(yVal.toFixed(1) + '°', padding.left - 8, y + 3);
  }

  // Garis data suhu
  ctx.beginPath();
  ctx.strokeStyle = '#00A67E';
  ctx.lineWidth = 2;

  const count = chartData.length;
  for (let i = 0; i < count; i++) {
    const x = padding.left + (i * chartW / (count - 1));
    const val = chartData[i];
    const y = padding.top + chartH - ((val - minT) / (maxT - minT) * chartH);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // Area gradien bawah
  const lastX = padding.left + chartW;
  const bottomY = padding.top + chartH;
  ctx.lineTo(lastX, bottomY);
  ctx.lineTo(padding.left, bottomY);
  ctx.closePath();
  const grad = ctx.createLinearGradient(0, padding.top, 0, bottomY);
  grad.addColorStop(0, 'rgba(0, 166, 126, 0.25)');
  grad.addColorStop(1, 'rgba(0, 166, 126, 0.0)');
  ctx.fillStyle = grad;
  ctx.fill();

  // Label Sumbu X
  ctx.fillStyle = '#9aa0a6';
  ctx.textAlign = 'left';
  ctx.fillText('1 jam lalu', padding.left, h - 10);
  ctx.textAlign = 'right';
  ctx.fillText('Sekarang', w - padding.right, h - 10);
}

window.addEventListener('resize', drawChart);

// Mulai Polling API
updateStatus();
loadHistory();
setInterval(updateStatus, 2000);
setInterval(loadHistory, 10000);
</script>
</body>
</html>
)rawliteral";

#endif // DASHBOARD_HTML_H

