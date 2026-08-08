// ---- WiFi credentials ----
const char* WIFI_SSID     = "<YOUR_SSID>";
const char* WIFI_PASSWORD = "<YOUR_PASSWORD>";
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
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AHT10.h>
#include <Wire.h>
#include "NanoNeoBridge.h"
#include "html_temperature_humidity.h"

// ---- Static IP config ----
IPAddress local_IP(192, 168, 1, 154);
IPAddress gateway(192, 168, 1, 1);     // adjust to match your router if different
IPAddress subnet(255, 255, 255, 0);

// ---- NeoPixel bridge ----
#define SLAVE_ADDR 0x08
#define NUM_LEDS   10
NanoNeoBridge leds(SLAVE_ADDR, NUM_LEDS);

// ---- AHT10 sensor ----
AHT10 myAHT10(AHT10_ADDRESS_0X38);

// ---- Web server ----
ESP8266WebServer server(80);

// Latest readings, updated once per loop() pass
float currentTempC = NAN;
float currentHumidity = NAN;

void handleRoot() {
	server.send(200, "text/html", html);
}

void handleData() {
  char json[64];
  snprintf(json, sizeof(json), "{\"temperature\":%.1f,\"humidity\":%.1f}",
           currentTempC, currentHumidity);
  server.send(200, "application/json", json);
}

void setup(void) {
  Serial.begin(9600);

  leds.begin();
  leds.show();
  leds.setBrightness(50);
  myAHT10.begin();

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Static IP configuration failed");
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server started.");
}

void loop(void) {
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