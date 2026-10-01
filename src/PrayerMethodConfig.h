#ifndef PRAYER_METHOD_CONFIG_H
#define PRAYER_METHOD_CONFIG_H

#include <stdint.h>

struct PrayerMethodConfig
{
  float fajrAngle;
  bool maghribInMinutes;
  float maghribValue;
  bool ishaInMinutes;
  float ishaValue;
};

inline PrayerMethodConfig prayerMethodConfig(uint8_t method)
{
  // PrayTimes defaults: Maghrib is sunset + 0 min except where a method
  // explicitly defines another rule; Makkah Isha is 90 min after Maghrib.
  static const PrayerMethodConfig methods[] = {
      {19.5f, true, 0.0f, false, 17.5f}, // Egypt
      {18.5f, true, 0.0f, true, 90.0f},  // Makkah
      {18.0f, true, 0.0f, false, 17.5f}, // Custom baseline
      {18.0f, true, 0.0f, false, 17.0f}, // MWL
      {18.0f, true, 0.0f, false, 18.0f}, // Karachi
  };
  return method < (sizeof(methods) / sizeof(methods[0])) ? methods[method]
                                                          : methods[0];
}

#endif
