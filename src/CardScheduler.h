#ifndef CARD_SCHEDULER_H
#define CARD_SCHEDULER_H

#include <stdint.h>

namespace CardScheduler {

constexpr uint8_t COUNT = 12;
#ifndef CARD_COUNT
constexpr uint8_t CARD_COUNT = COUNT;
#endif

// In narrow small-font layouts there is no second top-line information area.
// Cards 2..4 (day, Gregorian date, Hijri date) are therefore rendered in the
// lower row; card 7 remains the fixed clock shown on the top row.
inline bool smallFontBottomInfo(uint8_t cardIndex, uint16_t displayWidth)
{
  return displayWidth <= 64 && cardIndex >= 2 && cardIndex <= 4;
}

inline bool validOrder(const uint8_t order[COUNT])
{
  uint16_t seen = 0;
  for (uint8_t i = 0; i < COUNT; ++i)
  {
    if (order[i] >= COUNT || (seen & (uint16_t(1) << order[i])))
      return false;
    seen |= uint16_t(1) << order[i];
  }
  return seen == ((uint16_t(1) << COUNT) - 1);
}

inline uint8_t firstIndex(const uint8_t order[COUNT], uint16_t enabledMask)
{
  for (uint8_t i = 0; i < COUNT; ++i)
    if (enabledMask & (uint16_t(1) << order[i]))
      return order[i];
  return 0;
}

inline uint8_t nextIndex(const uint8_t order[COUNT], uint16_t enabledMask,
                         uint8_t current)
{
  uint8_t position = 0;
  for (; position < COUNT; ++position)
    if (order[position] == current)
      break;
  for (uint8_t step = 1; step <= COUNT; ++step)
  {
    const uint8_t candidate = order[(position + step) % COUNT];
    if (enabledMask & (uint16_t(1) << candidate))
      return candidate;
  }
  return 0;
}

} // namespace CardScheduler

#endif
