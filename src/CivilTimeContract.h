#ifndef CIVIL_TIME_CONTRACT_H
#define CIVIL_TIME_CONTRACT_H

#include <stdint.h>

namespace CivilTime
{
constexpr uint16_t MIN_YEAR = 2025;
constexpr uint16_t MAX_YEAR = 2099;

inline uint8_t daysInMonth(int year, int month)
{
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12)
    return 0;
  if (month == 2 && year % 4 == 0 &&
      (year % 100 != 0 || year % 400 == 0))
    return 29;
  return days[month - 1];
}

inline bool validDate(int year, int month, int day)
{
  return year >= MIN_YEAR && year <= MAX_YEAR && day >= 1 &&
         day <= daysInMonth(year, month);
}

inline uint8_t displayHour(uint8_t hour, bool use24)
{
  return use24 ? hour : (hour % 12 ? hour % 12 : 12);
}

// Call only after complete I2C reads; chip presence is separate from time validity.
template <typename Date>
inline bool decodeDs3231(uint8_t registers[7], uint8_t status, Date &value)
{
  if (status & 0x80) // Oscillator-stop flag: a plausible calendar may be stale.
    return false;
  registers[0] &= 0x7F;
  registers[1] &= 0x7F;
  const bool hour12 = registers[2] & 0x40;
  const bool pm = registers[2] & 0x20;
  registers[2] &= hour12 ? 0x1F : 0x3F;
  registers[4] &= 0x3F;
  const bool century = registers[5] & 0x80;
  registers[5] &= 0x1F;
  for (uint8_t index = 0; index < 7; ++index)
    if (index != 3 &&
        ((registers[index] & 0x0F) > 9 || (registers[index] >> 4) > 9))
      return false;
  const auto decimal = [](uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 15); };
  const uint8_t rawHour = decimal(registers[2]);
  const uint8_t month = decimal(registers[5]), day = decimal(registers[4]);
  const uint8_t minute = decimal(registers[1]), second = decimal(registers[0]);
  const uint16_t year = 2000 + (century ? 100 : 0) + decimal(registers[6]);
  if (!validDate(year, month, day) || minute > 59 || second > 59 ||
      (!hour12 && rawHour > 23) || (hour12 && (rawHour < 1 || rawHour > 12)))
    return false;
  value = Date(year, month, day,
               hour12 ? rawHour % 12 + (pm ? 12 : 0) : rawHour,
               minute, second);
  return value.isValid();
}

inline int dayOfWeek(int year, int month, int day)
{
  static const int8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3)
    --year;
  return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

inline int lastFridayInApril(int year)
{
  return 30 - ((dayOfWeek(year, 4, 30) - 5 + 7) % 7);
}

inline int lastThursdayInOctober(int year)
{
  return 31 - ((dayOfWeek(year, 10, 31) - 4 + 7) % 7);
}

// Product wall-clock rule shared by P10 and 7-segment. This is used when a
// local date is entered by the user; the end date is inclusive.
inline bool isDstDate(int year, int month, int day)
{
  if (month > 4 && month < 10)
    return true;
  if (month == 4)
    return day >= lastFridayInApril(year);
  if (month == 10)
    return day <= lastThursdayInOctober(year);
  return false;
}

// The RTC stores standard time. Egypt changes at standard 00:00 on the last
// Friday of April and at standard 23:00 on the last Thursday of October.
// Using the instant prevents the end-of-October date from advancing early.
inline bool isDstAtStandardTime(int year, int month, int day,
                                int hour, int minute)
{
  const int minutes = hour * 60 + minute;
  if (month > 4 && month < 10)
    return true;
  if (month < 4 || month > 10)
    return false;
  if (month == 4)
    return day > lastFridayInApril(year) ||
           (day == lastFridayInApril(year) && minutes >= 0);
  const int endDay = lastThursdayInOctober(year);
  return day < endDay || (day == endDay && minutes < 23 * 60);
}

// Shared Gregorian-to-Hijri conversion. Display firmware stores the result
// differently, but the calendar calculation must remain identical.
inline __attribute__((always_inline)) void gregorianToHijri(
    int year, int month, int day, int correction,
    int16_t &hYear, int8_t &hMonth, int8_t &hDay)
{
  const int64_t a = (14 - month) / 12;
  const int64_t y = (int64_t)year + 4800 - a;
  const int64_t m = (int64_t)month + 12 * a - 3;
  const int64_t jd = (int64_t)day + ((153 * m + 2) / 5) + 365 * y +
                     y / 4 - y / 100 + y / 400 - 32045 + correction;
  int64_t l = jd - 1948440 + 10632;
  const int64_t n = (l - 1) / 10631;
  l = l - 10631 * n + 354;
  const int64_t j = ((10985 - l) / 5316) * ((50 * l) / 17719) +
                    (l / 5670) * ((43 * l) / 15238);
  l = l - ((30 - j) / 15) * ((17719 * j) / 50) -
      (j / 16) * ((15238 * j) / 43) + 29;
  hMonth = (int8_t)((24 * l) / 709);
  hDay = (int8_t)(l - (709 * hMonth) / 24);
  hYear = (int16_t)(30 * n + j - 30);
}
}

#endif
