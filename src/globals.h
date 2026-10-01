#ifndef GLOBALS_H
#define GLOBALS_H

#include "constants.h"
#include "PrayerIndex.h"
#include "DisplayProfile.h"
#include <Arduino.h>

// ============ PROGMEM Data ============
static const char ENCRYPT_KEY[] PROGMEM = "Abonagy@2";
static const uint8_t HOUR12_TABLE[24] PROGMEM = {12, 1, 2,  3,  4,  5, 6,  7,
                                                 8,  9, 10, 11, 12, 1, 2,  3,
                                                 4,  5, 6,  7,  8,  9, 10, 11};
#define hour12(h) pgm_read_byte(&HOUR12_TABLE[(h) % 24])

// ============ Prayer Cache ============
struct PrayerCache {
  int16_t times[7];
  int16_t activeStart; // Start minute for active prayer window
  int16_t activeEnd;   // End minute for active prayer window
  int16_t lastYear;
  int8_t lastMonth;
  int8_t lastDay;
  bool valid;
};

// ============ System Flags (bit-packed) ============
struct SystemFlags {
  uint8_t bits;
  unsigned long restartAt;

  enum : uint8_t {
    F_UPDATE = 0x01,
    F_SPLASH = 0x02,
    F_RESTART = 0x04,
    F_TEST = 0x08,
    F_RTC_OK = 0x10
  };

  bool updateNow() const { return bits & F_UPDATE; }
  bool splashScreen() const { return bits & F_SPLASH; }
  bool restartNow() const { return bits & F_RESTART; }
  bool testMode() const { return bits & F_TEST; }
  bool rtcOk() const { return bits & F_RTC_OK; }

  void setUpdate(bool v) { v ? (bits |= F_UPDATE) : (bits &= ~F_UPDATE); }
  void setSplash(bool v) { v ? (bits |= F_SPLASH) : (bits &= ~F_SPLASH); }
  void setRestart(bool v) { v ? (bits |= F_RESTART) : (bits &= ~F_RESTART); }
  void setTest(bool v) { v ? (bits |= F_TEST) : (bits &= ~F_TEST); }
  void setRtc(bool v) { v ? (bits |= F_RTC_OK) : (bits &= ~F_RTC_OK); }
};

// ============ Display State ============
struct DisplayState {
  char runText[BUF_LARGE];
  char tempBuf[BUF_SMALL];
  unsigned long cardTimer;
  uint16_t scrollSpeed;
  uint8_t cardNum;
  uint8_t flags;

  enum : uint8_t { F_FIRST_SHOW = 0x01, F_STOP_CARDS = 0x02 };

  bool firstShow() const { return flags & F_FIRST_SHOW; }
  bool stopCards() const { return flags & F_STOP_CARDS; }

  void setFirstShow(bool v) {
    v ? (flags |= F_FIRST_SHOW) : (flags &= ~F_FIRST_SHOW);
  }
  void setStopCards(bool v) {
    v ? (flags |= F_STOP_CARDS) : (flags &= ~F_STOP_CARDS);
  }
};

// ============ Time Data ============
struct TimeData {
  int16_t year, hYear;
  int8_t month, day, hour, min, sec, dow;
  int8_t hMonth, hDay;
  int8_t temperature;
};

// ============ Cached Display Params ============
struct DisplayParams {
  int8_t yPos;
  int8_t areaY;
  int8_t areaH;
  bool isSmall;
};

// ============ Configuration (EEPROM) ============
struct Configuration {
  uint16_t eepromVersion;
  int8_t calcMethod, wayMethod, dst, hijriDayCorrection, summerTime;
  int8_t eqamaFajr, eqamaShrouq, eqamaZohr, eqamaAsr, eqamaMagrb, eqamaEsha;
  int8_t sleepGomaaTime, sleepPrayerTime;
  float latitude, longitude;
  int16_t country, government;
  int16_t password, wifiPassword;
  uint8_t brightness, scrollingSpeed, isSmallFont;
  int8_t language;
  int16_t powerSaveAfterIsha, powerSaveBeforeFajr;
  int16_t city;
  char masjedName[100];
  char message[240];
  char apPassword[64];
  uint8_t mustChangePassword;
    uint8_t fontFamily;
    uint8_t digitStyle; // 0: 0123456789, 1: Indic; independent of language/font
    uint8_t hour24; // Display only; RTC/prayer arithmetic always uses 24h.

  bool isValid() const {
    const bool eqamaValid =
        eqamaFajr >= 0 && eqamaFajr <= 120 && eqamaShrouq >= 0 &&
        eqamaShrouq <= 120 && eqamaZohr >= 0 && eqamaZohr <= 120 &&
        eqamaAsr >= 0 && eqamaAsr <= 120 && eqamaMagrb >= 0 &&
        eqamaMagrb <= 120 && eqamaEsha >= 0 && eqamaEsha <= 120;
    return eepromVersion == EEPROM_VERSION && isfinite(latitude) &&
           latitude >= -90.0f && latitude <= 90.0f && isfinite(longitude) &&
           longitude >= -180.0f && longitude <= 180.0f && calcMethod >= 0 &&
           calcMethod <= 4 && wayMethod >= 0 && wayMethod <= 1 && dst >= -12 &&
           dst <= 14 && hijriDayCorrection >= -3 && hijriDayCorrection <= 3 &&
           summerTime >= 0 && summerTime <= 1 && eqamaValid &&
           sleepGomaaTime >= 0 && sleepGomaaTime <= 120 &&
           sleepPrayerTime >= 0 && sleepPrayerTime <= 120 && isSmallFont <= 1 &&
           scrollingSpeed <= 5 && (language == LANG_AR || language == LANG_EN) &&
           powerSaveAfterIsha >= 0 && powerSaveAfterIsha <= 720 &&
           powerSaveBeforeFajr >= 0 && powerSaveBeforeFajr <= 720 &&
           memchr(apPassword, '\0', sizeof(apPassword)) != nullptr &&
           strlen(apPassword) >= 8 && strlen(apPassword) <= 63 &&
             mustChangePassword <= 1 && fontFamily <= 2 && digitStyle <= 1 && hour24 <= 1;
  }

  void setDefaults() {
    eepromVersion = EEPROM_VERSION;
    calcMethod = wayMethod = dst = hijriDayCorrection = summerTime = 0;
    eqamaFajr = eqamaZohr = eqamaAsr = eqamaMagrb = eqamaEsha = 0;
    eqamaShrouq = 10;
    sleepGomaaTime = sleepPrayerTime = 0;
    latitude = longitude = 10.05f;
    country = government = password = wifiPassword = 0;
    brightness = scrollingSpeed = 1;
    isSmallFont = 0;
    language = LANG_AR;
    powerSaveAfterIsha = powerSaveBeforeFajr = 30;
    city = 0;
    masjedName[0] = message[0] = '0';
    masjedName[1] = message[1] = '\0';
    memcpy(apPassword, AP_PASSWORD, sizeof(AP_PASSWORD));
    // Password changes are optional; a forgotten password is recovered by
    // the physical factory-reset button.
    mustChangePassword = 0;
      fontFamily = 0;
      digitStyle = 0;
      hour24 = 0;
  }
};

// ============ Helper Functions ============
inline void invalidateCaches(PrayerCache &p, int16_t &hijriYear,
                             int8_t &hijriMonth, int8_t &hijriDay) {
  p.valid = false;
  p.lastYear = -1;
  p.lastMonth = -1;
  p.lastDay = -1;
  p.activeStart = p.activeEnd = INT16_MIN;
  hijriYear = -1;
  hijriMonth = -1;
  hijriDay = -1;
}

// Fast time formatting (avoids snprintf overhead)
inline void minToHM(int16_t mins, uint8_t &h, uint8_t &m) {
  h = mins / 60;
  m = mins - h * 60;
}

// Fast integer to 2-digit string (with leading zero)
inline void int2str2(char *buf, uint8_t val) {
  buf[0] = '0' + val / 10;
  buf[1] = '0' + val % 10;
}

// Fast time string formatter "H:MM" or "HH:MM"

// ============ Eqama Cache ============
struct EqamaCache {
  int8_t times[7];
  int8_t sleepTime;
};

// ============ Global Variable Definitions ============
// Note: These use 'inline' (C++17) to avoid multiple definition errors
inline PrayerCache prayerCache = {{0}, INT16_MIN, INT16_MIN, -1, -1, -1, false};
inline SystemFlags sysFlags = {0x02, 0};
inline DisplayState dispState = {{0}, {0}, 0, 50, 1, 0x01};
inline TimeData timeData = {0, 1400, 0, 0, 0, 0, 0, 0, 1, 1};
inline DisplayParams dispParams = {DISPLAY_PROFILE.smallTextY,
                                   DISPLAY_PROFILE.smallAreaY,
                                   DISPLAY_PROFILE.smallAreaH, true};
inline Configuration config;
// Stored separately from Configuration to preserve the v6 EEPROM layout.
inline uint8_t makkahIshaMode = 0; // 0: calculation method, 1: fixed 90 min
inline EqamaCache eqamaCache = {{-1, -1, -1, -1, -1, -1, -1}, -1};

inline int16_t hijriLastYear = -1;
inline int8_t hijriLastMonth = -1;
inline int8_t hijriLastDay = -1;
inline uint32_t lastSecondMs = 0;
inline unsigned long lastRtcSyncMs = 0;
inline char resetReason[40] = "unknown";
inline char resetInfo[96] = "unknown";
inline int8_t cachedTemp = -99;
inline char fmtBuf[BUF_SMALL];

// String caches (sized for UTF-8 Arabic names)
inline char cachedDayName[16] = {0};
inline int8_t cachedDayIndex = -1;
inline char cachedMonthName[26] = {0};
inline int8_t cachedMonthIndex = -1;

// Prayer indices constant
inline const int8_t PRAYER_INDICES[6] = {
    PrayerIndex::Fajr, PrayerIndex::Sunrise, PrayerIndex::Dhuhr,
    PrayerIndex::Asr, PrayerIndex::Maghrib, PrayerIndex::Isha};

// Eqama cache refresh function
inline void refreshEqamaCache() {
  eqamaCache.times[0] = config.eqamaFajr;
  eqamaCache.times[1] = config.eqamaShrouq;
  eqamaCache.times[2] = config.eqamaZohr;
  eqamaCache.times[3] = config.eqamaAsr;
  eqamaCache.times[4] = 0;
  eqamaCache.times[5] = config.eqamaMagrb;
  eqamaCache.times[6] = config.eqamaEsha;
  eqamaCache.sleepTime = config.sleepPrayerTime;
}

#endif
