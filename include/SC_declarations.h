// ---- EEPROM layout for WiFi failsafe ----
#define EEPROM_SIZE        200
#define EEPROM_FLAG_ADDR   0    // 1 byte: EEPROM_VALID_FLAG once credentials are stored
#define EEPROM_SSID_ADDR   1    // 33 bytes
#define EEPROM_PASS_ADDR   34   // 129 bytes
#define EEPROM_SUBNET_ADDR 163  // 1 byte, static IP 3rd octet (192.168.X.___)
#define EEPROM_HOST_ADDR   164  // 1 byte, static IP 4th octet (192.168.___.X)
#define EEPROM_AP_FALLBACK_ADDR 165 // 1 byte: set when WiFi connect fails to force AP on next boot
#define EEPROM_VALID_FLAG  0xAA

// ---- NeoPixel bridge ----
#define SLAVE_ADDR 0x08
#define NUM_LEDS   10

/** Forward function declarations */
void handleRoot();
void handleData();
bool loadCredentialsFromEEPROM();
void saveCredentialsToEEPROM(const String& ssid, const String& pass, uint8_t subnetOctet, uint8_t hostOctet);
void handleUpdateForm();
void handleUpdate();
void startFailsafeAccessPoint();