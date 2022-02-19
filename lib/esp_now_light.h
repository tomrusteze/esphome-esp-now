#include "esphome.h"
#include "MeshRC.h"

class esp_now_light : public Component, public LightOutput {
 private:
  std::string command = "";
  String name;
  bool updated = false;
  u8 dest[6];
  
 public:
  esp_now_light(u8 *mac_address){
    memcpy(dest, mac_address,6);
  }
    
  void setup() override {
    MeshRC::begin();    
  }

  std::string toFormat(float red, float green, float blue, float brightness, std::string effect, char delimiter)
  {
		return ">SET" + to_string(red).substr(0,4) + delimiter + to_string(green).substr(0,4) + delimiter + to_string(blue).substr(0,4) + delimiter + to_string(brightness).substr(0,4) + delimiter + effect;
  }
  
  LightTraits get_traits() override {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::RGB, ColorMode::BRIGHTNESS});
    return traits;
  }
	
  void write_state(LightState *state) override {
    // This will be called by the light to get a new state to be written.
    float red, green, blue, brightness;
    // use any of the provided current_values methods
    //state->current_values_as_rgb(&red, &green, &blue);
    state->remote_values.as_rgb(&red, &green, &blue);
    state->remote_values.as_brightness(&brightness);
    std::string newCommand = toFormat(red, green, blue, brightness, state->get_effect_name(),';');
    // Write red, green and blue to HW
    if(command != newCommand)
    {
      MeshRC::send(dest, 6, newCommand);
      command = newCommand;
    }
  }
};
