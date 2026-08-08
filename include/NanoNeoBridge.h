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
      // nothing device-specific to init; kept for API symmetry / future use
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
      const uint8_t maxPayload = 29; // 32 - (cmd + offset + len)
      uint16_t offset = 0;
      bool ok = true;

      while (offset < _frameBytes) {
        uint8_t len = min((uint16_t)maxPayload, (uint16_t)(_frameBytes - offset));

        Wire.beginTransmission(_addr);
        Wire.write(CMD_FRAME_CHUNK);
        Wire.write((uint8_t)offset);
        Wire.write(len);
        Wire.write(_buf + offset, len);
        uint8_t err = Wire.endTransmission();
        if (err != 0) ok = false;

        offset += len;
      }
      return ok;
    }

    // Set global brightness (0-255). Takes effect on the slave's next show.
    bool setBrightness(uint8_t value) {
      Wire.beginTransmission(_addr);
      Wire.write(CMD_BRIGHTNESS);
      Wire.write(value);
      return Wire.endTransmission() == 0;
    }

    // Zero all pixels on the slave immediately (also clears local buffer).
    bool clear() {
      memset(_buf, 0, _frameBytes);
      Wire.beginTransmission(_addr);
      Wire.write(CMD_CLEAR);
      return Wire.endTransmission() == 0;
    }

  private:
    static const uint8_t CMD_FRAME_CHUNK = 0x01;
    static const uint8_t CMD_BRIGHTNESS  = 0x02;
    static const uint8_t CMD_CLEAR       = 0x03;

    uint8_t  _addr;
    uint8_t  _numLeds;
    uint16_t _frameBytes;
    uint8_t* _buf;
};

#endif
