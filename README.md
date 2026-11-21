# ESP-NOW lights for ESPHome

Control lights on ESP32 or ESP8266 nodes over ESP-NOW. A hub exposes remote lights to Home Assistant, or a physical switch controls a light directly. A receiver can run on a fixed radio channel without joining a Wi-Fi network. An optional UART relay lets the hub use Ethernet or a Wi-Fi channel different from the light nodes.

The project builds on [iphong's ESPHome ESP-NOW integration](https://github.com/iphong/esphome-espnow) and [MeshRC](https://github.com/iphong/lib-esp-rc).

## Install

Use **ESPHome 2026.11.0 or later** on ESP8266 or ESP32. Add this block to each ESPHome device that uses the integration. [ESPHome external components documentation](https://esphome.io/components/external_components/)

```yaml
external_components:
  - source: github://tomrusteze/esphome-esp-now@main
    components: [esp_now_light]
```

For a local checkout, see the [example configurations](examples/README.md).

## Configure a hub and receiver

Set `mac_address` on the hub to the receiver's Wi-Fi station MAC address. The package registers outgoing peers with ESPHome's built-in `espnow` component. A hub that joins Wi-Fi uses the access point's channel. A hub without Wi-Fi needs an explicit `espnow.channel`. [ESPHome ESP-NOW documentation](https://esphome.io/components/espnow/)

```yaml
# Hub entries for a device already configured with wifi:
espnow:
  id: radio

light:
  - platform: esp_now_light
    name: "Garden Light"
    espnow_id: radio
    mac_address: "02:00:00:00:00:01"
    light_type: rgb
    light_number: 1
```

On the receiver, point `light_id` at an existing ESPHome light. Use the same light type and number at both ends. If the receiver joins Wi-Fi, omit `channel` and make sure its access point uses the sender's channel.

```yaml
# Receiver entries for a device that uses ESP-NOW without wifi:
espnow:
  id: radio
  channel: 7

esp_now_light:
  espnow_id: radio
  receivers:
    - light_id: garden_light
      light_type: rgb
      light_number: 1
```

- `light_type` accepts `binary`, `monochromatic`, `rgb`, `rgbw`, and `rgbww`.
- `light_number` defaults to `1` and accepts `1` through `9`. Give each light on the same receiver a different number. Lights on different devices can reuse a number.
- `espnow_id` is optional on the receiver when there is only one `espnow` instance. Set `source_address` on the receiver to accept commands from a specific transmitter MAC.
- For RGBW, either target a single RGBW light or set `white_light_id` to a separate monochromatic light alongside the RGB target.
- Define effects on the physical light. To expose those effects on the hub, add automation effects with the same names and a no-op sequence such as `delay: 0ms`. The sender forwards the effect name and the configured default transition length for dimmable lights.
- An optional `power_supply` on each hub light holds an ESPHome power supply on while the light is on. Set its `enable_time` to allow the receiver to boot and its `keep_on_time` to allow an off command and transition to finish.

The hub state is optimistic. A change made on the receiver is independent of the hub's state.

## UART relay

Flash [serial-to-espnow.ino](serial-to-espnow/serial-to-espnow.ino) with the Arduino ESP8266 core. Set the channel in `MeshRC::setupwifi(7)` to match the receivers. Connect the hub's TX to the relay's RX, the hub's RX to the relay's TX, and share ground. Use 3.3 V logic and supply appropriate power to the relay.

Use `uart_id` in place of `espnow_id` on each hub light. Set the UART to 115200 baud, 8 data bits, no parity, and one stop bit. The optional `sensor` platform `esp_now_light` reads the relay's cumulative transmission error count on the same bus. See the [UART hub example](examples/1-hub-serial.yaml).

## Upgrade a legacy configuration

Replace the hub's `platform: custom` light with `platform: esp_now_light`. Convert each C++ MAC array to a colon-separated `mac_address` and move its light number, transport, and power supply into the YAML fields above. Replace the receiver's `custom_component` lambda with `esp_now_light.receivers`. When using the UART error sensor, replace its custom platform with `platform: esp_now_light`. Remove the old `esphome.includes` entries from each migrated configuration.

The light commands retain the `>SETLIGHT<number>` prefix and semicolon-delimited values. The UART relay also retains its decimal MAC prefix and newline framing. The original headers remain in [lib/](lib/) for legacy configurations.
