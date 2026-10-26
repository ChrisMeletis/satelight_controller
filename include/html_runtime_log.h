const char html_log[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Runtime Log</title>
  <style>
    body {
      font-family: -apple-system, Arial, sans-serif;
      background: #10151c;
      color: #eee;
      margin: 0;
      padding: 20px;
    }
    .card {
      max-width: 960px;
      margin: 0 auto;
      background: #1c2530;
      border-radius: 16px;
      padding: 24px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    }
    h1 {
      margin: 0 0 16px;
      font-size: 1.5rem;
    }
    pre {
      background: #0f1720;
      color: #dfeaff;
      border-radius: 10px;
      padding: 16px;
      white-space: pre-wrap;
      word-break: break-word;
      min-height: 300px;
      max-height: 70vh;
      overflow: auto;
      font-size: 0.85rem;
      line-height: 1.5;
    }
    .toolbar {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: center;
      margin-bottom: 12px;
    }
    button {
      background: #2b7a78;
      color: white;
      border: none;
      border-radius: 8px;
      padding: 8px 12px;
      cursor: pointer;
    }
    a {
      color: #7ec8ff;
      text-decoration: none;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="toolbar">
      <h1>Runtime Log</h1>
      <div>
        <button onclick="refreshLog()">Refresh</button>
        <a href="/" style="margin-left:12px;">Home</a>
      </div>
    </div>
    <pre id="logOutput">loading...</pre>
  </div>

  <script>
    async function refreshLog() {
      try {
        const response = await fetch('/logs/data');
        const text = await response.text();
        document.getElementById('logOutput').textContent = text || 'No log entries yet.';
      } catch (e) {
        document.getElementById('logOutput').textContent = 'Unable to load log output.';
      }
    }
    refreshLog();
    setInterval(refreshLog, 2000);
  </script>
</body>
</html>
)=====";
