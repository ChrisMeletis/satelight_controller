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
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
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

// ---- NeoPixel bridge ----
NanoNeoBridge leds(SLAVE_ADDR, NUM_LEDS);

// ---- AHT10 sensor ----
AHT10 myAHT10(AHT10_ADDRESS_0X38);

// ---- Web server ----
ESP8266WebServer server(80);

// Latest readings, updated once per loop() pass
float currentTempC = NAN;
float currentHumidity = NAN;

void setup(void) {
  Serial.begin(9600);

  leds.begin();
  leds.show();
  leds.setBrightness(50);
  myAHT10.begin();

  EEPROM.begin(EEPROM_SIZE);

  bool forceAP = shouldForceFailsafeAP();
  if (!loadCredentialsFromEEPROM()) {
    startFailsafeAccessPoint();
    return; // stay in AP / credential-setup mode; loop() just serves the form
  }

  if (forceAP) {
    Serial.println("Forced AP fallback after previous WiFi failure.");
    setForcedFailsafeAP(false);
    startFailsafeAccessPoint();
    return;
  }

  if (!attemptWiFiConnection(15000)) {
    Serial.println("Failed to connect to saved WiFi. Restarting into AP mode...");
    setForcedFailsafeAP(true);
    delay(100);
    ESP.restart();
    return;
  }

  startNormalWebServer();
}

void loop(void) {
  if (apMode) {
    server.handleClient(); // only serve the WiFi Setup form until reboot

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

  server.handleClient(); // must be called often, so it's sprinkled below too

  currentTempC = myAHT10.readTemperature();
  currentHumidity = myAHT10.readHumidity();

  Serial.print(F("Temperature:")); Serial.print(currentTempC); Serial.print(",");
  Serial.print(F("Humidity:")); Serial.println(currentHumidity);
  delay(1000);
  server.handleClient();

  // Fade all pixels in to white
  for (uint8_t fade_in = 0; fade_in < 255; fade_in += 5) {
    for (int i = 0; i <= 5; i++) { leds.setPixel(i, fade_in, fade_in, fade_in); }
    leds.show();
    delay(20);
    server.handleClient(); // keep the web server responsive during the fade
  }
  // Fade all pixels out to black
  for (uint8_t fade_out = 255; fade_out > 0; fade_out -= 5) {
    for (int i = 0; i <= 5; i++) { leds.setPixel(i, fade_out, fade_out, fade_out); }
    leds.show();
    delay(20);
    server.handleClient();
  }
}

void handleRoot() {
	server.send(200, "text/html", html);
}

void handleData() {
  char json[64];
  snprintf(json, sizeof(json), "{\"temperature\":%.1f,\"humidity\":%.1f}",
           currentTempC, currentHumidity);
  server.send(200, "application/json", json);
}

/** ---- WiFi failsafe: EEPROM credential storage ----
 * Returns true if valid credentials were found and loaded into
 * WIFI_SSID / WIFI_PASSWORD / local_IP.
*/
bool loadCredentialsFromEEPROM() {
  if (EEPROM.read(EEPROM_FLAG_ADDR) != EEPROM_VALID_FLAG) {
    return false;
  }
  for (uint8_t i = 0; i < sizeof(WIFI_SSID); i++) {
    WIFI_SSID[i] = EEPROM.read(EEPROM_SSID_ADDR + i);
  }
  for (uint8_t i = 0; i < sizeof(WIFI_PASSWORD); i++) {
    WIFI_PASSWORD[i] = EEPROM.read(EEPROM_PASS_ADDR + i);
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
  Serial.print("Connected! IP address: ");
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
  server.onNotFound(handleUpdateForm);
  server.begin();
  Serial.println("AP web server started.");
}