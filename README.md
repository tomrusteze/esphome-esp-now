# ESP Now integration for ESPhome
First of all, this is based on a similar integration by [iphong](https://github.com/iphong/esphome-espnow), which is now updated to [MeshRC](https://github.com/iphong/lib-esp-rc). 
## Purpose
The purpose of this software is to enable communication between esp devices without a wifi connection. In the example the light acts as a slave and the hub controls the light. I have tried to make the communication as lightweight as possible and similar to service calls in HomeAssistant. The idea is explained in this diagram 
![Communication diagram](./img/Dataflow_example.svg)

## Features
- Using a node running esphome you can control a light on a different node also running esphome with the communcation using esp-now. Changing colour, effects and brightness are supported. As of Jan 29, multiple lights are supported. Differentiation between lights is done using the MAC address.
- Since Feb 20 support for the ESP32 microcontroller is added for the Hub.
- Nodes can run Binary, Monochromatic, RGB and RGBW Lights.
- The configured transition length in the hub will be transmitted to the nodes.
- Each node can run multiple lights.
- Nodes can either use their WiFi connection provided by esphome(Connecting to an Access Point or Setting up an Access point. See the [documentation](https://esphome.io/components/wifi.html)) or no Wi-Fi connection at all, which is in the ```light.yaml``` example. This must be initialized by ```MeshRC::setupwifi(${wifichannel});```.
- Connect the lights to a power supply. I have all my lights connected behind a relay, which turns the lights on when they must produce light. However, Initialising esp-now takes about a second, so using a power supply, we can easily send the command a little bit later to give esp-now time to initialise and save a lot of power.
- Light nodes transmit a ping each 10 seconds. This is received by the hub and shows a binary sensor with the status of light nodes. Very usefull for seeing why some lights do not turn on.

### Communication Protocol
All messages sent over esp-now are of the form: <br>
![Data frame](./img/Data_frame.svg)
- MAC address is the address of the light node, this is unique and basically functions as the channel on which messages are being sent. Nodes only listen to messages send to their own address.
- Direction can be either '>' or '<'. '>' means the messages is for the light node (from the hub) and '<' means the message is for the hub (from the light node).
- Examples of command can be PING, or LIGHT, these are implemented in the esphome files and can be easily changed or added.
- Arguments can supplement a command with information, such as parameters for the lights.

This implementation is used to generate so-called channels, these are based on the mac addresses and allow us to easily set up communication with different nodes and easily direct the traffic.
![Communication diagram](./img/Communication_example.svg)

## Improvements
- Alter the communication protocol, such that messages have to be confirmed to be received by the hub. (Now it is somewhat similar to UDP, we want to change it to TCP-ish)
- Add support for more types of lights, such as RGBCT.

## Goal
I wanted to have some nice lights in my garden, they should be controlleable by HomeAssistant so that they eventually would be able to react to movement.
### First attempt
Firstly, I decided to use [cheap commercial solar lights](https://offers.kd2.org/en/gb/lidl/pFpK/) and replace the hardware with some NeoPixel LEDs (WS2812 Ring). These LEDs would be controlled via an ESP chip from indoors. So all the data would travel underground to all the lights. All lights would be chained together so that every led in every light could be controlled individually. Unfortunately carrying the data over such a long distance underground proved almost impossible.
### Second attempt
As a second attempt, I decided to give each light it's own ESP-01s controller. They are very cheap (~1 USD) and they have wifi capability. I do however not want to use WiFi for all lights (4 at the moment) in my garden, since this would put some stress on my home network. Hence I decided to use the esp-now protocol to communicate between a hub and the lights.

## Usage
For this guide, I assume that you have esphome up and running and are familiar with how it all works. We need one microcontroller as a hub and one as a light.
- Clone this repository.
- Put the lib folder in you esphome folder.
- Take the two examples and put them in your esphome folder.
- Change the data in the examples, such that they are correct for your devices
  - First of all, you need to determine the WiFi channel of the Access Point. You can either download an app such as [WiFi Analyzer](https://play.google.com/store/apps/details?id=com.farproc.wifi.analyzer&hl=nl&gl=US) to determine the WiFi channel of the hub or look at the logs from any esphome device. These will show to what channel your node is connected. Then, make sure that your node always connects to this Access Point, see the esphome [documentation](https://esphome.io/components/wifi.html#connecting-to-multiple-networks).
  - Now configure the light node to also connect to this WiFi channel using the substitution:
```
substitutions:
  devicename: light
  wifichannel: "7"
```
  - Find the MAC address of the light node and write it in the hub in its substitutions:
```
substitutions:
  devicename: hub
  light_address_1: "{0xff,0xff,0xff,0xff,0xff,0xff};"
```
- Flash the two devices, try looking at the log first to see if it works.
