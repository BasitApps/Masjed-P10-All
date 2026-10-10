#ifndef PRAYER_PHASE_CONTRACT_H
#define PRAYER_PHASE_CONTRACT_H

#include <stdint.h>

enum class PrayerPhase : uint8_t
{
  Outside,
  PreAzan,
  Azan,
  EqamaCountdown,
  EqamaSeconds,
  Eqama,
  Sleep,
  Azkar
};

inline PrayerPhase prayerPhase(int16_t since, int16_t eqama, int16_t sleep)
{
  if (since >= -3 && since < 0)
    return PrayerPhase::PreAzan;
  if (since >= 0 && since <= 1)
    return PrayerPhase::Azan;
  if (eqama <= 0)
    return PrayerPhase::Outside;
  if (since >= 2 && since < eqama - 1)
    return PrayerPhase::EqamaCountdown;
  if (since == eqama - 1)
    return PrayerPhase::EqamaSeconds;
  if (since == eqama)
    return PrayerPhase::Eqama;
  if (since > eqama && since < eqama + sleep)
    return PrayerPhase::Sleep;
  if (since >= eqama + sleep && since < eqama + sleep + 1)
    return PrayerPhase::Azkar;
  return PrayerPhase::Outside;
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

// Night dimming and ultra mode share [start, end), including midnight.
// Zero offsets retain the existing disabled-night-mode convention.
inline bool nightWindowActive(int now, int isha, int fajr, int after, int before)
{
  if (now < 0 || now >= 1440 || isha < 0 || isha >= 1440 ||
      fajr < 0 || fajr >= 1440 || after <= 0 || before <= 0)
    return false;
  const int start = (isha + after) % 1440;
  const int end = ((fajr - before) % 1440 + 1440) % 1440;
  if (start == end) return false;
  return start < end ? now >= start && now < end : now >= start || now < end;
}

#endif
