#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#ifdef USE_ESP_NOW_LIGHT_OUTPUT
#include "esphome/components/light/light_output.h"
#endif
#if defined(USE_ESP_NOW_LIGHT_OUTPUT) || defined(USE_ESP_NOW_LIGHT_RECEIVER)
#include "esphome/components/light/light_state.h"
#endif

#ifdef USE_ESPNOW
#include "esphome/components/espnow/espnow_component.h"
#endif
#ifdef USE_POWER_SUPPLY
#include "esphome/components/power_supply/power_supply.h"
#endif
#if defined(USE_ESP_NOW_LIGHT_UART) || defined(USE_ESP_NOW_LIGHT_UART_SENSOR)
#include "esphome/components/uart/uart.h"
#endif
#ifdef USE_ESP_NOW_LIGHT_UART_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#include <array>
#include <string>
#include <vector>

namespace esphome::esp_now_light {

enum class LightType : uint8_t { BINARY, MONOCHROMATIC, RGB, RGBW, RGBWW };
using MACAddress = std::array<uint8_t, 6>;

#ifdef USE_ESP_NOW_LIGHT_OUTPUT
class ESPNowLightOutput : public light::LightOutput {
 public:
  ESPNowLightOutput(MACAddress address, uint8_t number, LightType type)
      : address_(address), number_(number), type_(type) {}

  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

#ifdef USE_ESPNOW
  void set_transport(espnow::ESPNowComponent *transport) { this->transport_ = transport; }
#endif
#ifdef USE_ESP_NOW_LIGHT_UART
  void set_uart(uart::UARTComponent *uart) { this->uart_ = uart; }
#endif
#ifdef USE_POWER_SUPPLY
  void set_power_supply(power_supply::PowerSupply *supply) { this->power_.set_parent(supply); }
#endif

 protected:
  bool send_(const std::string &command);

  MACAddress address_;
  uint8_t number_;
  LightType type_;
  std::string last_command_;
#ifdef USE_ESPNOW
  espnow::ESPNowComponent *transport_{nullptr};
#endif
#ifdef USE_ESP_NOW_LIGHT_UART
  uart::UARTComponent *uart_{nullptr};
#endif
#ifdef USE_POWER_SUPPLY
  power_supply::PowerSupplyRequester power_;
#endif
};

#endif

#ifdef USE_ESP_NOW_LIGHT_RECEIVER
class ESPNowLightReceiver : public Component,
                            public espnow::ESPNowReceivedPacketHandler,
                            public espnow::ESPNowUnknownPeerHandler,
                            public espnow::ESPNowBroadcastHandler {
 public:
  void set_transport(espnow::ESPNowComponent *transport) { this->transport_ = transport; }
  void set_source_address(MACAddress address) { this->source_address_ = address; }
  void add_light(uint8_t number, LightType type, light::LightState *target, light::LightState *white) {
    this->lights_.push_back({number, type, target, white});
  }

  void setup() override;
  bool on_receive(const espnow::ESPNowRecvInfo &info, const uint8_t *data, uint16_t size) override;
  bool on_unknown_peer(const espnow::ESPNowRecvInfo &info, const uint8_t *data, uint16_t size) override {
    return this->on_receive(info, data, size);
  }
  bool on_broadcast(const espnow::ESPNowRecvInfo &info, const uint8_t *data, uint16_t size) override {
    return this->on_receive(info, data, size);
  }

 protected:
  struct ReceiverLight {
    uint8_t number;
    LightType type;
    light::LightState *target;
    light::LightState *white;
  };

  espnow::ESPNowComponent *transport_{nullptr};
  optional<MACAddress> source_address_;
  std::vector<ReceiverLight> lights_;
};
#endif

#ifdef USE_ESP_NOW_LIGHT_UART_SENSOR
class UARTErrorSensor : public sensor::Sensor, public Component, public uart::UARTDevice {
 public:
  void setup() override { this->publish_state(0); }
  void loop() override;

 protected:
  std::string line_;
  bool overflow_{false};
};
#endif

}  // namespace esphome::esp_now_light
