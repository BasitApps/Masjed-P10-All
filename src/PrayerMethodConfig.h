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

inline bool validMakkahIshaMode(uint8_t mode)
{
  return mode <= 1;
}

inline PrayerMethodConfig prayerMethodConfig(uint8_t method)
{
  // Keep the prior firmware behavior as the method-derived Makkah baseline;
  // the optional fixed-90 rule is applied by prayerMethodConfigForMode().
  static const PrayerMethodConfig methods[] = {
      {19.5f, true, 0.0f, false, 17.5f}, // Egypt
      {18.5f, true, 0.0f, false, 17.0f}, // Makkah (method-derived)
      {18.0f, true, 0.0f, false, 17.5f}, // Custom baseline
      {18.0f, true, 0.0f, false, 17.0f}, // MWL
      {18.0f, true, 0.0f, false, 18.0f}, // Karachi
  };
  return method < (sizeof(methods) / sizeof(methods[0])) ? methods[method]
                                                          : methods[0];
}

inline PrayerMethodConfig prayerMethodConfigForMode(uint8_t method,
                                                     uint8_t makkahIshaMode)
{
  PrayerMethodConfig config = prayerMethodConfig(method);
  if (method == 1 && makkahIshaMode == 1)
  {
    config.ishaInMinutes = true;
    config.ishaValue = 90.0f;
  }
  return config;
}

#endif
