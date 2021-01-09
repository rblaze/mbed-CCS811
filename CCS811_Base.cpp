#include "CCS811_Base.h"

namespace {
// Register list
static const uint8_t CCS811_STATUS = 0x00;
static const uint8_t CCS811_MEAS_MODE = 0x01;
static const uint8_t CCS811_ALG_RESULT_DATA = 0x02;  // up to 8 bytes
static const uint8_t CCS811_RAW_DATA = 0x03;         // 2 bytes
static const uint8_t CCS811_ENV_DATA = 0x05;         // 4 bytes
static const uint8_t CCS811_THRESHOLDS = 0x10;       // 5 bytes
static const uint8_t CCS811_BASELINE = 0x11;         // 2 bytes
static const uint8_t CCS811_HW_ID = 0x20;
static const uint8_t CCS811_HW_VERSION = 0x21;
static const uint8_t CCS811_FW_BOOT_VERSION = 0x23;  // 2 bytes
static const uint8_t CCS811_FW_APP_VERSION = 0x24;   // 2 bytes
static const uint8_t CCS811_ERROR_ID = 0xE0;
static const uint8_t CCS811_APP_ERASE = 0xF1;   // 4 bytes
static const uint8_t CCS811_APP_DATA = 0xF2;    // 9 bytes
static const uint8_t CCS811_APP_VERIFY = 0xF3;  // 0 bytes
static const uint8_t CCS811_APP_START = 0xF4;   // 0 bytes
static const uint8_t CCS811_SW_RESET = 0xFF;    // 4 bytes

// Refresh period
static const uint8_t CCS811_MODE_IDLE = 0;
static const uint8_t CCS811_MODE_1SEC = 1;
static const uint8_t CCS811_MODE_10SEC = 2;
static const uint8_t CCS811_MODE_60SEC = 3;
}  // namespace

int CCS811_Base::init() {
  int ret;
  uint8_t sw_reset[] = {0x11, 0xE5, 0x72, 0x8A};
  uint8_t u8;

  do {
    // Reset controller.
    ret = i2c_write(CCS811_SW_RESET, sw_reset, sizeof(sw_reset));
    if (ret != CCS811_OK) {
      break;
    }

    delay_us(2000);

    // Check hardware id and version.
    ret = i2c_read(CCS811_HW_ID, &u8, sizeof(u8));
    if (ret != CCS811_OK) {
      break;
    }
    if (u8 != 0x81) {
      ret = CCS811_NOT_FOUND;
      break;
    }

    ret = i2c_read(CCS811_HW_VERSION, &u8, sizeof(u8));
    if (ret != CCS811_OK) {
      break;
    }
    if ((u8 & 0xf0) != 0x10) {
      ret = CCS811_NOT_FOUND;
      break;
    }

    // Check that device is in boot mode after reset.
    ret = i2c_read(CCS811_STATUS, &u8, sizeof(u8));
    if (ret != CCS811_OK) {
      break;
    }
    if (u8 != 0x10) {
      ret = CCS811_INVALID_STATE;
      break;
    }

    // Switch to app mode.
    ret = i2c_write(CCS811_APP_START, nullptr, 0);
    if (ret != CCS811_OK) {
      break;
    }
    delay_us(1000);

    // Check that device is in app mode.
    ret = i2c_read(CCS811_STATUS, &u8, sizeof(u8));
    if (ret != CCS811_OK) {
      break;
    }
    if (u8 != 0x90) {
      ret = CCS811_INVALID_STATE;
      break;
    }
  } while (false);

  return ret;
}

int CCS811_Base::set_mode(CCS811_Base::Mode mode) {
  uint8_t status;

  switch (mode) {
    case Mode::IDLE:
      status = CCS811_MODE_IDLE << 4;
      break;
    case Mode::EVERY_1_S:
      status = CCS811_MODE_1SEC << 4;
      break;
    case Mode::EVERY_10_S:
      status = CCS811_MODE_10SEC << 4;
      break;
    case Mode::EVERY_60_S:
      status = CCS811_MODE_60SEC << 4;
      break;
  }

  return i2c_write(CCS811_MEAS_MODE, &status, sizeof(status));
}

int CCS811_Base::get_baseline(uint16_t &value) {
  auto p = reinterpret_cast<uint8_t *>(&value);
  return i2c_read(CCS811_BASELINE, p, sizeof(value));
}

int CCS811_Base::set_baseline(uint16_t value) {
  auto p = reinterpret_cast<const uint8_t *>(&value);
  return i2c_write(CCS811_BASELINE, p, sizeof(value));
}

int CCS811_Base::set_env_data(uint16_t humidity, int16_t temperature) {
  int32_t tadj = temperature;
  // Add 25C offset.
  tadj += 25 * 512;

  uint8_t buf[4] = {
      (uint8_t)((humidity >> 8) & 0xff), (uint8_t)(humidity & 0xff),
      (uint8_t)((tadj >> 8) & 0xff), (uint8_t)(tadj & 0xff)};

  return i2c_write(CCS811_ENV_DATA, buf, sizeof(buf));
}

int CCS811_Base::update_data() {
  uint8_t buf[5];
  auto ret = i2c_read(CCS811_ALG_RESULT_DATA, buf, sizeof(buf));

  if (ret == CCS811_OK) {
    if (buf[4] & 1) {
      ret = CCS811_SENSOR_ERROR;
    } else {
      eCO2_ = buf[0];
      eCO2_ <<= 8;
      eCO2_ |= buf[1];

      tvoc_ = buf[2];
      tvoc_ <<= 8;
      tvoc_ |= buf[3];
    }
  }

  return ret;
}

int CCS811_Base::get_sensor_error(uint8_t &value) {
  return i2c_read(CCS811_ERROR_ID, &value, sizeof(value));
}
