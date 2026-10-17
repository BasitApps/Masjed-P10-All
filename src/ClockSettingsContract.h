#ifndef CLOCK_SETTINGS_CONTRACT_H
#define CLOCK_SETTINGS_CONTRACT_H

#include <math.h>

namespace ClockSettings
{
constexpr int MAX_EQAMA = 99;

inline int migrateEqamaValue(int value)
{
  return value > MAX_EQAMA && value <= 120 ? MAX_EQAMA : value;
}

// Accept old 100..120-minute records without resetting unrelated settings.
template <typename Config>
bool migrateEqama(Config &c)
{
  bool changed = false;
  const auto limit = [&](auto &value) {
    const int next = migrateEqamaValue(value);
    changed |= next != value;
    value = next;
  };
  limit(c.eqamaFajr); limit(c.eqamaShrouq); limit(c.eqamaZohr);
  limit(c.eqamaAsr); limit(c.eqamaMaghrib); limit(c.eqamaEsha);
  return changed;
}

template <typename Config, typename Minutes>
void setDefaults(Config &c, Minutes &afterIsha, Minutes &beforeFajr)
{
  c.prayerCalculationMethod = c.asrMethod = c.hijriDayCorrection = 0;
  c.daylightSavingTime = c.hour24 = 0;
  c.timezoneOffset = 2;
  c.eqamaFajr = 15;
  c.eqamaShrouq = c.eqamaZohr = c.eqamaAsr = c.eqamaEsha = 10;
  c.eqamaMaghrib = 8;
  c.sleepGomaaTime = c.sleepPrayerTime = 0;
  c.latitude = c.longitude = 10.05f;
  c.country = c.government = c.city = 0;
  afterIsha = beforeFajr = 30;
}

inline bool inRange(int value, int minimum, int maximum)
{
  return value >= minimum && value <= maximum;
}

// Storage and display fields stay local; both clocks share calculation limits.
template <typename Config>
bool valid(const Config &c, int afterIsha, int beforeFajr)
{
  return isfinite(c.latitude) && c.latitude >= -90 && c.latitude <= 90 &&
         isfinite(c.longitude) && c.longitude >= -180 && c.longitude <= 180 &&
         inRange(c.prayerCalculationMethod, 0, 4) && inRange(c.asrMethod, 0, 1) &&
         inRange(c.timezoneOffset, -12, 14) && inRange(c.hijriDayCorrection, -3, 3) &&
         inRange(c.daylightSavingTime, 0, 1) && inRange(c.hour24, 0, 1) &&
         inRange(c.eqamaFajr, 0, MAX_EQAMA) && inRange(c.eqamaShrouq, 0, MAX_EQAMA) &&
         inRange(c.eqamaZohr, 0, MAX_EQAMA) && inRange(c.eqamaAsr, 0, MAX_EQAMA) &&
         inRange(c.eqamaMaghrib, 0, MAX_EQAMA) && inRange(c.eqamaEsha, 0, MAX_EQAMA) &&
         inRange(c.sleepPrayerTime, 0, 120) && inRange(c.sleepGomaaTime, 0, 120) &&
         inRange(afterIsha, 0, 720) && inRange(beforeFajr, 0, 720);
}
}
#endif
