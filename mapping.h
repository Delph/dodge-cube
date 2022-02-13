#ifndef MAPPING_H_INCLUDE
#define MAPPING_H_INCLUDE

#include <stdint.h>


uint8_t led_height(const size_t led)
{
  const uint8_t section = led / 7;

  switch (section)
  {
    case 0:
    case 4:
      return 0 + led % 7;
    case 3:
      return 6 - led % 7;
    case 1:
    case 5:
    case 11:
    case 13:
    case 21:
      return 7 + led % 7;
    case 2:
    case 10:
    case 12:
    case 20:
      return 13 - led % 7;
    case 6:
    case 14:
    case 16:
    case 18:
    case 22:
      return 14 + led % 7;
    case 9:
    case 15:
    case 17:
    case 19:
      return 20 - led % 7;
    case 7:
    case 23:
      return 21 + led % 7;
    case 8:
      return 27 - led % 7;
    default:
      return 255;
  }
}

uint16_t led_angle(const size_t led)
{
  const uint8_t section = led / 7;

  switch (section)
  {
    case 0:
    case 12:
      return 0;
    case 3:
    case 21:
      return 240;
    case 4:
    case 10:
      return 120;
    case 8:
    case 14:
      return 60;
    case 19:
    case 7:
      return 180;
    case 17:
    case 23:
      return 300;
    // all the rest need math
    default:
      return 360;
  }
}

#endif
