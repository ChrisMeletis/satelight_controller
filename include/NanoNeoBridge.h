/*
 * NanoNeoBridge - minimal master-side library
 * ---------------------------------------------
 * Talks to the "Nano I2C to SK6812 NeoPixel translator" sketch.
 * Buffer pixels locally with setPixel(), then call show() to flush.
 * show() automatically splits the frame into <=32-byte I2C writes,
 * matching the CMD_FRAME_CHUNK protocol on the slave.
 *
 * License: MIT (do whatever you want with this file)
 */

#ifndef NANO_NEO_BRIDGE_H
#define NANO_NEO_BRIDGE_H

#include <Arduino.h>
#include <Wire.h>

class NanoNeoBridge {
  public:
    // numLeds must match NUM_LEDS on the slave sketch.
    NanoNeoBridge(uint8_t i2cAddress, uint8_t numLeds)
      : _addr(i2cAddress), _numLeds(numLeds), _frameBytes(numLeds * 4) {
      _buf = new uint8_t[_frameBytes];
      memset(_buf, 0, _frameBytes);
    }

    ~NanoNeoBridge() {
      delete[] _buf;
    }

    // Call once in setup(), after Wire.begin() on the master.
    void begin() {
      _deviceAvailable = probe();
      if (!_deviceAvailable) {
        Serial.println("NanoNeoBridge not detected; LED output disabled.");
      }
    }

    bool isAvailable() const {
      return _deviceAvailable;
    }

    bool probe() {
      Wire.beginTransmission(_addr);
      Wire.write(CMD_BRIGHTNESS);
      Wire.write(0);
      uint8_t err = Wire.endTransmission();
      if (err == 0) {
        _deviceAvailable = true;
        return true;
      }
      _deviceAvailable = false;
      return false;
    }

    // Stage a pixel's color in the local buffer. Does not send anything yet.
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
      if (index >= _numLeds) return;
      uint16_t i = index * 4;
      _buf[i]     = r;
      _buf[i + 1] = g;
      _buf[i + 2] = b;
      _buf[i + 3] = w;
    }

    // Stage all pixels the same color at once.
    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
      for (uint8_t i = 0; i < _numLeds; i++) setPixel(i, r, g, b, w);
    }

    // Flush the local buffer to the slave. Splits into <=32-byte I2C
    // transactions automatically (3-byte header + up to 29 data bytes each).
    // Returns true if every chunk was acknowledged (Wire.endTransmission()==0).
    bool show() {
      if (!_deviceAvailable) {
        return false;
      }

      const uint8_t maxPayload = 29; // 32 - (cmd + offset + len)
      uint16_t offset = 0;
      bool ok = true;
        // try the whole frame a few times in case of transient I2C errors
        const int maxAttempts = 3;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
          offset = 0;
          ok = true;
          while (offset < _frameBytes) {
            uint8_t len = min((uint16_t)maxPayload, (uint16_t)(_frameBytes - offset));

            Wire.beginTransmission(_addr);
            Wire.write(CMD_FRAME_CHUNK);
            Wire.write((uint8_t)offset);
            Wire.write(len);
            Wire.write(_buf + offset, len);
            uint8_t err = Wire.endTransmission();
            if (err != 0) {
              ok = false;
              _deviceAvailable = false;
              Serial.print("NanoNeoBridge::show chunk err offset="); Serial.print(offset);
              Serial.print(" len="); Serial.print(len);
              Serial.print(" err="); Serial.println(err);
              break; // abort this attempt
            }

            offset += len;
          }

          if (ok) return true;
          // small backoff and re-init Wire in case the bus/slave needs resetting
          Serial.print("NanoNeoBridge::show retry "); Serial.println(attempt + 1);
          delay(10);
          Wire.begin();
        }
        return false;
    }

    // Set global brightness (0-255). Takes effect on the slave's next show.
    bool setBrightness(uint8_t value) {
      if (!_deviceAvailable) {
        return false;
      }

      Wire.beginTransmission(_addr);
      Wire.write(CMD_BRIGHTNESS);
      Wire.write(value);
      // retry a few times on transient errors
      const int maxAttempts = 3;
      for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
          _deviceAvailable = true;
          return true;
        }
        _deviceAvailable = false;
        Serial.print("NanoNeoBridge::setBrightness err="); Serial.println(err);
        Serial.print("NanoNeoBridge::setBrightness retry "); Serial.println(attempt + 1);
        delay(10);
        Wire.begin();
        Wire.beginTransmission(_addr);
        Wire.write(CMD_BRIGHTNESS);
        Wire.write(value);
      }
      return false;
    }

    // Zero all pixels on the slave immediately (also clears local buffer).
    bool clear() {
      if (!_deviceAvailable) {
        return false;
      }

      memset(_buf, 0, _frameBytes);
      Wire.beginTransmission(_addr);
      Wire.write(CMD_CLEAR);
      // retry clear a few times
      const int maxAttempts = 3;
      for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        uint8_t err = Wire.endTransmission();
        if (err == 0) return true;
        _deviceAvailable = false;
        Serial.print("NanoNeoBridge::clear err="); Serial.println(err);
        Serial.print("NanoNeoBridge::clear retry "); Serial.println(attempt + 1);
        delay(10);
        Wire.begin();
        Wire.beginTransmission(_addr);
        Wire.write(CMD_CLEAR);
      }
      return false;
    }

  private:
    static const uint8_t CMD_FRAME_CHUNK = 0x01;
    static const uint8_t CMD_BRIGHTNESS  = 0x02;
    static const uint8_t CMD_CLEAR       = 0x03;

    uint8_t  _addr;
    uint8_t  _numLeds;
    uint16_t _frameBytes;
    uint8_t* _buf;
    bool     _deviceAvailable = true;
};

#endif
