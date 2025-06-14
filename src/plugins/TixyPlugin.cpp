#include "plugins/TixyPlugin.h"

#define X_COL i % 16
#define Y_COL i / 16

void TixyPlugin::setup()
{
  // loading screen
  Screen.setPixel(4, 7, 1);
  Screen.setPixel(5, 7, 1);
  Screen.setPixel(7, 7, 1);
  Screen.setPixel(8, 7, 1);
  Screen.setPixel(10, 7, 1);
  Screen.setPixel(11, 7, 1);
}

void TixyPlugin::loop()
{
  double t = millis() / 1000.0;
#pragma unroll 1
  for (uint8_t i = 0; i < 255; i++)
  {
    float val = TixyPlugin::code(t, i, X_COL, Y_COL);
    Screen.setPixel(X_COL, Y_COL, val > 0.01, max((uint8_t)0, (uint8_t)min((uint16_t)255, (uint16_t)(val * 255))));
  }
}

// This code adapted from
// https://github.com/owenmcateer/tixy.land-display
// Credit owenmcateer
#pragma no_inline
float TixyPlugin::code(double t, double i, double x, double y)
{
  y = y * 1.4;
  switch (millis() / (15000L) % 24)
  {
  /**
   * Motus Art
   * @motus_art
   */
  case 0:
    // Plasma
    return sin(x + t) + sin(y + t) + sin(x + y + t) / 3;
    break;

  case 1:
    // Up&Down
    return sin(cos(x) * y / 8 + t);
    break;

  case 3:
    return sin(i / 5 + (t));
    break;

  case 4:
    return (abs(sin(t)) * 32 > x + y) * 0.5;
    break;

  /**
   * tixy.land website
   */
  case 5:
    // Emitting rings
    return sin(t - sqrt(pow((x - 7.5), 2) + pow((y - 6), 2)));
    break;

  case 6:
    // Rotation
    return sin(PI * 2 * atan((y - 8) / (x - 8)) + 5 * t);
    break;

  case 7:
    // Vertical fade
    return sin(y / 8 + t);
    break;

  case 8:
    // Smooth noise
    return cos(t + i + x * y);
    break;

  case 9: /// MEEH
    // Waves
    return sin(x / 2) - sin(x - t) - y + 6;
    break;

  case 10:
    // Ripples @thespite
    return sin(t - sqrt(x * x + y * y));
    break;

    /**
     * Reddit
     */

  case 11:
    // sierpinski
    return ((int)(4 * t) & (int)i & (int)x & (int)y);
    break;

  case 12:
    // lurkerurke https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgcwsn/
    return sin((x - 7.5) * (y - 7.5) / 5 * t + t);
    break;

  case 13:
    // SN0WFAKER https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgk7c0/
    return sin(atan((y - 7.5) / (x - 7.5)) + t * 6);
    break;

  case 14: // Diagonal hell
    // LeadingNegotiation9 https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbjcoho/
    return pow(cos(((int)y ^ (int)x) + t), cos((x > y) + t));
    break;

  case 15: // Smooth worm
    // detunized https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgk30l/
    return sin(y / 8 + t) + x / 16 - 0.5;
    break;

  case 16: // Spotlight
    // Andres_A https://www.reddit.com/r/programming/comments/jpqbux/minimal_16x16_dots_coding_environment/gbgzdnj/
    return 1 - hypot(sin(t) * 9 - x, cos(t) * 9 - y) / 9;
    break;

  /**
   * @akella
   * https://twitter.com/akella/status/1323549082552619008
   */
  case 17: // Windmill on left
    return sin(6 * atan2(y - 8, x) + t);
    break;

  /**
   * Paul Malin
   * https://twitter.com/P_Malin/
   */
  case 18:
    // Parallax Scrolling Checkerboard https://twitter.com/P_Malin/status/1323609539648905218
    return ((int)((x - 8) / y + t * 5) & 1 ^ (int)(1 / y * 8) & 1) * y / 5;
    break;

  case 19:
    // Matrix Rain https://twitter.com/P_Malin/status/1323583013880553472
    return 1 - fmod((x * x - y + t * (fmod(1 + x * x, 5.0)) * 3.0), 16.0) / 16.0;
    break;

  case 20:
    // Burst https://twitter.com/P_Malin/status/1323605999274594304
    return -4 / ((x - 8) * (x - 8) + (y - 8) * (y - 8) - fmod(t / 2, 1) * 200);
    break;

  case 21:
    // Rays
    return sin(atan2(x, y) * 5 + t * 5);
    break;

  case 22:
    // Starfield https://twitter.com/P_Malin/status/1323702220320313346
    return !((int)(x + t * 50 / (fmod(y * y, 5.9) + 1)) & 15) / (fmod(y * y, 5.9) + 1);
    break;

  case 23:
    // Henry https://github.com/henmel
    float a,
        b, c;
    return a = 3 - fabs(fmod(t, 6) - 3), b = fmod(t, 2) - 1, c = 3 - hypot(x - 4 * a - 1.5, y - 10 * b * b - 3), c < 0 ? 0.2 : c * (((int)x ^ (int)y) & 1 ? -1 : 1);
    break;

  default:
    return 0.0;
  }
}

const char *TixyPlugin::getName() const
{
  return "TixyLand";
}
