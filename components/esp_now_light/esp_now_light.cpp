#include "esp_now_light.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <string_view>

namespace esphome::esp_now_light {

#if defined(USE_ESP_NOW_LIGHT_OUTPUT) || defined(USE_ESP_NOW_LIGHT_RECEIVER)
static constexpr std::string_view COMMAND_PREFIX = ">SETLIGHT";
static constexpr size_t MAX_COMMAND_SIZE = 250;

static light::ColorMode color_mode(LightType type) {
  switch (type) {
    case LightType::BINARY:
      return light::ColorMode::ON_OFF;
    case LightType::MONOCHROMATIC:
      return light::ColorMode::BRIGHTNESS;
    case LightType::RGB:
      return light::ColorMode::RGB;
    case LightType::RGBW:
      return light::ColorMode::RGB_WHITE;
    case LightType::RGBWW:
      return light::ColorMode::RGB_COLD_WARM_WHITE;
  }
  return light::ColorMode::UNKNOWN;
}

#endif

#ifdef USE_ESP_NOW_LIGHT_OUTPUT
static const char *const TAG = "esp_now_light";

light::LightTraits ESPNowLightOutput::get_traits() {
  light::LightTraits traits;
  traits.set_supported_color_modes({color_mode(this->type_)});
  return traits;
}

static void append_level(std::string &command, float level) {
  const unsigned hundredths = static_cast<unsigned>(level * 100.0f + 0.5f);
  command += static_cast<char>('0' + hundredths / 100);
  command += '.';
  command += static_cast<char>('0' + hundredths / 10 % 10);
  command += static_cast<char>('0' + hundredths % 10);
  command += ';';
}

void ESPNowLightOutput::write_state(light::LightState *state) {
  const auto &values = state->remote_values;
  const bool on = values.is_on();
  std::string command(COMMAND_PREFIX);
  command += static_cast<char>('0' + this->number_);

  if (this->type_ == LightType::BINARY) {
    command += on ? "1;" : "0;";
  } else {
    float brightness = on ? values.get_brightness() : 0.0f;
    if (this->type_ != LightType::MONOCHROMATIC) {
      append_level(command, values.get_red());
      append_level(command, values.get_green());
      append_level(command, values.get_blue());
    }
    if (this->type_ == LightType::RGB) {
      brightness *= values.get_color_brightness();
    } else if (this->type_ == LightType::RGBW) {
      append_level(command, brightness * values.get_white());
    } else if (this->type_ == LightType::RGBWW) {
      // The existing format puts warm white before cold white.
      append_level(command, values.get_warm_white());
      append_level(command, values.get_cold_white());
    }
    append_level(command, brightness);
    command += std::to_string(state->get_default_transition_length());
    command += ';';
  }
  const auto effect = state->get_effect_name();
  command.append(effect.c_str(), effect.size());

#ifdef USE_POWER_SUPPLY
  if (on)
    this->power_.request();
#endif
  if (command != this->last_command_ && this->send_(command))
    this->last_command_ = std::move(command);
#ifdef USE_POWER_SUPPLY
  if (!on)
    this->power_.unrequest();
#endif
}

bool ESPNowLightOutput::send_(const std::string &command) {
  if (command.size() > MAX_COMMAND_SIZE || command.find_first_of("\r\n") != std::string::npos) {
    ESP_LOGW(TAG, "Invalid light command length or effect name");
    return false;
  }
#ifdef USE_ESP_NOW_LIGHT_UART
  if (this->uart_ != nullptr) {
    for (uint8_t byte : this->address_) {
      this->uart_->write_str(std::to_string(byte).c_str());
      this->uart_->write_byte(' ');
    }
    this->uart_->write_str(command.c_str());
    this->uart_->write_byte('\n');
    return true;
  }
#endif
#ifdef USE_ESPNOW
  if (this->transport_ != nullptr &&
      this->transport_->add_peer(this->address_.data()) == 0 &&
      this->transport_->send(this->address_.data(), reinterpret_cast<const uint8_t *>(command.data()),
                             command.size()) == 0)
    return true;
#endif
  ESP_LOGW(TAG, "Could not queue light command");
  return false;
}

#endif

#ifdef USE_ESP_NOW_LIGHT_RECEIVER
template<typename T> static bool read_field(std::string_view &payload, T &value) {
  const auto delimiter = payload.find(';');
  if (delimiter == std::string_view::npos)
    return false;
  const auto field = payload.substr(0, delimiter);
  if (field.empty() || field.find('\0') != std::string_view::npos || field.front() == '-')
    return false;
  auto parsed = parse_number<T>(std::string(field));
  if (!parsed.has_value())
    return false;
  value = *parsed;
  payload.remove_prefix(delimiter + 1);
  return true;
}

static bool read_levels(std::string_view &payload, float *levels, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (!read_field(payload, levels[i]) || !(levels[i] >= 0.0f && levels[i] <= 1.0f))
      return false;
  }
  return true;
}

static bool apply_light(LightType type, light::LightState *target, light::LightState *white, std::string_view payload) {
  if (type == LightType::BINARY) {
    if (payload.size() < 2 || payload[1] != ';')
      return false;
    const char state = payload.front();
    if (state != '0' && state != '1' && state != '\0' && state != '\1')
      return false;
    const bool on = state == '1' || state == '\1';
    auto call = target->make_call();
    call.set_state(on);
    if (on && payload.size() > 2)
      call.set_effect(payload.data() + 2, payload.size() - 2);
    call.perform();
    return true;
  }

  const size_t channels = type == LightType::RGBWW ? 5 : type == LightType::RGBW ? 4 : type == LightType::RGB ? 3 : 0;
  float color[5]{};
  float brightness;
  uint32_t transition;
  if (!read_levels(payload, color, channels) || !read_levels(payload, &brightness, 1) ||
      !read_field(payload, transition))
    return false;

  const float white_level = channels == 4 ? color[3] : 0.0f;
  const bool has_rgb = channels >= 3 && (color[0] > 0 || color[1] > 0 || color[2] > 0);
  const bool on = brightness > 0 && (white == nullptr || has_rgb);
  auto call = target->make_call();
  call.set_state(on);
  if (on) {
    call.set_color_mode_if_supported(white == nullptr ? color_mode(type) : light::ColorMode::RGB);
    call.set_brightness(brightness);
    if (has_rgb)
      call.set_rgb(color[0], color[1], color[2]);
    if (type == LightType::RGBW && white == nullptr)
      call.set_white(std::min(white_level / brightness, 1.0f));
    if (type == LightType::RGBWW) {
      call.set_warm_white(color[3]);
      call.set_cold_white(color[4]);
    }
    if (!payload.empty())
      call.set_effect(payload.data(), payload.size());
  }
  call.set_transition_length_if_supported(transition);
  call.perform();

  if (white != nullptr) {
    auto white_call = white->make_call();
    const bool white_on = brightness > 0 && white_level > 0;
    white_call.set_state(white_on);
    if (white_on)
      white_call.set_brightness(white_level);
    white_call.set_transition_length_if_supported(transition);
    white_call.perform();
  }
  return true;
}

void ESPNowLightReceiver::setup() {
  this->transport_->register_receive_handler(this);
  this->transport_->register_unknown_peer_handler(this);
  this->transport_->register_broadcast_handler(this);
}

bool ESPNowLightReceiver::on_receive(const espnow::ESPNowRecvInfo &info, const uint8_t *data, uint16_t size) {
  if (size <= COMMAND_PREFIX.size() || size > MAX_COMMAND_SIZE ||
      (this->source_address_.has_value() &&
       !std::equal(this->source_address_->begin(), this->source_address_->end(), info.src_addr))) {
    return false;
  }

  std::string_view payload(reinterpret_cast<const char *>(data), size);
  if (!payload.starts_with(COMMAND_PREFIX))
    return false;
  payload.remove_prefix(COMMAND_PREFIX.size());
  const int number = payload.front() - '0';
  payload.remove_prefix(1);
  for (const auto &receiver : this->lights_) {
    if (receiver.number == number)
      return apply_light(receiver.type, receiver.target, receiver.white, payload);
  }
  return false;
}
#endif

#ifdef USE_ESP_NOW_LIGHT_UART_SENSOR
void UARTErrorSensor::loop() {
  uint8_t byte;
  while (this->read_byte(&byte)) {
    if (byte == '\r' || byte == '\n') {
      if (!this->overflow_ && !this->line_.empty()) {
        const auto value = parse_number<uint32_t>(this->line_);
        if (value.has_value())
          this->publish_state(*value);
      }
      this->line_.clear();
      this->overflow_ = false;
    } else if (byte >= '0' && byte <= '9' && this->line_.size() < 10) {
      this->line_ += static_cast<char>(byte);
    } else {
      this->overflow_ = true;
    }
  }
}
#endif

}  // namespace esphome::esp_now_light
