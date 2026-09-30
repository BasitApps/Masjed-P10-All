#ifndef DISPLAY_PROFILE_H
#define DISPLAY_PROFILE_H

#include "constants.h"

// One compile-time geometry contract for every display layout.
struct DisplayProfile
{
  uint16_t width;
  uint8_t height;
  uint8_t smallTextY;
  uint8_t smallAreaY;
  uint8_t smallAreaH;
  uint16_t clockAreaWidth;
  uint16_t infoStart;
  uint8_t edgeMargin;
  uint8_t topLineHeight;
  bool hasTopInfo;

  constexpr bool narrow() const { return width <= 64; }
  constexpr bool bottomInfoCard(uint8_t cardIndex) const
  {
    return narrow() && cardIndex >= 2 && cardIndex <= 4;
  }
};

inline constexpr DisplayProfile DISPLAY_PROFILE = {
    uint16_t(DISPLAYS_WIDE * 32),
    uint8_t(DISPLAYS_HIGH * 16),
    uint8_t(DISPLAYS_HIGH * 16 >= 16 ? 8 : 0),
    uint8_t(DISPLAYS_HIGH * 16 >= 16 ? 8 : 0),
    uint8_t(DISPLAYS_HIGH * 16 >= 16 ? 8 : DISPLAYS_HIGH * 16),
    uint16_t(DISPLAYS_WIDE * 32 >= 96 ? (DISPLAYS_WIDE * 32) / 2
                                      : DISPLAYS_WIDE * 32),
    uint16_t(DISPLAYS_WIDE * 32 >= 96 ? (DISPLAYS_WIDE * 32) / 2
                                      : DISPLAYS_WIDE * 32),
    4,
    8,
    DISPLAYS_WIDE * 32 >= 96};

#endif
