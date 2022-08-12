#pragma once

#include <mbed.h>

class CCS811 {
 public:
  enum class Mode {
    IDLE,
    EVERY_1_S,
    EVERY_10_S,
    EVERY_60_S,
  };

  CCS811(I2C &bus, int address) : bus_{bus}, addr_{address} {}

  int init();

  // Set update frequency
  int setMode(Mode);
  // Read measurements results from sensor
  int refreshData();
  // Unit is PPM
  uint16_t getCO2() const { return eCO2_; }
  // Unit is PPB
  uint16_t getTVOC() const { return tvoc_; }

  int getBaseline(uint16_t &);
  int setBaseline(uint16_t);

  // Humidity: relative humidity pct * 512, 0x6400 for 50%
  // Temperature: temperature in degrees Celsius: (t + 25) * 512.
  int setEnvData(uint16_t humidity, uint16_t temperature);
  int getSensorError(uint8_t &);

  static const int CCS811_OK = 0;
  static const int CCS811_IOERROR = -127;
  static const int CCS811_NOT_FOUND = -128;
  static const int CCS811_INVALID_STATE = -129;
  static const int CCS811_SENSOR_ERROR = -130;
  static const int CCS811_STALE_DATA = -131;

 private:
  int i2c_read(uint8_t reg_addr, char *buf, size_t len);

  I2C &bus_;
  int addr_;
  uint16_t eCO2_{0};
  uint16_t tvoc_{0};
};
