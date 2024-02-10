#include "MeshRC.h"

#define USE_ESP8266
#define NO_ESPHOME

uint32_t sent_error = 0;

void setup()
{
  Serial.begin(115200);

  MeshRC::setupwifi(7);
  MeshRC::begin();
}

void loop()
{

  if (Serial.available())
  {
    String s = Serial.readStringUntil('\n');
    sendCommand(s);
    Serial.println(sent_error);
  }

  if (sent_error != MeshRC::sent_error)
  {
    sent_error = MeshRC::sent_error;
    Serial.println(sent_error);
  }
}

uint8_t parseInt(String &data, char delimiter)
{
  int pos = data.indexOf(delimiter);
  uint8_t value = data.substring(0, pos).toInt();
  data.remove(0, pos + 1);
  return value;
}

void sendCommand(String &str)
{
  int pos = str.indexOf('>');
  String address_str = str.substring(0, pos);
  String command = str.substring(pos);

  uint8_t address[6];

  address[0] = parseInt(address_str, ' ');
  address[1] = parseInt(address_str, ' ');
  address[2] = parseInt(address_str, ' ');
  address[3] = parseInt(address_str, ' ');
  address[4] = parseInt(address_str, ' ');
  address[5] = parseInt(address_str, ' ');

  MeshRC::send(address, command);
}
