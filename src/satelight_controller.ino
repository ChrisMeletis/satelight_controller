/*
 * ESP8266 + AHT10 + NanoNeoBridge, with a simple live web page
 * -------------------------------------------------------------
 * Serves:
 *   GET /       -> HTML page, auto-refreshes readings every 2s via fetch()
 *   GET /data   -> JSON: {"temperature":23.4,"humidity":41.2}
 *
 * Temperature/humidity values shift from yellow to red client-side as they
 * approach the "unsafe" thresholds defined below (see TEMP_SAFE_MAX /
 * TEMP_UNSAFE_MAX / HUM_SAFE_MAX / HUM_UNSAFE_MAX in the HTML's <script>).
 * These are just example comfort/heat-stress thresholds, not a medical
 * standard — tune them for your environment.
 *
 * WIFI FAILSAFE
 * -------------
 * On boot, checks EEPROM for stored WiFi credentials. If none are found
 * (first boot, or a wipe), the ESP8266 becomes an access point serving the
 * "WiFi Setup" form so credentials can be entered from a phone/laptop. On
 * submit, credentials + static IP octets are written to EEPROM and the
 * board reboots into normal station mode.
 */

#include <Arduino.h>
#include <SC_declarations.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AHT10.h>
#include <Wire.h>
#include "NanoNeoBridge.h"
#include "html_temperature_humidity.h"
#include "html_update_wifi_credentials_.h"

// ---- WiFi credentials ----
// Loaded from EEPROM at boot (see loadCredentialsFromEEPROM). Buffers, not
// literals, since the failsafe AP flow needs to write into them.
char WIFI_SSID[33]      = "";
char WIFI_PASSWORD[129] = "";

// ---- Failsafe access point (used only when no credentials are stored) ----
const char* AP_SSID     = "Satelight-Setup";
const char* AP_PASSWORD = ""; // open network for setup; set a password if you'd like
bool apMode = false; // true while running as the credential-setup access point
const unsigned long AP_RETRY_INTERVAL_MS = 120000; // retry connection after 2 min in AP when no client is connected
unsigned long lastApRetryMillis = 0;

// ---- Static IP config ----
IPAddress local_IP(192, 168, 1, 154); // fallback; 3rd/4th octets overwritten from EEPROM
IPAddress gateway(192, 168, 1, 1);     // adjust to match your router if different
IPAddress subnet(255, 255, 255, 0);

bool attemptWiFiConnection(unsigned long timeoutMs);
void startNormalWebServer();
bool shouldForceFailsafeAP();
void setForcedFailsafeAP(bool enable);
// LED control handlers
void handleLedSet();
void handleLedPattern();
void handleLedBrightness();

// ---- NeoPixel bridge ----
NanoNeoBridge leds(SLAVE_ADDR, NUM_LEDS);

// ---- AHT10 sensor ----
AHT10 myAHT10(AHT10_ADDRESS_0X38);

// ---- Web server ----
WebServer server(80);

// Latest readings, updated once per loop() pass
float currentTempC = NAN;
float currentHumidity = NAN;

// ---- LED/pattern state ----
enum Pattern { PATTERN_OFF = 0, PATTERN_STATIC, PATTERN_RAINBOW, PATTERN_CHASE };
volatile Pattern currentPattern = PATTERN_OFF;
uint8_t staticR = 0, staticG = 0, staticB = 0;
uint8_t staticW = 0;
unsigned long lastPatternMillis = 0;
const unsigned long PATTERN_INTERVAL_MS = 50;
int patternPos = 0; // moving index used by patterns
unsigned long lastSensorMillis = 0;
const unsigned long SENSOR_INTERVAL_MS = 2000;

void setup(void) {
  // Keep serial alive first so we can always see startup diagnostics.
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  if (shouldForceFailsafeAP()) {
    Serial.println("Forced AP fallback after previous WiFi failure.");
    setForcedFailsafeAP(false);
    startFailsafeAccessPoint();
    return;
  }

  if (!loadCredentialsFromEEPROM()) {
    Serial.println("No valid WiFi credentials found. Starting failsafe access point...");
    startFailsafeAccessPoint();
    return; // stay in AP / credential-setup mode; loop() just serves the form
  }

  if (!attemptWiFiConnection(15000)) {
    Serial.println("Failed to connect to saved WiFi. Restarting into AP mode...");
    setForcedFailsafeAP(true);
    delay(100);
    ESP.restart();
    return;
  }

  startNormalWebServer();

  // Only initialise devices that are needed in normal WiFi mode after a valid credential record is present.
  Wire.begin();
  leds.begin();
  if (leds.isAvailable()) {
    leds.show();
    leds.setBrightness(50);
    // Simple startup fade to white
    for (int v = 0; v <= 255; v += 8) {
      uint8_t vv = (uint8_t)v;
      leds.fill(vv, vv, vv);
      leds.show();
      delay(10);
    }
  } else {
    Serial.println("LED bridge not present; continuing without LED output.");
  }
  myAHT10.begin();

}

void loop(void) {
  server.handleClient(); // keep serving web requests frequently

  if (apMode) {
    // Only serve the WiFi Setup form until reboot

    if (WiFi.softAPgetStationNum() == 0 && millis() - lastApRetryMillis >= AP_RETRY_INTERVAL_MS) {
      lastApRetryMillis = millis();
      Serial.println("No AP clients connected. Retrying saved WiFi connection...");
      if (attemptWiFiConnection(15000)) {
        Serial.println("Saved WiFi connected. Leaving AP mode.");
        server.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        apMode = false;
        startNormalWebServer();
        return;
      }
      Serial.println("Retry failed. Remaining in AP mode.");
    }
    return;
  }

  // Non-blocking sensor read (every SENSOR_INTERVAL_MS)
  if (millis() - lastSensorMillis >= SENSOR_INTERVAL_MS) {
    lastSensorMillis = millis();
    currentTempC = myAHT10.readTemperature();
    currentHumidity = myAHT10.readHumidity();
    Serial.print(F("Temperature:")); Serial.print(currentTempC); Serial.print(",");
    Serial.print(F("Humidity:")); Serial.println(currentHumidity);
  }

  // LED pattern updates (non-blocking)
  if (leds.isAvailable() && millis() - lastPatternMillis >= PATTERN_INTERVAL_MS) {
    lastPatternMillis = millis();
    patternPos = (patternPos + 1) & 0xFF;

    switch (currentPattern) {
      case PATTERN_STATIC:
        leds.fill(staticR, staticG, staticB, staticW);
        leds.show();
        break;
      case PATTERN_RAINBOW: {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
          uint8_t wheelPos = (uint8_t)((i * 256 / NUM_LEDS) + patternPos);
          uint8_t r,g,b;
          // wheel function
          if (wheelPos < 85) { r = wheelPos * 3; g = 255 - wheelPos * 3; b = 0; }
          else if (wheelPos < 170) { wheelPos -= 85; r = 255 - wheelPos * 3; g = 0; b = wheelPos * 3; }
          else { wheelPos -= 170; r = 0; g = wheelPos * 3; b = 255 - wheelPos * 3; }
          leds.setPixel(i, r, g, b);
          // leave white channel off for color wheel
          leds.setPixel(i, r, g, b, 0);
        }
        leds.show();
        break;
      }
      case PATTERN_CHASE: {
        leds.fill(0,0,0,0);
        int on = patternPos % NUM_LEDS;
        leds.setPixel(on, staticR, staticG, staticB, staticW);
        leds.show();
        break;
      }
      case PATTERN_OFF:
      default:
        // nothing
        break;
    }
  }
}

void handleRoot() {
	server.send(200, "text/html", html);
}

void handleData() {
  char json[64];
  snprintf(json, sizeof(json), "{\"temperature\":%.1f,\"humidity\":%.1f}", currentTempC, currentHumidity);
  server.send(200, "application/json", json);
}

// --- LED control HTTP handlers ---
void handleLedSet() {
  // Accept GET or POST args: r,g,b
  int r = server.hasArg("r") ? server.arg("r").toInt() : -1;
  int g = server.hasArg("g") ? server.arg("g").toInt() : -1;
  int b = server.hasArg("b") ? server.arg("b").toInt() : -1;
  int w = server.hasArg("w") ? server.arg("w").toInt() : -1;
  if (!leds.isAvailable()) {
    server.send(200, "application/json", "{\"status\":\"leds_unavailable\"}");
    return;
  }

  if (r >= 0 && g >= 0 && b >= 0) {
    staticR = (uint8_t)constrain(r, 0, 255);
    staticG = (uint8_t)constrain(g, 0, 255);
    staticB = (uint8_t)constrain(b, 0, 255);
    if (w >= 0) staticW = (uint8_t)constrain(w, 0, 255);
    currentPattern = PATTERN_STATIC;
    leds.fill(staticR, staticG, staticB, staticW);
    bool ok = leds.show();
    Serial.print("handleLedSet: r="); Serial.print(staticR);
    Serial.print(" g="); Serial.print(staticG);
    Serial.print(" b="); Serial.print(staticB);
    Serial.print(" w="); Serial.print(staticW);
    Serial.print(" show_ok="); Serial.println(ok);
    server.send(200, "application/json", ok ? "{\"status\":\"ok\"}" : "{\"status\":\"i2c_error\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"missing r/g/b\"}");
  }
}

void handleLedPattern() {
  if (!leds.isAvailable()) {
    server.send(200, "application/json", "{\"status\":\"leds_unavailable\"}");
    return;
  }

  // pattern=name where name in {off, static, rainbow, chase}
  String p = server.arg("pattern");
  if (p == "off") {
    currentPattern = PATTERN_OFF;
    staticR = staticG = staticB = 0;
    bool cleared = leds.clear();
    Serial.print("handleLedPattern: off cleared="); Serial.println(cleared);
    server.send(200, "application/json", cleared ? "{\"status\":\"off\"}" : "{\"status\":\"i2c_error\"}");
    return;
  }
  if (p == "rainbow") {
    currentPattern = PATTERN_RAINBOW;
    Serial.println("handleLedPattern: rainbow");
    server.send(200, "application/json", "{\"status\":\"rainbow\"}");
    return;
  }
  if (p == "chase") {
    currentPattern = PATTERN_CHASE;
    Serial.println("handleLedPattern: chase");
    server.send(200, "application/json", "{\"status\":\"chase\"}");
    return;
  }
  if (p == "static") {
    // optional r/g/b args to set static color
    int r = server.hasArg("r") ? server.arg("r").toInt() : staticR;
    int g = server.hasArg("g") ? server.arg("g").toInt() : staticG;
    int b = server.hasArg("b") ? server.arg("b").toInt() : staticB;
    int w = server.hasArg("w") ? server.arg("w").toInt() : staticW;
    staticR = (uint8_t)constrain(r, 0, 255);
    staticG = (uint8_t)constrain(g, 0, 255);
    staticB = (uint8_t)constrain(b, 0, 255);
    staticW = (uint8_t)constrain(w, 0, 255);
    currentPattern = PATTERN_STATIC;
    bool ok2;
    leds.fill(staticR, staticG, staticB, staticW);
    ok2 = leds.show();
    Serial.print("handleLedPattern: static r="); Serial.print(staticR);
    Serial.print(" g="); Serial.print(staticG);
    Serial.print(" b="); Serial.print(staticB);
    Serial.print(" w="); Serial.print(staticW);
    Serial.print(" show_ok="); Serial.println(ok2);
    server.send(200, "application/json", ok2 ? "{\"status\":\"static\"}" : "{\"status\":\"i2c_error\"}");
    return;
  }
  server.send(400, "application/json", "{\"error\":\"unknown pattern\"}");
}

void handleLedBrightness() {
  if (!leds.isAvailable()) {
    server.send(200, "application/json", "{\"status\":\"leds_unavailable\"}");
    return;
  }

  if (!server.hasArg("b")) {
    server.send(400, "application/json", "{\"error\":\"missing b\"}");
    return;
  }
  int bval = server.arg("b").toInt();
  bool ok = leds.setBrightness((uint8_t)constrain(bval, 0, 255));
  Serial.print("handleLedBrightness: b="); Serial.print(bval); Serial.print(" ok="); Serial.println(ok);
  server.send(200, "application/json", ok ? "{\"status\":\"ok\"}" : "{\"status\":\"i2c_error\"}");
}

/** ---- WiFi failsafe: EEPROM credential storage ----
 * Returns true if valid credentials were found and loaded into
 * WIFI_SSID / WIFI_PASSWORD / local_IP.
*/
bool loadCredentialsFromEEPROM() {
  if (EEPROM.read(EEPROM_FLAG_ADDR) != EEPROM_VALID_FLAG) {
    return false;
  }

  bool ssidPresent = false;
  for (uint8_t i = 0; i < sizeof(WIFI_SSID); i++) {
    uint8_t value = EEPROM.read(EEPROM_SSID_ADDR + i);
    if (value != 0xFF && value != 0) {
      ssidPresent = true;
    }
    WIFI_SSID[i] = (value == 0xFF || value == 0) ? '\0' : (char)value;
  }
  if (!ssidPresent) {
    WIFI_SSID[0] = '\0';
    WIFI_PASSWORD[0] = '\0';
    return false;
  }

  for (uint8_t i = 0; i < sizeof(WIFI_PASSWORD); i++) {
    uint8_t value = EEPROM.read(EEPROM_PASS_ADDR + i);
    WIFI_PASSWORD[i] = (value == 0xFF || value == 0) ? '\0' : (char)value;
  }

  local_IP[2] = EEPROM.read(EEPROM_SUBNET_ADDR);
  local_IP[3] = EEPROM.read(EEPROM_HOST_ADDR);
  return true;
}

void saveCredentialsToEEPROM(const String& ssid, const String& pass, uint8_t subnetOctet, uint8_t hostOctet) {
  for (uint8_t i = 0; i < sizeof(WIFI_SSID); i++) {
    EEPROM.write(EEPROM_SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
  }
  for (uint8_t i = 0; i < sizeof(WIFI_PASSWORD); i++) {
    EEPROM.write(EEPROM_PASS_ADDR + i, i < pass.length() ? pass[i] : 0);
  }
  EEPROM.write(EEPROM_SUBNET_ADDR, subnetOctet);
  EEPROM.write(EEPROM_HOST_ADDR, hostOctet);
  EEPROM.write(EEPROM_FLAG_ADDR, EEPROM_VALID_FLAG);
  EEPROM.write(EEPROM_AP_FALLBACK_ADDR, 0x00);
  EEPROM.commit();
}

bool attemptWiFiConnection(unsigned long timeoutMs) {
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Static IP configuration failed");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (millis() - start >= timeoutMs) {
      Serial.println();
      Serial.println("WiFi connection attempt timed out.");
      return false;
    }
  }

  Serial.println();
  Serial.print("WiFi connected successfully to SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("Assigned IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool shouldForceFailsafeAP() {
  return EEPROM.read(EEPROM_AP_FALLBACK_ADDR) == 0x55;
}

void setForcedFailsafeAP(bool enable) {
  EEPROM.write(EEPROM_AP_FALLBACK_ADDR, enable ? 0x55 : 0x00);
  EEPROM.commit();
}

/** Serves the "WiFi Setup" form (failsafe access point mode) */
void handleUpdateForm() {
  server.send(200, "text/html", html_update_wifi_credentials);
}

/** Handles the form POST, stores credentials, then reboots into normal WiFi mode */
void handleUpdate() {
  String newSSID = server.arg("NEW_SSID");
  String newPass = server.arg("NEW_PASS");
  uint8_t subnetOctet = server.arg("NEW_ADDR_SUBNET").toInt();
  uint8_t hostOctet   = server.arg("NEW_ADDR_HOST").toInt();

  saveCredentialsToEEPROM(newSSID, newPass, subnetOctet, hostOctet);

  server.send(200, "text/html",
    "<html><body style='background:#10151c;color:#eee;font-family:Arial;"
    "text-align:center;padding-top:40px;'><h2>Saved. Rebooting...</h2></body></html>");
  delay(1000);
  ESP.restart();
}

void startNormalWebServer() {
  server.stop();
  server.on("/", handleRoot);
  server.on("/data", handleData);
  // LED control endpoints
  server.on("/leds/set", HTTP_GET, handleLedSet);
  server.on("/leds/pattern", HTTP_GET, handleLedPattern);
  server.on("/leds/brightness", HTTP_GET, handleLedBrightness);
  server.on("/update_wifi", handleUpdateForm);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  Serial.println("Web server started.");
}

/** No stored credentials found: become an access point so credentials can be entered */
void startFailsafeAccessPoint() {
  apMode = true;
  lastApRetryMillis = millis();
  Serial.println("No WiFi credentials found. Starting failsafe access point...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500);
  Serial.print("AP started. Connect to \"");
  Serial.print(AP_SSID);
  Serial.print("\" and visit ");
  Serial.println(WiFi.softAPIP());
  Serial.println("If no client connects, the device will retry saved WiFi in 2 minutes.");

  server.stop();
  server.on("/", handleUpdateForm);
  server.on("/update", HTTP_GET, handleUpdateForm);
  server.on("/update", HTTP_POST, handleUpdate);
  // expose LED endpoints in AP mode too so clients can test lights
  server.on("/leds/set", HTTP_GET, handleLedSet);
  server.on("/leds/pattern", HTTP_GET, handleLedPattern);
  server.on("/leds/brightness", HTTP_GET, handleLedBrightness);
  server.onNotFound(handleUpdateForm);
  server.begin();
  Serial.println("AP web server started.");
}

void clearWifiEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF);   // 0xFF = erased/default flash state
  }

  EEPROM.write(EEPROM_FLAG_ADDR, 0x00);
  EEPROM.write(EEPROM_AP_FALLBACK_ADDR, 0x00);
  EEPROM.commit();
}