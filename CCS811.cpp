#include "CCS811.h"

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

int CCS811::init() {
  const char sw_reset[] = {CCS811_SW_RESET, 0x11, 0xE5, 0x72, 0x8A};
  char byte;

  // Reset controller.
  if (bus_.write(addr_, sw_reset, sizeof(sw_reset)) != 0) {
    return CCS811_IOERROR;
  }

  ThisThread::sleep_for(2ms);

  // Check hardware id and version.
  if (i2c_read(CCS811_HW_ID, &byte, sizeof(byte)) != 0) {
    return CCS811_IOERROR;
  }

  if (byte != 0x81) {
    return CCS811_NOT_FOUND;
  }

  if (i2c_read(CCS811_HW_VERSION, &byte, sizeof(byte)) != 0) {
    return CCS811_IOERROR;
  }

  if ((byte & 0xf0) != 0x10) {
    return CCS811_NOT_FOUND;
  }

  // Check that device is in boot mode after reset.
  if (i2c_read(CCS811_STATUS, &byte, sizeof(byte)) != 0) {
    return CCS811_IOERROR;
  }

  if (byte != 0x10) {
    return CCS811_INVALID_STATE;
  }

  // Switch to app mode.
  byte = CCS811_APP_START;
  if (bus_.write(addr_, &byte, sizeof(byte)) != 0) {
    return CCS811_IOERROR;
  }
  ThisThread::sleep_for(1ms);

  // Check that device is in app mode.
  if (i2c_read(CCS811_STATUS, &byte, sizeof(byte)) != 0) {
    return CCS811_IOERROR;
  }

  if (byte != 0x90) {
    return CCS811_INVALID_STATE;
  }

  return CCS811_OK;
}

int CCS811::setMode(CCS811::Mode mode) {
  char cmd[2] = {CCS811_MEAS_MODE, 0};

  switch (mode) {
    case Mode::IDLE:
      cmd[1] = CCS811_MODE_IDLE << 4;
      break;
    case Mode::EVERY_1_S:
      cmd[1] = CCS811_MODE_1SEC << 4;
      break;
    case Mode::EVERY_10_S:
      cmd[1] = CCS811_MODE_10SEC << 4;
      break;
    case Mode::EVERY_60_S:
      cmd[1] = CCS811_MODE_60SEC << 4;
      break;
  }

  if (bus_.write(addr_, cmd, sizeof(cmd)) != 0) {
    return CCS811_IOERROR;
  }

  return CCS811_OK;
}

int CCS811::getBaseline(uint16_t &value) {
  auto p = reinterpret_cast<char *>(&value);
  if (i2c_read(CCS811_BASELINE, p, sizeof(value)) != 0) {
    return CCS811_IOERROR;
  }

  return CCS811_OK;
}

int CCS811::setBaseline(uint16_t value) {
  auto p = reinterpret_cast<const char *>(&value);
  char cmd = CCS811_BASELINE;
  if (bus_.write(addr_, &cmd, sizeof(cmd), true) != 0) {
    bus_.stop();
    return CCS811_IOERROR;
  }

  bus_.write(p[0] & 0xff);
  bus_.write(p[0] & 0xff);
  bus_.stop();

  return CCS811_OK;
}

int CCS811::setEnvData(uint16_t humidity, uint16_t temperature) {
  char buf[5] = {CCS811_ENV_DATA, ((humidity >> 8) & 0xff), (humidity & 0xff),
                 ((temperature >> 8) & 0xff), (temperature & 0xff)};

  if (bus_.write(addr_, buf, sizeof(buf)) != 0) {
    return CCS811_IOERROR;
  }

  return CCS811_OK;
}

int CCS811::refreshData() {
  char buf[5];
  if (i2c_read(CCS811_ALG_RESULT_DATA, buf, sizeof(buf)) != 0) {
    return CCS811_IOERROR;
  }

  if (buf[4] & 0x01) {
    return CCS811_SENSOR_ERROR;
  }

  eCO2_ = buf[0];
  eCO2_ <<= 8;
  eCO2_ |= buf[1];

  tvoc_ = buf[2];
  tvoc_ <<= 8;
  tvoc_ |= buf[3];

  if (!(buf[4] & 0x08)) {
    return CCS811_STALE_DATA;
  }

  return CCS811_OK;
}

int CCS811::getSensorError(uint8_t &value) {
  return i2c_read(CCS811_ERROR_ID, (char *)&value, sizeof(value));
}

int CCS811::i2c_read(uint8_t reg_addr, char *buf, size_t len) {
  if (bus_.write(addr_, (const char *)&reg_addr, sizeof(reg_addr)) != 0) {
    return -1;
  }

  return bus_.read(addr_, buf, len);
}
