#ifndef TIME_MATH_H
#define TIME_MATH_H

#include <stdint.h>

inline bool sameCalendarDate(int16_t year, int8_t month, int8_t day,
                             int16_t cachedYear, int8_t cachedMonth,
                             int8_t cachedDay)
{
  return year == cachedYear && month == cachedMonth && day == cachedDay;
}

inline uint8_t consumeSecondTicks(uint32_t nowMs, uint32_t &lastMs,
                                   uint8_t maxTicks = 60)
{
  const uint32_t due = (nowMs - lastMs) / 1000UL;
  if (due == 0)
    return 0;
  const uint32_t ticks = due > maxTicks ? maxTicks : due;
  lastMs += ticks * 1000UL;
  return static_cast<uint8_t>(ticks);
}

inline bool minuteInWindow(int16_t nowMins, int16_t startMins, int16_t endMins)
{
  const int16_t span = endMins - startMins;
  int16_t elapsed = nowMins - startMins;
  while (elapsed < 0)
    elapsed += 1440;
  while (elapsed >= 1440)
    elapsed -= 1440;
  return span >= 0 && span < 1440 && elapsed <= span;
}
#endif
