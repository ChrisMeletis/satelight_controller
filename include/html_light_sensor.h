const char html_light[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Ambient Light</title>
  <style>
    body {
      font-family: -apple-system, Arial, sans-serif;
      background: #10151c;
      color: #eee;
      display: flex;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
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
    #status { margin-top: 16px; font-size: 0.8rem; color: #555; }
  </style>
</head>
<body>
  <div class="card">
    <div class="label">Ambient Light</div>
    <div class="value"><span id="lux">--</span><span class="unit"> lux</span></div>
    <div id="status">connecting...</div>
  </div>

  <script>
    async function refresh() {
      try {
        const res = await fetch('/data');
        const json = await res.json();
        const lux = Number(json.lux || 0);
        const luxEl = document.getElementById('lux');
        luxEl.textContent = lux.toLocaleString();
        luxEl.style.color = lux < 1000 ? '#bfe3ff' : lux < 5000 ? '#f6d365' : '#ff6b6b';
        document.getElementById('status').textContent = 'updated ' + new Date().toLocaleTimeString();
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
