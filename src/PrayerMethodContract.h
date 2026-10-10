#ifndef PRAYER_METHOD_CONTRACT_H
#define PRAYER_METHOD_CONTRACT_H

#include <stdint.h>

struct PrayerMethodSpec {
  double fajrAngle;
  double maghribMinutes;
  bool ishaMinutes;
  double ishaValue;
};

inline bool validMakkahIshaMode(uint8_t mode) {
  return mode <= 1;
}

inline PrayerMethodSpec prayerMethodSpec(uint8_t method, uint8_t makkahIsha) {
  static const PrayerMethodSpec methods[] = {
      {19.5, 0.0, false, 17.5}, // Egypt
      {18.5, 0.0, false, 17.0}, // Makkah
      {18.0, 0.0, false, 17.5}, // Custom baseline
      {18.0, 0.0, false, 17.0}, // MWL
      {18.0, 0.0, false, 18.0}, // Karachi
  };
  PrayerMethodSpec result = method < sizeof(methods) / sizeof(methods[0])
                                ? methods[method]
                                : methods[0];
  if (method == 1 && makkahIsha == 1) {
    result.ishaMinutes = true;
    result.ishaValue = 90.0;
  }
  return result;
}

#endif
