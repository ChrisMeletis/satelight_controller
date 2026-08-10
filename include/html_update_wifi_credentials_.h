const char html_update_wifi_credentials[] = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Setup</title>
<style>
  body {
    font-family: -apple-system, Arial, sans-serif;
    background: #10151c;
    color: #eee;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    margin: 0;
  }
  .card {
    background: #1c2530;
    border-radius: 16px;
    padding: 32px 40px;
    text-align: center;
    box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    max-width: 360px;
    width: 85%;
  }
  h2 {
    font-size: 1.4rem;
    color: #eee;
    margin: 20px 0 8px;
    font-weight: 600;
  }
  .label {
    font-size: 0.9rem;
    color: #8a97a8;
    text-transform: uppercase;
    letter-spacing: 1px;
    margin-top: 16px;
  }
  input[type="text"] {
    text-align: center;
    width: 90%;
    font-size: 1.3rem;
    padding: 8px;
    margin-top: 6px;
    border-radius: 8px;
    border: 1px solid #333c48;
    background: #10151c;
    color: #eee;
  }
  input[type="small_text"] {
    width: 20%;
    font-size: 1.3rem;
    padding: 8px;
    margin-top: 6px;
    border-radius: 8px;
    border: 1px solid #333c48;
    background: #10151c;
    color: #eee;
    text-align: center;
  }
  input[type="submit"] {
    width: 60%;
    font-size: 1.2rem;
    padding: 10px;
    margin-top: 28px;
    border-radius: 8px;
    border: none;
    background: #eee;
    color: #10151c;
    font-weight: 600;
    cursor: pointer;
  }
  .ip-row {
    font-size: 1.2rem;
    color: #eee;
    margin-top: 6px;
  }
</style>
</head>

<body>
  <div class="card">
    <h2>WiFi Setup</h2>
    <form action='/update' method='POST'>
      <div class="label">Network Name</div>
      <input type="text" name="NEW_SSID" maxlength="32">

      <div class="label">Network Password</div>
      <input type="text" name="NEW_PASS" maxlength="128">

      <div class="label">Network Address</div>
      <div class="ip-row">
        192.168.
        <input  type="small_text"
                name="NEW_ADDR_SUBNET"
                value="002"
                maxlength="3"
                onkeyup="if(this.value.length>=3) NEW_ADDR_HOST.focus()">.
        <input  type="small_text"
                name="NEW_ADDR_HOST"
                value="200"
                maxlength="3">
      </div>

      <input type="submit" value="Submit">
    </form>
  </div>
</body>
</html>
)=====";
