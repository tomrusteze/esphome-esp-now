#include "esphome.h"
#include "MeshRC.h"

// Sending Side
// Supported Light objects: Binary, Monochromatic, RGB
class esp_now_light : public Component, public LightOutput
{
protected:
  std::string command = "";
  bool updated = false;
  uint8_t dest[6];
  int number;

  bool use_uart = true;
  uart::UARTComponent *uart;

#ifdef USE_POWER_SUPPLY
  power_supply::PowerSupplyRequester power_{};
#endif

  void send_command(std::string newCommand, LightState *state)
  {
    // Send the command over esp-now
    if (this->command != newCommand)
    {
#ifdef USE_POWER_SUPPLY
      state->remote_values.is_on() ? this->power_.request() : this->power_.unrequest();
#endif

      if (this->use_uart)
      {
        for (int i = 0; i < 6; i++)
        {
          String s = String(this->dest[i]);
          this->uart->write_str(s.c_str());
          this->uart->write_byte(' ');
        }
        this->uart->write_str(newCommand.c_str());
        this->uart->write_byte('\n');
      }
      else
      {
        MeshRC::send(this->dest, newCommand);
      }

      this->command = newCommand;
    }
  }

public:
  esp_now_light(uint8_t *mac_address, int number)
  {
    memcpy(this->dest, mac_address, 6);
    this->number = number;
  }

  void set_uart(uart::UARTComponent *uart)
  {
    this->use_uart = true;
    this->uart = uart;
  }

#ifdef USE_POWER_SUPPLY
  void set_power_supply(power_supply::PowerSupply *power_supply)
  {
    this->power_.set_parent(power_supply);
  }
#endif

  // Setup ESP-NOW must be done after Wi-Fi
  float get_setup_priority() const override
  {
    return esphome::setup_priority::AFTER_WIFI;
  }

  void setup() override
  {
    MeshRC::begin();
    MeshRC::setAddr(dest);
  }

  LightTraits get_traits() override
  {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::ON_OFF});
    return traits;
  }

  std::string toFormat(bool binary, std::string effect, char delimiter)
  {
    // Put all the light parameter in a proper format
    return ">SETLIGHT" + to_string(number).substr(0, 1) + char(binary) + delimiter + effect;
  }

  void write_state(LightState *state) override
  {
    // Get the light values:
    bool binary;
    state->remote_values.as_binary(&binary);

    // Process the light values:
    std::string newCommand = toFormat(binary, state->get_effect_name(), ';');
    send_command(newCommand, state);
  }
};

class esp_now_light_RGB : public esp_now_light
{
public:
  esp_now_light_RGB(uint8_t *mac_address, int number) : esp_now_light(mac_address, number){};

  LightTraits get_traits() override
  {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::RGB, ColorMode::BRIGHTNESS});
    return traits;
  }

  std::string toFormat(float red, float green, float blue, float brightness, uint32_t transition, std::string effect, char delimiter)
  {
    return ">SETLIGHT" + to_string(number).substr(0, 1) + to_string(red).substr(0, 4) + delimiter + to_string(green).substr(0, 4) + delimiter + to_string(blue).substr(0, 4) + delimiter + to_string(brightness).substr(0, 4) + delimiter + to_string(transition).substr(0, 4) + delimiter + effect;
  }

  void write_state(LightState *state) override
  {
    // Get the light values:
    float red, green, blue, brightness;
    uint32_t transition = state->get_default_transition_length();
    state->remote_values.as_rgb(&red, &green, &blue);
    state->remote_values.as_brightness(&brightness);

    // Process the light values:
    std::string newCommand = toFormat(red, green, blue, brightness, transition, state->get_effect_name(), ';');
    send_command(newCommand, state);
  }
};

class esp_now_light_RGB_serial : public esp_now_light_RGB
{
public:
  esp_now_light_RGB_serial(uint8_t *mac_address, int number) : esp_now_light_RGB(mac_address, number){};

  void send_command(std::string newCommand, LightState *state)
  {
    // Send the command over esp-now
    if (command != newCommand)
    {
#ifdef USE_POWER_SUPPLY
      state->remote_values.is_on() ? this->power_.request() : this->power_.unrequest();
#endif
      MeshRC::send(dest, newCommand);
      command = newCommand;
    }
  }
};

class esp_now_light_RGBWW : public esp_now_light
{
public:
  esp_now_light_RGBWW(uint8_t *mac_address, int number) : esp_now_light(mac_address, number){};

  LightTraits get_traits() override
  {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::RGB_COLD_WARM_WHITE, ColorMode::BRIGHTNESS});
    return traits;
  }

  std::string toFormat(float red, float green, float blue, float white_1, float white_2, float brightness, uint32_t transition, std::string effect, char delimiter)
  {
    return ">SETLIGHT" + to_string(number).substr(0, 1) + to_string(red).substr(0, 4) + delimiter + to_string(green).substr(0, 4) + delimiter + to_string(blue).substr(0, 4) + delimiter + to_string(white_1).substr(0, 4) + delimiter + to_string(white_2).substr(0, 4) + delimiter + to_string(brightness).substr(0, 4) + delimiter + to_string(transition).substr(0, 4) + delimiter + effect;
  }

  void write_state(LightState *state) override
  {
    // Get the light values:
    float red, green, blue, white_1, white_2, brightness;
    uint32_t transition = state->get_default_transition_length();
    state->remote_values.as_rgbww(&red, &green, &blue, &white_2, &white_1);
    brightness = state->remote_values.get_color_brightness();
    // Process the light values:
    std::string newCommand = toFormat(red, green, blue, white_1, white_2, brightness, transition, state->get_effect_name(), ';');
    send_command(newCommand, state);
  }
};

class esp_now_light_RGBW : public esp_now_light
{
public:
  esp_now_light_RGBW(uint8_t *mac_address, int number) : esp_now_light(mac_address, number){};

  LightTraits get_traits() override
  {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::RGB_WHITE, ColorMode::BRIGHTNESS});
    return traits;
  }

  std::string toFormat(float red, float green, float blue, float white, float brightness, uint32_t transition, std::string effect, char delimiter)
  {
    return ">SETLIGHT" + to_string(number).substr(0, 1) + to_string(red).substr(0, 4) + delimiter + to_string(green).substr(0, 4) + delimiter + to_string(blue).substr(0, 4) + delimiter + to_string(white).substr(0, 4) + delimiter + to_string(brightness).substr(0, 4) + delimiter + to_string(transition).substr(0, 4) + delimiter + effect;
  }

  void write_state(LightState *state) override
  {
    // Get the light values:
    float red, green, blue, white, brightness;
    uint32_t transition = state->get_default_transition_length();
    state->remote_values.as_rgbw(&red, &green, &blue, &white);
    brightness = state->remote_values.get_color_brightness();
    // Process the light values:
    std::string newCommand = toFormat(red, green, blue, white, brightness, transition, state->get_effect_name(), ';');
    send_command(newCommand, state);
  }
};

class esp_now_light_Monochromatic : public esp_now_light
{
public:
  esp_now_light_Monochromatic(uint8_t *mac_address, int number) : esp_now_light(mac_address, number){};

  LightTraits get_traits() override
  {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::BRIGHTNESS});
    return traits;
  }

  std::string toFormat(float brightness, uint32_t transition, std::string effect, char delimiter)
  {
    return ">SETLIGHT" + to_string(number).substr(0, 1) + to_string(brightness).substr(0, 4) + delimiter + to_string(transition).substr(0, 4) + delimiter + effect;
  }

  void write_state(LightState *state) override
  {
    // Get the light values:
    float brightness;
    uint32_t transition = state->get_default_transition_length();
    state->remote_values.as_brightness(&brightness);

    // Process the light values:
    std::string newCommand = toFormat(brightness, transition, state->get_effect_name(), ';');
    send_command(newCommand, state);
  }
};

// Receiving Side:
float parseFloat(String &data, char delimiter)
{
  int pos = data.indexOf(delimiter);
  float value = data.substring(0, pos).toFloat();
  data.remove(0, pos + 1);
  return value;
}

int parseInt(String &data, char delimiter)
{
  int pos = data.indexOf(delimiter);
  int value = data.substring(0, pos).toInt();
  data.remove(0, pos + 1);
  return value;
}

String parseString(String &data, char delimiter)
{
  int pos = data.indexOf(delimiter);
  String value = data.substring(0, pos);
  data.remove(0, pos + 1);
  return value;
}

void parseLight(uint8_t *data, uint8_t size, LightState *dest)
{
  ESP_LOGD("custom", "Receiving Light command");
  // Process data
  String values = "";
  for (auto i = 0; i < size; i++)
    values.concat((const char)data[i]);

  bool binary = parseInt(values, ';');
  String effect = parseString(values, ';');

  // Change Light
  if (binary)
  {
    auto call = dest->turn_on();
    call.set_effect(effect.c_str());
    call.perform();
  }
  else
  {
    auto call = dest->turn_off();
    call.perform();
  }
}

void parseLightMonochromatic(uint8_t *data, uint8_t size, LightState *dest)
{
  ESP_LOGD("custom", "Receiving Monochromatic Light command");
  // Process data
  String values = "";
  for (auto i = 0; i < size; i++)
    values.concat((const char)data[i]);
  float brightness = parseFloat(values, ';');
  float transition = parseFloat(values, ';');
  String effect = parseString(values, ';');

  // Change Light
  auto call = dest->turn_on();
  call.set_brightness(brightness);
  call.set_transition_length(transition);
  call.set_effect(effect.c_str());
  call.perform();
}

void parseLightRGB(uint8_t *data, uint8_t size, AddressableLightState *dest)
{
  ESP_LOGD("custom", "Receiving RGB Light command");
  // Process data
  String values = "";
  for (auto i = 0; i < size; i++)
    values.concat((const char)data[i]);

  float red = parseFloat(values, ';');
  float green = parseFloat(values, ';');
  float blue = parseFloat(values, ';');
  float brightness = parseFloat(values, ';');
  float transition = parseFloat(values, ';');
  String effect = parseString(values, ';');

  // Change Light
  auto call = dest->turn_on();
  call.set_rgb(red, green, blue);
  call.set_brightness(brightness);
  call.set_transition_length(transition);
  call.set_effect(effect.c_str());
  call.perform();
}

void parseLightRGBWW(uint8_t *data, uint8_t size, LightState *dest)
{
  ESP_LOGD("custom", "Receiving RGBWW Light command");
  // Process data
  String values = "";
  for (auto i = 0; i < size; i++)
    values.concat((const char)data[i]);

  float red = parseFloat(values, ';');
  float green = parseFloat(values, ';');
  float blue = parseFloat(values, ';');
  float white_1 = parseFloat(values, ';');
  float white_2 = parseFloat(values, ';');
  float brightness = parseFloat(values, ';');
  float transition = parseFloat(values, ';');
  String effect = parseString(values, ';');

  // Change Light
  auto call = dest->turn_on();
  call.set_rgb(red, green, blue);
  call.set_warm_white(white_1);
  call.set_cold_white(white_2);
  call.set_brightness(brightness);
  call.set_transition_length(transition);
  call.set_effect(effect.c_str());
  call.perform();
}

void parseLightRGBW_separate(uint8_t *data, uint8_t size, AddressableLightState *dest_1, LightState *dest_2)
{
  ESP_LOGD("custom", "Receiving RGB Light command");
  // Process data
  String values = "";
  for (auto i = 0; i < size; i++)
    values.concat((const char)data[i]);

  float red = parseFloat(values, ';');
  float green = parseFloat(values, ';');
  float blue = parseFloat(values, ';');
  float white = parseFloat(values, ';');
  float brightness = parseFloat(values, ';');
  float transition = parseFloat(values, ';');
  String effect = parseString(values, ';');

  // Change the RGB light
  auto call_1 = dest_1->turn_on();
  call_1.set_rgb(red, green, blue);
  if (red == 0 && green == 0 && blue == 0)
  {
    call_1.set_brightness(0);
  }
  else
  {
    call_1.set_brightness(brightness);
  }
  call_1.set_transition_length(transition);
  call_1.set_effect(effect.c_str());
  call_1.perform();

  // Change the Monochromatic Light
  auto call_2 = dest_2->turn_on();
  call_2.set_brightness(white);
  call_2.set_transition_length(transition);
  call_2.set_effect(effect.c_str());
  call_2.perform();
}

// Ping function for light nodes
void ping()
{
  uint8_t dest[6];
  get_mac_address_raw(dest);
  std::string command = "<PING";
  MeshRC::send(dest, command);
}
