# Examples
This folder contains useable examples for 3 scenarios. At the very least you will need two esp devices flashed with esphome.
1. One hub with lights only accessible over esp-now. This is the original scenario that this project was created for. The lights have **NO** direct connection to Wi-Fi or HomeAssistant. It contains two lights but it can easily extend to include more (or only one). This example also includes a power supply that turns off the lights when they are not used. The lights will be presented to HomeAssistant as being part of the hub. 
2. A switch and a light. The switch only has a binary sensor, when pressed, it toggles the light. Both devices are connected to HomeAssistant, but the switch will **ONLY** use esp-now to control the light. The switch currently does not know when the state of the light has changed, so it may not work perfectly.

## Light types
Different types of lights are supported (RGB(W), monochromatic, binary). You need to select the same light in both nodes. Different types are shown in the following table:

| Type          | Sender                          | Receiver                    |
|---------------|---------------------------------|-----------------------------|
| Binary        | `esp_now_light()`               | `parseLight()`              |
| Monochromatic | `esp_now_light_Monochromatic()` | `parseLightMonochromatic()` |
| RGB           | `esp_now_light_RGB()`           | `parseLightRGB()`           |
| RGBW          | `esp_now_light_RGBW()`          | `parseLightRGBW_separate()` |

Keep in mind that addressable lights and non addressable lights have to be handled differently on the receiving side (light). 
```
MeshRC::on("#>SETLIGHT1", [](uint8_t* data, uint8_t size) {
      AddressableLightState* dest = id(light_1);
      parseLightRGB(data, size, dest);
    });
```
```
MeshRC::on("#>SETLIGHT1", [](uint8_t* data, uint8_t size) {
      LightState* dest = id(light_1);
      parseLightRGB(data, size, dest);
    });
```
