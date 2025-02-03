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

  bool get_digital_volume(uint8_t* raw_volume);

 protected:
   GPIOPin *enable_pin_{nullptr};

   bool configure_registers();
   bool set_digital_volume(uint8_t raw_volume);

   bool tas5805m_write_byte(uint8_t a_register, uint8_t data);
   bool tas5805m_read_byte(uint8_t a_register, uint8_t* data);

   enum ErrorCode {
     NONE = 0,
     WRITE_REGISTER_FAILED,
     READ_REGISTER_FAILED
   } error_code_{NONE};

   bool deep_sleep_mode_{false};

   float volume_{0};

   uint8_t raw_volume_{48}; // tas5805 volume 0-254 == 24 to -103 db; 48 = 0db
   uint8_t i2c_error_{0};

   uint16_t number_registers_configured_{0};

};

}  // namespace tas5805m
}  // namespace esphome
