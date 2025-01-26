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
static const uint8_t AGAIN_REGISTER         = 0x54;

void Tas5805mComponent::setup() {
  if (this->enable_pin_ != nullptr) {
    // Set enable pin as OUTPUT and disable the enable pin
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(false);
    delay(10);
    this->enable_pin_->digital_write(true);
    //delay(10);
  }

  this->set_timeout(100, [this]() {
      if (!configure_registers()) {
        this->error_code_ = WRITE_REGISTER_FAILED;
        this->mark_failed();
      }
  });

  // uint16_t number_configurations = sizeof(tas5805m_registers) / sizeof(tas5805m_registers[0]);
  // if (!configure_registers(number_configurations)) {
  //   this->error_code_ = WRITE_REGISTER_FAILED;
  //   this->mark_failed();
  //   return;
  // }

  // //this->set_gain(2);
  // this->set_raw_volume(47);
}

bool Tas5805mComponent::configure_registers() {
  uint16_t i, counter = 0;
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
  this->number_configuration_registers_ = counter;
  return true;
}

void Tas5805mComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas5805m:");

  switch (this->error_code_) {
    case WRITE_REGISTER_FAILED:
      ESP_LOGE(TAG, "  Write register failed with error code = %i",this->last_i2c_error_);
      break;
    case NONE:
      ESP_LOGD(TAG, "  Registers configured: %i", this->number_configuration_registers_);
      ESP_LOGD(TAG, "  Setup successful");
      LOG_I2C_DEVICE(this);
      break;
  }
  uint8_t volume;
  if (!this->get_raw_volume(&volume)) {
    ESP_LOGD(TAG, "  error reading volume");
  } else {
    ESP_LOGD(TAG, "  tas5805m volume = %i",volume);
  }
  // uint8_t gain;
  // if (!this->get_gain(&gain)) {
  //   ESP_LOGD(TAG, "  error reading gain");
  // } else {
  //   ESP_LOGD(TAG, "  tas5805m gain = %i",gain);
  // }
}


bool Tas5805mComponent::set_volume(float value) {
  uint8_t raw = (uint8_t)((100.0-(value*100.0)) * 2.54);
  ESP_LOGD(TAG, "  tas5805m volume = %i",raw);
  return true;
}

bool Tas5805mComponent::set_mute_off() {
  if (!this->tas5805m_write_byte(DIG_VOL_CTRL_REGISTER, this->last_raw_volume_)) return false;
  this->is_muted_ = false;
  return true;
}

bool Tas5805mComponent::set_mute_on() {
  uint8_t raw;
  if (!this->get_raw_volume(&raw)) return false;
  if (!this->tas5805m_write_byte(DIG_VOL_CTRL_REGISTER, 0xFF)) return false;
  this->last_raw_volume_ = raw;
  this->is_muted_ = true;
  return true;
}

void Tas5805mComponent::set_tas5805m_state(bool deep_sleep) {
  uint8_t mode = deep_sleep ? 0x00 : 0x03; // device state Deep Sleep or PLAY
  this->tas5805m_write_byte(DEVICE_CTRL_2_REGISTER, mode);
}

// 0-255, where 0 = 0 Db, 255 = -15.5 Db
// bool Tas5805mComponent::get_gain(uint8_t* value) {
//     *value = 0;
//     uint8_t raw;
//     if (!this->tas5805m_read_byte(AGAIN_REGISTER, &raw)) return false;
//     // remove top 3 reserved bits
//     *value = raw & 0x1F;
//     return true;
// }

// Analog Gain Control , with 0.5dB one step
// lower 5 bits controls the analog gain.
// 00000: 0 dB (29.5V peak voltage)
// 00001: -0.5db
// 11111: -15.5 dB
// bool Tas5805mComponent::set_gain(uint8_t value) {
//   uint8_t raw;
//   this->get_gain(&raw);
//   // keep top 3 reserved bits combine with bottom 5 gain bits
//   value = (raw & 0xE0) | (value & 0x1F);
//   return this->tas5805m_write_byte(AGAIN_REGISTER, value);
// }

bool Tas5805mComponent::get_raw_volume(uint8_t* volume) {
  *volume = 0;
  uint8_t raw;
  if(!this->tas5805m_read_byte(DIG_VOL_CTRL_REGISTER, &raw)) return false;
  *volume = raw;
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
bool Tas5805mComponent::set_raw_volume(uint8_t value) {
  return this->tas5805m_write_byte(DIG_VOL_CTRL_REGISTER, value);
}

bool Tas5805mComponent::tas5805m_write_byte(uint8_t a_register, uint8_t data) {
    i2c::ErrorCode error_code = this->write_register(a_register, &data, 1, true);
    if (error_code != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "  write register error %i", error_code);
      this->last_i2c_error_ = (uint8_t)error_code;
      return false;
    }
    return true;
}

bool Tas5805mComponent::tas5805m_read_byte(uint8_t a_register, uint8_t* data) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "  read register - first write error %i", error_code);
    this->last_i2c_error_ = (uint8_t)error_code;
    return false;
  }
  error_code = this->read_register(a_register, data, 1, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "  read register error %i", error_code);
    this->last_i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace tas5805m
}  // namespace esphome
