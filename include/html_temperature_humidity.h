const char html[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Sensor Dashboard</title>
  <style>
    :root {
      --bg: #10151c;
      --panel: #1c2530;
      --panel-border: rgba(255,255,255,0.08);
      --label: #8a97a8;
      --text: #e9eef7;
      --shadow: rgba(0,0,0,0.35);
    }

    * { box-sizing: border-box; }

    body {
      font-family: -apple-system, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
      margin: 0;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }

    .dashboard {
      width: min(100%, 420px);
      display: flex;
      flex-direction: column;
      gap: 18px;
      align-items: stretch;
    }

    .panel {
      background: var(--panel);
      border: 1px solid var(--panel-border);
      border-radius: 16px;
      box-shadow: 0 4px 20px var(--shadow);
      padding: 18px 20px 16px;
      width: 100%;
      max-width: 420px;
      min-height: 320px;
      margin: 0 auto;
      display: flex;
      flex-direction: column;
      justify-content: flex-start;
      align-items: center;
    }

    .label {
      font-size: 0.9rem;
      color: var(--label);
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-bottom: 10px;
      text-align: center;
    }

    .metric-stack {
      display: flex;
      flex: 1;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      width: 100%;
    }

    .metric {
      width: 100%;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      flex: 1;
    }

    .value {
      font-size: clamp(2.2rem, 4vw, 3.3rem);
      font-weight: 600;
      margin: 8px 0;
      line-height: 1.1;
      text-align: center;
    }

    .unit {
      font-size: 1.5rem;
      color: var(--label);
    }

    .sensor-panel {
      justify-content: center;
    }

    .control-panel {
      justify-content: center;
    }

    .panel-status {
      margin-top: 10px;
      font-size: 0.8rem;
      color: #6d798c;
      min-height: 1.2em;
      text-align: center;
    }

    .color-row {
      display: flex;
      gap: 12px;
      align-items: center;
      justify-content: center;
      width: 100%;
      margin-top: 4px;
    }

    .color-wheel-wrap {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
    }

    .color-controls {
      display: flex;
      flex-direction: column;
      gap: 8px;
      align-items: center;
    }

    #colorPreview {
      width: 56px;
      height: 56px;
      border-radius: 10px;
      background: #fff;
      border: 1px solid #222;
    }

    .mini-label {
      font-size: 0.8rem;
      color: var(--label);
      text-align: center;
    }

    .action-row {
      display: flex;
      gap: 8px;
      justify-content: center;
      flex-wrap: wrap;
      margin-top: 10px;
    }

    button {
      background: #273445;
      color: var(--text);
      border: 1px solid rgba(255,255,255,0.08);
      border-radius: 8px;
      padding: 8px 10px;
      cursor: pointer;
    }

    input[type="range"] {
      width: 100%;
    }

    @media (max-width: 980px) {
      .dashboard {
        grid-template-columns: 1fr;
        width: min(100%, 360px);
      }
      .panel {
        max-width: 360px;
      }
    }
  </style>
</head>
<body>
  <div class="dashboard">
    <div class="panel sensor-panel">
      <div class="label">Temperature / Humidity</div>
      <div class="metric-stack">
        <div class="metric" id="temp-row">
          <div class="value"><span id="temp">--</span><span class="unit">&deg;C</span></div>
        </div>
        <div class="metric" id="hum-row">
          <div class="value"><span id="hum">--</span><span class="unit">%</span></div>
        </div>
      </div>
      <div id="status" class="panel-status">connecting...</div>
    </div>

    <div class="panel sensor-panel">
      <div class="label">Light</div>
      <div class="metric-stack" id="lux-row">
        <div class="metric">
          <div class="value"><span id="lux">--</span><span class="unit"> lux</span></div>
        </div>
      </div>
      <div id="lux-status" class="panel-status">connecting...</div>
    </div>

    <div class="panel control-panel">
      <div class="label">LED Color</div>
      <div class="color-row">
        <div class="color-wheel-wrap">
          <canvas id="colorWheel" width="200" height="200" style="border-radius:8px;background:#1c2530"></canvas>
        </div>
        <div class="color-controls">
          <div id="colorPreview"></div>
          <div class="mini-label">White (warm)</div>
          <input id="whiteSlider" type="range" min="0" max="255" value="0" style="width:120px;">
        </div>
      </div>
      <div class="action-row" style="margin-top:16px;">
        <button onclick="setPattern('off')">Off</button>
        <button onclick="setPattern('static')">Static</button>
        <button onclick="setPattern('rainbow')">Rainbow</button>
        <button onclick="setPattern('chase')">Chase</button>
        <button onclick="setPattern('random_fade')">Random Fade</button>
      </div>
      <div style="width:100%;margin-top:14px;">
        <div class="mini-label" style="margin-bottom:6px;">Brightness</div>
        <input id="bright" type="range" min="0" max="255" value="50" style="width:100%;">
      </div>
    </div>
  </div>

  <script>
    const TEMP_COMFORT_MIN = 20;
    const TEMP_COMFORT_MAX = 25;
    const TEMP_WARN_DELTA = 5;
    const TEMP_DANGER_DELTA = 15;

    const HUM_COMFORT_MIN = 40;
    const HUM_COMFORT_MAX = 60;
    const HUM_WARN_DELTA = 15;
    const HUM_DANGER_DELTA = 35;

    const DEFAULT_COLOR = [238, 238, 238];
    const YELLOW = [255, 214, 0];
    const RED = [220, 38, 38];

    function lerp(a, b, t) {
      return Math.round(a + t * (b - a));
    }

    function mixColor(c1, c2, t) {
      return `rgb(${lerp(c1[0], c2[0], t)},${lerp(c1[1], c2[1], t)},${lerp(c1[2], c2[2], t)})`;
    }

    function colorFor(value, comfortMin, comfortMax, warnDelta, dangerDelta) {
      let deviation = 0;
      if (value < comfortMin) deviation = comfortMin - value;
      else if (value > comfortMax) deviation = value - comfortMax;

      if (deviation <= 0) return mixColor(DEFAULT_COLOR, DEFAULT_COLOR, 0);
      if (deviation <= warnDelta) {
        return mixColor(DEFAULT_COLOR, YELLOW, deviation / warnDelta);
      }
      const t = Math.min((deviation - warnDelta) / (dangerDelta - warnDelta), 1);
      return mixColor(YELLOW, RED, t);
    }

    async function refresh() {
      try {
        const res = await fetch('/data');
        const json = await res.json();

        const tempEl = document.getElementById('temp');
        const humEl = document.getElementById('hum');
        const luxEl = document.getElementById('lux');
        const tempRow = document.getElementById('temp-row');
        const humRow = document.getElementById('hum-row');
        const luxRow = document.getElementById('lux-row');

        const tempAvailable = json.temperatureAvailable !== undefined ? !!json.temperatureAvailable : true;
        const humAvailable = json.humidityAvailable !== undefined ? !!json.humidityAvailable : true;
        const luxAvailable = json.luxAvailable !== undefined ? !!json.luxAvailable : true;

        tempRow.style.display = tempAvailable ? 'flex' : 'none';
        humRow.style.display = humAvailable ? 'flex' : 'none';
        luxRow.style.display = luxAvailable ? 'flex' : 'none';

        if (!tempAvailable) {
          tempEl.textContent = '--';
        } else {
          tempEl.textContent = Number(json.temperature || 0).toFixed(1);
          tempEl.style.color = colorFor(
            Number(json.temperature || 0), TEMP_COMFORT_MIN, TEMP_COMFORT_MAX, TEMP_WARN_DELTA, TEMP_DANGER_DELTA
          );
        }

        if (!humAvailable) {
          humEl.textContent = '--';
        } else {
          humEl.textContent = Number(json.humidity || 0).toFixed(1);
          humEl.style.color = colorFor(
            Number(json.humidity || 0), HUM_COMFORT_MIN, HUM_COMFORT_MAX, HUM_WARN_DELTA, HUM_DANGER_DELTA
          );
        }

        if (!luxAvailable) {
          luxEl.textContent = '--';
        } else {
          luxEl.textContent = Number(json.lux || 0).toLocaleString();
          luxEl.style.color = json.lux < 1000 ? '#bfe3ff' : json.lux < 5000 ? '#f6d365' : '#ff6b6b';
        }

        const timeStamp = 'updated ' + new Date().toLocaleTimeString();
        document.getElementById('status').textContent = timeStamp;
        document.getElementById('lux-status').textContent = timeStamp;
      } catch (e) {
        document.getElementById('status').textContent = 'connection lost, retrying...';
        document.getElementById('lux-status').textContent = 'connection lost, retrying...';
      }
    }

    refresh();
    setInterval(refresh, 10000);

    function sendLedRequest(path) {
      return fetch(path).catch(() => {
        document.getElementById('status').textContent = 'LED request failed';
      });
    }

    function hsvToRgb(h, s, v) {
      let r, g, b;
      let i = Math.floor(h * 6);
      let f = h * 6 - i;
      let p = v * (1 - s);
      let q = v * (1 - f * s);
      let t = v * (1 - (1 - f) * s);
      switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
      }
      return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
    }

    const wheel = document.getElementById('colorWheel');
    const wctx = wheel.getContext('2d');
    const cw = wheel.width, ch = wheel.height;
    const cx = cw / 2, cy = ch / 2, radius = Math.min(cx, cy) - 1;
    const img = wctx.createImageData(cw, ch);

    for (let y = 0; y < ch; y++) {
      for (let x = 0; x < cw; x++) {
        const dx = x - cx;
        const dy = y - cy;
        const d = Math.sqrt(dx * dx + dy * dy);
        const idx = (y * cw + x) * 4;
        if (d <= radius) {
          const angle = Math.atan2(dy, dx);
          const hue = (angle < 0 ? angle + Math.PI * 2 : angle) / (Math.PI * 2);
          const sat = d / radius;
          const [r, g, b] = hsvToRgb(hue, sat, 1);
          img.data[idx] = r; img.data[idx + 1] = g; img.data[idx + 2] = b; img.data[idx + 3] = 255;
        } else {
          img.data[idx] = 255; img.data[idx + 1] = 255; img.data[idx + 2] = 255; img.data[idx + 3] = 0;
        }
      }
    }
    wctx.putImageData(img, 0, 0);

    let selectedColor = [255, 255, 255];
    function updatePreview() {
      document.getElementById('colorPreview').style.background = `rgb(${selectedColor[0]},${selectedColor[1]},${selectedColor[2]})`;
    }

    wheel.addEventListener('click', (ev) => {
      const rect = wheel.getBoundingClientRect();
      const x = ev.clientX - rect.left;
      const y = ev.clientY - rect.top;
      const dx = x - cx, dy = y - cy;
      const d = Math.sqrt(dx * dx + dy * dy);
      if (d > radius) return;
      const angle = Math.atan2(dy, dx);
      const hue = (angle < 0 ? angle + Math.PI * 2 : angle) / (Math.PI * 2);
      const sat = d / radius;
      selectedColor = hsvToRgb(hue, sat, 1);
      updatePreview();
      const w = document.getElementById('whiteSlider').value;
      sendLedRequest(`/leds/set?r=${selectedColor[0]}&g=${selectedColor[1]}&b=${selectedColor[2]}&w=${w}`);
    });

    document.getElementById('whiteSlider').addEventListener('input', (e) => {
      const w = e.target.value;
      document.getElementById('colorPreview').style.filter = `brightness(${100 + (w / 255) * 50}%)`;
      sendLedRequest(`/leds/set?r=${selectedColor[0]}&g=${selectedColor[1]}&b=${selectedColor[2]}&w=${w}`);
    });

    function setPattern(name) {
      const w = document.getElementById('whiteSlider').value;
      if (name === 'static') {
        sendLedRequest(`/leds/pattern?pattern=static&r=${selectedColor[0]}&g=${selectedColor[1]}&b=${selectedColor[2]}&w=${w}`);
      } else {
        sendLedRequest(`/leds/pattern?pattern=${name}`);
      }
    }

    document.getElementById('bright').addEventListener('input', (e) => {
      sendLedRequest(`/leds/brightness?b=${e.target.value}`);
    });
  </script>
</body>
</html>
)=====";
