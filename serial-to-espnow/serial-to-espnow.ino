#include "MeshRC.h"

uint32_t last_error_count = 0;

void setup()
{
  Serial.begin(115200);
  MeshRC::setupwifi(7);
  MeshRC::begin();
}

void sendCommand(const String &line)
{
  unsigned parts[6];
  int offset = 0;
  if (sscanf(line.c_str(), "%u %u %u %u %u %u %n",
             &parts[0], &parts[1], &parts[2], &parts[3], &parts[4], &parts[5], &offset) != 6 ||
      !line.substring(offset).startsWith(">SETLIGHT"))
  {
    MeshRC::rejected++;
    return;
  }

  uint8_t address[6];
  for (size_t i = 0; i < sizeof(address); i++)
  {
    if (parts[i] > 255)
    {
      MeshRC::rejected++;
      return;
    }
    address[i] = parts[i];
  }

  MeshRC::send(address, line.substring(offset));
}

void loop()
{
  if (Serial.available())
    sendCommand(Serial.readStringUntil('\n'));

  const uint32_t errors = MeshRC::sent_error + MeshRC::rejected;
  if (errors != last_error_count)
  {
    last_error_count = errors;
    Serial.println(errors);
  }
}
