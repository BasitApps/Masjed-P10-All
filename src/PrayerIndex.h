#ifndef PRAYER_INDEX_H
#define PRAYER_INDEX_H

#include <stdint.h>

namespace PrayerIndex
{
enum : uint8_t
{
  Fajr = 0,
  Sunrise = 1,
  Dhuhr = 2,
  Asr = 3,
  Sunset = 4,
  Maghrib = 5,
  Isha = 6
};

static constexpr uint8_t EventIndices[6] = {
    Fajr, Sunrise, Dhuhr, Asr, Maghrib, Isha};
}

#endif
