#include "tas5805m.h"
#include "tas5805m_config_minimal.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tas5805m {

static const char *const TAG = "tas5805m";

// tas5805m registers
static const uint8_t DEVICE_CTRL_2_REGISTER = 0x03; // Device state control register
static const uint8_t DIG_VOL_CTRL_REGISTER  = 0x4c;

void Tas5805mComponent::setup() {
  if (this->enable_pin_ != nullptr) {
    // Set enable pin as OUTPUT and disable the enable pin
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(false);
    delay(10);
    this->enable_pin_->digital_write(true);
  }

  this->set_timeout(100, [this]() {
      if (!configure_registers()) {
        this->error_code_ = WRITE_REGISTER_FAILED;
        this->mark_failed();
      }
      uint8_t raw;
      if (!this->get_digital_volume(&raw)) {
        this->error_code_ = READ_REGISTER_FAILED;
        this->mark_failed();
      }
      else {
        this->raw_volume_ = raw;
      }
  });
}

bool Tas5805mComponent::configure_registers() {
  uint16_t i = 0;
  uint16_t counter = 0;
  uint16_t number_configurations = sizeof(tas5805m_registers) / sizeof(tas5805m_registers[0]);

  while (i < number_configurations) {
    switch (tas5805m_registers[i].offset) {
      case CFG_META_DELAY:
        delay(tas5805m_registers[i].value);
        break;
      default:
        if (!this->tas5805m_write_byte(tas5805m_registers[i].offset, tas5805m_registers[i].value)) return false;
        counter++;
        break;
    }
    i++;
  }
  this->number_registers_configured_ = counter;
  return true;
}

void Tas5805mComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas5805m:");
  uint8_t volume;

  switch (this->error_code_) {
    case WRITE_REGISTER_FAILED:
      ESP_LOGE(TAG, "  Write register failure with error code = %i",this->i2c_error_);
      break;
    case READ_REGISTER_FAILED:
      ESP_LOGE(TAG, "  Read register failure with error code = %i",this->i2c_error_);
      break;
    case NONE:
      ESP_LOGD(TAG, "  Registers configured: %i", this->number_registers_configured_);
      ESP_LOGD(TAG, "  Digital volume control: %i", this->raw_volume_);
      ESP_LOGD(TAG, "  Setup successful");
      LOG_I2C_DEVICE(this);
      break;
  }
}

bool Tas5805mComponent::set_volume(float volume) {
  this->volume_ = clamp<float>(volume, 0.0, 1.0);
  //uint8_t raw = (uint8_t)((1.0 - this->volume_) * 254);
  uint8_t raw = remap<uint8_t, float>(this->volume_, 0.0f, 1.0f, 254, 0);
  //this->set_digital_volume(raw);
  ESP_LOGD(TAG, "  raw digital volume = %i", raw);
  return true;
}

bool Tas5805mComponent::set_mute_off() {
  if (!this->tas5805m_write_byte(DIG_VOL_CTRL_REGISTER, this->raw_volume_)) return false;
  this->is_muted_ = false;
  ESP_LOGD(TAG, "  tas5805m mute off");
  return true;
}

bool Tas5805mComponent::set_mute_on() {
  uint8_t raw;
  if (!this->get_digital_volume(&raw)) return false;
  if (!this->tas5805m_write_byte(DIG_VOL_CTRL_REGISTER, 0xFF)) return false;
  this->raw_volume_ = raw;
  this->is_muted_ = true;
  ESP_LOGD(TAG, "  tas5805m mute on");
  return true;
}

bool Tas5805mComponent::set_deep_sleep_on() {
  if (this->deep_sleep_mode_) return true; //already in deep sleep
  this->deep_sleep_mode_ = this->tas5805m_write_byte(DEVICE_CTRL_2_REGISTER, 0x00);
  ESP_LOGD(TAG, "  tas5805m deep sleep on");
  return this->deep_sleep_mode_;
}

bool Tas5805mComponent::set_deep_sleep_off() {
  if (!this->deep_sleep_mode_) return true; // already not in deep sleep
  this->deep_sleep_mode_ = (!this->tas5805m_write_byte(DEVICE_CTRL_2_REGISTER, 0x03));
  ESP_LOGD(TAG, "  tas5805m deep sleep off");
  return this->deep_sleep_mode_;
}

bool Tas5805mComponent::get_digital_volume(uint8_t* raw_volume) {
  uint8_t raw;
  if(!this->tas5805m_read_byte(DIG_VOL_CTRL_REGISTER, &raw)) return false;
  *raw_volume = raw;
  return true;
}

// controls both left and right channel digital volume
// digital volume is 24 dB to -103 dB in -0.5 dB step
// 00000000: +24.0 dB
// 00000001: +23.5 dB
// 00101111: +0.5 dB
// 00110000: 0.0 dB
// 00110001: -0.5 dB
// 11111110: -103 dB
// 11111111: Mute
bool Tas5805mComponent::set_digital_volume(uint8_t raw_volume) {
  ESP_LOGD(TAG, "  tas5805m set digital volume %i", raw_volume);
  return this->tas5805m_write_byte(DIG_VOL_CTRL_REGISTER, raw_volume);
}

bool Tas5805mComponent::tas5805m_write_byte(uint8_t a_register, uint8_t data) {
    i2c::ErrorCode error_code = this->write_register(a_register, &data, 1, true);
    if (error_code != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "  write register error %i", error_code);
      this->i2c_error_ = (uint8_t)error_code;
      return false;
    }
    return true;
}

bool Tas5805mComponent::tas5805m_read_byte(uint8_t a_register, uint8_t* data) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "  read register - first write error %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  error_code = this->read_register(a_register, data, 1, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "  read register error %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace tas5805m
}  // namespace esphome
