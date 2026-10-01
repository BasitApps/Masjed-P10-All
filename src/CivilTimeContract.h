#ifndef CIVIL_TIME_CONTRACT_H
#define CIVIL_TIME_CONTRACT_H

#include <stdint.h>

namespace CivilTime
{
inline int dayOfWeek(int year, int month, int day)
{
  static const int8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3)
    --year;
  return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

// Product rule shared by P10 and 7-segment: last Friday of April through
// last Thursday of October, inclusive, when seasonal adjustment is enabled.
inline bool isDstDate(int year, int month, int day)
{
  if (month > 4 && month < 10)
    return true;
  if (month == 4)
    return day >= 30 - ((dayOfWeek(year, 4, 30) - 5 + 7) % 7);
  if (month == 10)
    return day <= 31 - ((dayOfWeek(year, 10, 31) - 4 + 7) % 7);
  return false;
}
}

#endif
