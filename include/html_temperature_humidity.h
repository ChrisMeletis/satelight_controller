const char html[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Temp / Humidity</title>
  <style>
    body {
      font-family: -apple-system, Arial, sans-serif;
      background: #10151c;
      color: #eee;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      margin: 0;
    }
    .card {
      background: #1c2530;
      border-radius: 16px;
      padding: 32px 48px;
      text-align: center;
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    }
    .value {
      font-size: 3rem;
      font-weight: 600;
      margin: 8px 0;
    }
    .label {
      font-size: 0.9rem;
      color: #8a97a8;
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    .unit { font-size: 1.5rem; color: #8a97a8; }
    .row { margin: 20px 0; }
    #status { margin-top: 16px; font-size: 0.8rem; color: #555; }
  </style>
</head>
<body>
  <div class="card">
    <div class="row">
      <div class="label">Temperature</div>
      <div class="value"><span id="temp">--</span><span class="unit">&deg;C</span></div>
    </div>
    <div class="row">
      <div class="label">Humidity</div>
      <div class="value"><span id="hum">--</span><span class="unit">%</span></div>
    </div>
    <div id="status">connecting...</div>
  </div>

  <div class="card" style="margin-top:16px; max-width:420px;">
    <div class="row">
      <div class="label">LED Color</div>
      <div style="display:flex;gap:8px;align-items:center;">
        <canvas id="colorWheel" width="200" height="200" style="border-radius:8px;background:#fff"></canvas>
        <div style="display:flex;flex-direction:column;gap:8px;">
          <div id="colorPreview" style="width:56px;height:56px;border-radius:8px;background:#ffffff;border:1px solid #222"></div>
          <div style="font-size:0.8rem;color:#8a97a8">White (warm)</div>
          <input id="whiteSlider" type="range" min="0" max="255" value="0" style="width:120px;">
        </div>
      </div>
    </div>
    <div class="row">
      <div class="label">Patterns</div>
      <div style="display:flex;gap:8px;justify-content:center;margin-top:8px;">
        <button onclick="setPattern('off')">Off</button>
        <button onclick="setPattern('static')">Static</button>
        <button onclick="setPattern('rainbow')">Rainbow</button>
        <button onclick="setPattern('chase')">Chase</button>
      </div>
    </div>
    <div class="row">
      <div class="label">Brightness</div>
      <input id="bright" type="range" min="0" max="255" value="50" style="width:100%;">
    </div>
  </div>

  <script>
    // Example comfort ranges — tune for your environment.
    // Inside [COMFORT_MIN, COMFORT_MAX] the reading stays the default color.
    // Deviating from that range fades toward yellow, then toward red.
    const TEMP_COMFORT_MIN = 20;   // deg C
    const TEMP_COMFORT_MAX = 25;   // deg C
    const TEMP_WARN_DELTA  = 5;    // deviation (past the range) that's fully yellow
    const TEMP_DANGER_DELTA = 15;  // deviation that's fully red

    const HUM_COMFORT_MIN = 40;    // % RH
    const HUM_COMFORT_MAX = 60;    // % RH
    const HUM_WARN_DELTA  = 15;
    const HUM_DANGER_DELTA = 35;

    const DEFAULT_COLOR = [238, 238, 238]; // matches body text color (#eee)
    const YELLOW = [255, 214, 0];
    const RED    = [220, 38, 38];

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

        tempEl.textContent = json.temperature.toFixed(1);
        humEl.textContent = json.humidity.toFixed(1);

        tempEl.style.color = colorFor(
          json.temperature, TEMP_COMFORT_MIN, TEMP_COMFORT_MAX, TEMP_WARN_DELTA, TEMP_DANGER_DELTA
        );
        humEl.style.color = colorFor(
          json.humidity, HUM_COMFORT_MIN, HUM_COMFORT_MAX, HUM_WARN_DELTA, HUM_DANGER_DELTA
        );

        document.getElementById('status').textContent =
          'updated ' + new Date().toLocaleTimeString();
      } catch (e) {
        document.getElementById('status').textContent = 'connection lost, retrying...';
      }
    }
    refresh();
    setInterval(refresh, 2000);

    // LED control helpers
    function sendLedRequest(path) {
      return fetch(path).catch(()=>{ document.getElementById('status').textContent='LED request failed'; });
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
      return [Math.round(r*255), Math.round(g*255), Math.round(b*255)];
    }

    // Draw a simple color wheel once
    const wheel = document.getElementById('colorWheel');
    const wctx = wheel.getContext('2d');
    const cw = wheel.width, ch = wheel.height;
    const cx = cw/2, cy = ch/2, radius = Math.min(cx, cy)-1;
    const img = wctx.createImageData(cw, ch);
    for (let y=0;y<ch;y++){
      for (let x=0;x<cw;x++){
        const dx = x - cx, dy = y - cy;
        const d = Math.sqrt(dx*dx+dy*dy);
        const idx = (y*cw + x)*4;
        if (d <= radius) {
          const angle = Math.atan2(dy, dx);
          const hue = (angle < 0 ? angle + Math.PI*2 : angle) / (Math.PI*2);
          const sat = d / radius;
          const [r,g,b] = hsvToRgb(hue, sat, 1);
          img.data[idx] = r; img.data[idx+1] = g; img.data[idx+2] = b; img.data[idx+3] = 255;
        } else {
          img.data[idx] = 255; img.data[idx+1] = 255; img.data[idx+2] = 255; img.data[idx+3] = 0;
        }
      }
    }
    wctx.putImageData(img, 0, 0);

    let selectedColor = [255,255,255];
    function updatePreview(){
      document.getElementById('colorPreview').style.background = `rgb(${selectedColor[0]},${selectedColor[1]},${selectedColor[2]})`;
    }

    wheel.addEventListener('click', (ev)=>{
      const rect = wheel.getBoundingClientRect();
      const x = ev.clientX - rect.left; const y = ev.clientY - rect.top;
      const dx = x - cx, dy = y - cy; const d = Math.sqrt(dx*dx+dy*dy);
      if (d > radius) return;
      const angle = Math.atan2(dy, dx);
      const hue = (angle < 0 ? angle + Math.PI*2 : angle) / (Math.PI*2);
      const sat = d / radius;
      selectedColor = hsvToRgb(hue, sat, 1);
      updatePreview();
      const w = document.getElementById('whiteSlider').value;
      sendLedRequest(`/leds/set?r=${selectedColor[0]}&g=${selectedColor[1]}&b=${selectedColor[2]}&w=${w}`);
    });

    document.getElementById('whiteSlider').addEventListener('input', (e)=>{
      const w = e.target.value;
      document.getElementById('colorPreview').style.filter = `brightness(${100 + (w/255)*50}%)`;
      // send white adjustment with current RGB
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

    document.getElementById('bright').addEventListener('input', (e)=>{
      sendLedRequest(`/leds/brightness?b=${e.target.value}`);
    });
  </script>
</body>
</html>
)=====";
