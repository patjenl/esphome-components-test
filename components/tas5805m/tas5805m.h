#pragma once

#include "esphome/components/audio_dac/audio_dac.h"
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/hal.h"


namespace esphome {
namespace tas5805m {

struct tas5805m_cfg_reg_t {
  uint8_t offset;
  uint8_t value;
};

class Tas5805mComponent : public audio_dac::AudioDac, public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  float volume() override { return this->volume_; }
  bool set_volume(float value) override;

  bool set_mute_off() override;
  bool set_mute_on() override;
  bool is_muted() override { return this->is_muted_; }

  bool set_deep_sleep_off();
  bool set_deep_sleep_on();

 protected:
   GPIOPin *enable_pin_{nullptr};

   bool configure_registers();

   bool get_digital_volume(uint8_t* value);
   bool set_digital_volume(uint8_t new_value);

   bool get_gain(uint8_t* value);
   bool set_gain(uint8_t new_value);

   bool tas5805m_write_byte(uint8_t a_register, uint8_t data);
   bool tas5805m_read_byte(uint8_t a_register, uint8_t* data);

   enum ErrorCode {
     NONE = 0,
     CONFIGURATION_FAILED,
     WRITE_REGISTER_FAILED
   } error_code_{NONE};

   bool deep_sleep_mode_{false};

   float volume_{0};

   uint8_t i2c_error_{0};
   uint8_t analog_gain_{0};
   uint8_t digital_volume_{0};

   uint16_t number_registers_configured_{0};
};

}  // namespace tas5805m
}  // namespace esphome
