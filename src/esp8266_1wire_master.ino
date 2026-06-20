#include <OneWire.h>
#include <DallasTemperature.h>

// 1-Wire data pin - GPIO2 (labeled D4 on NodeMCU / Wemos D1 mini).
// Idles HIGH via the bus pull-up, so it's safe to use despite being
// a boot-strapping pin. Change this if you're on a different board/pin.
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress sensorAddress;

void setup() {
  Serial.begin(9600);
  sensors.begin();

  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount());
  Serial.println(" device(s) on the bus.");

  if (!sensors.getAddress(sensorAddress, 0)) {
    Serial.println("No 1-Wire device found - check wiring/pull-up resistor.");
  } else {
    // Matches the slave's 9-bit resolution -> faster conversions (~94ms vs 750ms)
    sensors.setResolution(sensorAddress, 9);
  }
}

void loop() {
  sensors.requestTemperatures();

  // The slave (Nano) packs its light level (0-100%) into the
  // standard DS18B20 temperature field, so we just read it as if
  // it were a temperature and relabel it.
  float lightLevel = sensors.getTempCByIndex(0);

  Serial.print("Light level: ");
  Serial.print(lightLevel);
  Serial.println(" %");

  delay(1000);
}
