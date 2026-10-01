#ifndef TIME_MATH_H
#define TIME_MATH_H

#include <stdint.h>
inline uint8_t displayHour(uint8_t hour, bool use24) {
  return use24 ? hour : (hour % 12 ? hour % 12 : 12);
}

inline int16_t signedMinuteDelta(int16_t nowMins, int16_t eventMins)
{
  int16_t delta = nowMins - eventMins;
  if (delta < -720)
    delta += 1440;
  else if (delta > 720)
    delta -= 1440;
  return delta;
}

inline int16_t forwardMinuteDistance(int16_t nowMins, int16_t targetMins)
{
  int16_t delta = targetMins - nowMins;
  while (delta < 0)
    delta += 1440;
  while (delta >= 1440)
    delta -= 1440;
  return delta;
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

inline uint8_t daysInMonth(int year, int month)
{
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};
  if (month == 2 && year % 4 == 0 &&
      (year % 100 != 0 || year % 400 == 0))
    return 29;
  return days[month - 1];
}

#endif
