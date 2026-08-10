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
  </script>
</body>
</html>
)=====";
