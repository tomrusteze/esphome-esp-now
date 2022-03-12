#include "esphome.h"
#include "MeshRC.h"

class esp_now_light : public Component, public LightOutput {
 private:
  std::string command = "";
  bool updated = false;
  uint8_t dest[6];
  int number;
#ifdef USE_POWER_SUPPLY
  power_supply::PowerSupplyRequester power_{};
#endif  

 public:
  esp_now_light(uint8_t *mac_address, int number){
    memcpy(dest, mac_address,6);
    this->number = number;
  }

#ifdef USE_POWER_SUPPLY
  void set_power_supply(power_supply::PowerSupply *power_supply) { this->power_.set_parent(power_supply); }
#endif

  // Setup ESP-NOW must be done after Wi-Fi
  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  void setup() override {
    MeshRC::begin();    
  }
  
  LightTraits get_traits() override {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::RGB, ColorMode::BRIGHTNESS});
    return traits;
  }
	
  std::string toFormat(float red, float green, float blue, float brightness, uint32_t transition, std::string effect, char delimiter)
  {
		return ">SETLIGHT" + to_string(number).substr(0,1) + to_string(red).substr(0,4) + delimiter + to_string(green).substr(0,4) + delimiter + to_string(blue).substr(0,4)
               + delimiter + to_string(brightness).substr(0,4) + delimiter + to_string(transition).substr(0,4) + delimiter + effect;
  }

  void write_state(LightState *state) override {
    // This will be called by the light to get a new state to be written.
    float red, green, blue, brightness;
    uint32_t transition = state->get_default_transition_length();
    // use any of the provided current_values methods
    //state->current_values_as_rgb(&red, &green, &blue);
    
    state->remote_values.as_rgb(&red, &green, &blue);
    state->remote_values.as_brightness(&brightness);
    std::string newCommand = toFormat(red, green, blue, brightness, transition, state->get_effect_name(),';');
    // Write red, green and blue to HW
    if(command != newCommand)
    {
#ifdef USE_POWER_SUPPLY
      state->remote_values.is_on() ? this->power_.request() : this->power_.unrequest();
#endif
      MeshRC::send(dest, 6, newCommand);
      command = newCommand;
    }
  }
};

float parseFloat(String &data, char delimiter)
{
  int pos = data.indexOf(';');
  float value = data.substring(0, pos).toFloat();
  data.remove(0, pos+1);
  return value;
}

int parseInt(String &data, char delimiter)
{
  int pos = data.indexOf(';');
  int value = data.substring(0, pos).toInt();
  data.remove(0, pos+1);
  return value;
}

String parseString(String &data, char delimiter)
{
  int pos = data.indexOf(';');
  String value = data.substring(0, pos);
  data.remove(0, pos+1);
  return value;
}

void parseLightRGB(uint8_t* data, uint8_t size, AddressableLightState* dest)
{
  ESP_LOGD("custom", "Receiving Light command");

  String values = "";
  for (auto i=0; i<size; i++) 
    values.concat((const char)data[i]);

  float red = parseFloat(values, ';');
  float green = parseFloat(values, ';');
  float blue = parseFloat(values, ';');
  float brightness = parseFloat(values, ';');
  float transition = parseFloat(values, ';');
  String effect = parseString(values, ';');

  auto call = dest->turn_on();
  call.set_rgb(red, green, blue);
  call.set_brightness(brightness);
  call.set_transition_length(transition); 
  call.set_effect(effect.c_str());
  call.perform();
}


