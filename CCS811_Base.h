#pragma once

#include <cstddef>
#include <cstdint>

// Platform-independent API. Implement virtual methods in derived class.
class CCS811_Base {
public:
  enum class Mode {
    IDLE,
    EVERY_1_S,
    EVERY_10_S,
    EVERY_60_S,
  };

  int init();

  int set_mode(Mode);
  int update_data();
  // Unit is PPM
  uint16_t CO2() const { return eCO2_; }
  // Unit is PPB
  uint16_t TVOC() const { return tvoc_; }

  int get_baseline(uint16_t &);
  int set_baseline(uint16_t);

  // Humidity: relative humidity pct * 512, 0x6400 for 50%
  // Temperature: temperature in degrees Celsius * 512, -0x400 for -2 C.
  int set_env_data(uint16_t humidity, int16_t temperature);
  int get_sensor_error(uint8_t &);

  static const int CCS811_OK = 0;
  static const int CCS811_IOERROR = -127;
  static const int CCS811_NOT_FOUND = -128;
  static const int CCS811_INVALID_STATE = -129;
  static const int CCS811_SENSOR_ERROR = -130;

protected:
  virtual ~CCS811_Base() = default;

  virtual int i2c_read(uint8_t reg_addr, uint8_t *reg_data, size_t len) = 0;
  virtual int i2c_write(
      uint8_t reg_addr, const uint8_t *reg_data, size_t len) = 0;
  virtual void delay_us(unsigned int usec) = 0;

private:
  uint16_t eCO2_{0};
  uint16_t tvoc_{0};
};
