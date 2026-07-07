#include <OneWire.h>
#include <Adafruit_NeoPixel.h>

#define ONE_WIRE_BUS 2
#define LEDSTRIP_PIN 14

OneWire sensor(ONE_WIRE_BUS);
Adafruit_NeoPixel strip(5, LEDSTRIP_PIN, NEO_GRB + NEO_KHZ800);

void setup(void)
{
	Serial.begin(9600);
	strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
	strip.show();            // Turn OFF all pixels ASAP
	strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void loop(void){
	byte i;
	byte present = 0;
	byte data[9];
	byte addr[8];

	if (!sensor.search(addr)){
		Serial.println("No more addresses.");
		Serial.println();
		sensor.reset_search();
		delay(250);
		return;
	}

	Serial.print("ROM =");
	for (i = 0; i < 8; i++){
		Serial.write(' ');
		Serial.print(addr[i], HEX);
	}

	if (OneWire::crc8(addr, 7) != addr[7]){
		Serial.println("CRC is not valid!");
		return;
	}
	Serial.println();

	// the first ROM byte indicates which chip
	switch (addr[0]){
	case 0x80:
		Serial.println("Found Satelight Sensor Device"); // Family code detected
		break;
	default:
		Serial.println("Code not implemented for this device.");
		return;
	}

	present = sensor.reset();
	sensor.select(addr);
	sensor.write(0x01); // Read Scratchpad

	Serial.print("Data = ");
	Serial.print(present, HEX);
	Serial.print(" ");
	for (i = 0; i < 9; i++){ // we need 9 bytes
		data[i] = sensor.read();
		Serial.print(data[i], HEX);
		Serial.print(" ");
	}
	Serial.print(" CRC=");
	Serial.print(OneWire::crc8(data, 8), HEX);
	Serial.println();

	// Convert the data to actual luminence
	uint16_t lux = (data[1] << 8) | data[0];
	Serial.print("Luminence = ");
	Serial.print(lux);
	Serial.println(" lx");

	// Flash leds
	// Set in all pixels to yellow
	for (uint8_t fade_in = 0 ; fade_in < 255 ; fade_in += 5){
		for (int i = 0; i <= 5; i++) { strip.setPixelColor(i, strip.Color(fade_in, fade_in, fade_in)); } // Set pixel to yellow			
		strip.show(); // Update the pixel strip
		delay(20);
	}
	// Set out all pixels to black
	for (uint8_t fade_out = 255 ; fade_out > 0 ; fade_out -= 5){
		for (int i = 0; i <= 5; i++) { strip.setPixelColor(i, strip.Color(fade_out, fade_out, fade_out)); } // Set pixel to yellow			
		strip.show(); // Update the pixel strip
		delay(20);
	}
}