#include "CCS811.h"

CCS811::CCS811(I2C &i2c, int addr) : i2c_{i2c}, addr_{addr} {
  /* no-op */
}

int CCS811::i2c_read(uint8_t reg_addr, uint8_t *reg_data, size_t len) {
  auto reg = reinterpret_cast<const char *>(&reg_addr);
  auto data = reinterpret_cast<char *>(reg_data);

  auto res = i2c_.write(addr_, reg, sizeof(reg_addr));
  if (res == 0) {
    res = i2c_.read(addr_, data, len);
  }

  return res == 0 ? CCS811_OK : CCS811_IOERROR;
}

int CCS811::i2c_write(uint8_t reg_addr, const uint8_t *reg_data, size_t len) {
  auto reg = reinterpret_cast<const char *>(&reg_addr);
  auto data = reinterpret_cast<const char *>(reg_data);

  auto res = i2c_.write(addr_, reg, sizeof(reg_addr), true);
  if (res != 0) {
    i2c_.stop();
  } else {
    res = i2c_.write(addr_, data, len);
  }

  return res == 0 ? CCS811_OK : CCS811_IOERROR;
}

void CCS811::delay_us(unsigned int usec) {
  auto delay = std::chrono::microseconds(usec);
  auto u32delay =
      std::chrono::duration_cast<Kernel::Clock::duration_u32>(delay);

  if (u32delay < delay) {
    u32delay = Kernel::Clock::duration_u32{1};
  }

  ThisThread::sleep_for(u32delay);
}
