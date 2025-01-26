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
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  void set_tas5805m_state(bool deep_sleep);
  //bool get_gain(uint8_t* value);
  //bool set_gain(uint8_t value);

  float volume() override {return (float)this->last_raw_volume_; }
  bool set_volume(float value) override;
  bool set_mute_off() override;

  bool set_mute_on() override;

  bool is_muted() override { return this->is_muted_; }

 protected:
   GPIOPin *enable_pin_{nullptr};

   bool configure_registers();
   bool set_raw_volume(uint8_t value);
   bool get_raw_volume(uint8_t* volume);
   bool tas5805m_write_byte(uint8_t a_register, uint8_t data);
   bool tas5805m_read_byte(uint8_t a_register, uint8_t* data);

   enum ErrorCode {
     NONE = 0,
     WRITE_REGISTER_FAILED,
   } error_code_{NONE};

   int8_t last_raw_volume_{48}; // tas5805 volume 0-254 == 24 to -103 db; 48 = 0db
   uint8_t last_i2c_error_{0};
   uint16_t number_configuration_registers_;
};

}  // namespace tas5805m
}  // namespace esphome
