#ifndef CLOCK_SETTINGS_CONTRACT_H
#define CLOCK_SETTINGS_CONTRACT_H

#include <math.h>

namespace ClockSettings
{
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
         inRange(c.eqamaFajr, 0, 120) && inRange(c.eqamaShrouq, 0, 120) &&
         inRange(c.eqamaZohr, 0, 120) && inRange(c.eqamaAsr, 0, 120) &&
         inRange(c.eqamaMaghrib, 0, 120) && inRange(c.eqamaEsha, 0, 120) &&
         inRange(c.sleepPrayerTime, 0, 120) && inRange(c.sleepGomaaTime, 0, 120) &&
         inRange(afterIsha, 0, 720) && inRange(beforeFajr, 0, 720);
}
}
#endif
