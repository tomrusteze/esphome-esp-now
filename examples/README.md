# Examples

The examples load the checked-out component from `../components`. To use the examples outside this repository, replace their `external_components` blocks with the [GitHub source declaration](../README.md#install).

| Setup | Sender | Receiver |
| --- | --- | --- |
| Wi-Fi hub with lights off Wi-Fi | [1-hub.yaml](1-hub.yaml) | [1-light.yaml](1-light.yaml) |
| Hub with a dedicated UART radio | [1-hub-serial.yaml](1-hub-serial.yaml) | [1-light.yaml](1-light.yaml) |
| Physical switch and a light on Wi-Fi | [2-switch.yaml](2-switch.yaml) | [2-light.yaml](2-light.yaml) |

Replace the placeholder MAC addresses and Wi-Fi credentials. Set the light board, GPIO pins, and LED type for your hardware. If the receiver uses a fixed channel, match it to the hub's access point or the UART relay. For a single receiver, keep the first hub light. The second hub light targets a second device running the same receiver configuration with its own ESPHome name and MAC address.

The two hub examples include optional relay-controlled power for the lights. Adjust the power supply pin and its inversion for your relay, or remove the `power_supply` block and its references for independently powered lights. The UART setup also needs the [ESP8266 relay sketch](../README.md#uart-relay).

From the repository root, validate a configuration with `esphome config examples/1-hub.yaml` or build it with `esphome compile examples/1-hub.yaml`.
