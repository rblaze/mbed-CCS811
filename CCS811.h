#pragma once

#include <mbed.h>

#include "CCS811_Base.h"

// Mbed OS specific code
class CCS811 : public CCS811_Base {
public:
  CCS811(I2C& i2c, int addr);

protected:
  int i2c_read(uint8_t reg_addr, uint8_t* reg_data, size_t len) override;
  int i2c_write(uint8_t reg_addr, const uint8_t* reg_data, size_t len) override;
  void delay_us(unsigned int usec) override;

  I2C& i2c_;
  int addr_;
};
