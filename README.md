# ESP Now integration for ESPhome
First of all, this is based on a similar integration by [iphong](https://github.com/iphong/esphome-espnow). 
The purpose of this software is to enable communication between esp devices without a wifi connection. In the example the light acts as a slave and the hub controls the light. I have tried to make the communication as lightweight as possible and similar to service calls in HomeAssistant. 

As of now, the nodes still need wifi and they should be connected to an access point on the same channel.
