#include "esphome.h"

class UartReadIntSensor : public Component, public UARTDevice, public Sensor
{
public:
  UartReadIntSensor(UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override
  {
    // We (hopefully) start with 0 errors
    publish_state(0);
  }

  int readline(int readch, char *buffer, int len)
  {
    static int pos = 0;
    int rpos;

    if (readch > 0)
    {
      switch (readch)
      {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0; // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len - 1)
        {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
      }
    }
    // No end of line has been found, so return -1.
    return -1;
  }

  void loop() override
  {
    const int max_line_length = 80;
    static char buffer[max_line_length];
    while (available())
    {
      if (readline(read(), buffer, max_line_length) > 0)
      {
        publish_state(String(buffer).toInt());
      }
    }
  }
};