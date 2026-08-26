#ifndef GLOBALS_H
#define GLOBALS_H

#include "constants.h"
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

  bool isValid() const {
    return eepromVersion == EEPROM_VERSION && latitude >= -90.0f &&
           latitude <= 90.0f && longitude >= -180.0f && longitude <= 180.0f &&
           isSmallFont <= 1 && calcMethod <= 4 && wayMethod <= 1 &&
           scrollingSpeed <= 5 && (language == 0 || language == 1);
  }

  void setDefaults() {
    eepromVersion = EEPROM_VERSION;
    calcMethod = wayMethod = dst = hijriDayCorrection = summerTime = 0;
    eqamaFajr = eqamaShrouq = eqamaZohr = eqamaAsr = eqamaMagrb = eqamaEsha = 0;
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
  }
};

// ============ Helper Functions ============
inline void invalidateCaches(PrayerCache &p, int8_t &hijriDay) {
  p.valid = false;
  p.lastDay = -1;
  p.activeStart = p.activeEnd = -1;
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
inline PrayerCache prayerCache = {{0}, -1, -1, -1, false};
inline SystemFlags sysFlags = {0x02, 0};
inline DisplayState dispState = {{0}, {0}, 0, 50, 1, 0x01};
inline TimeData timeData = {0, 1400, 0, 0, 0, 0, 0, 0, 1, 1};
inline DisplayParams dispParams = {SMALL_FONT_Y_POS, 8, 8, true};
inline Configuration config;
inline EqamaCache eqamaCache = {{-1, -1, -1, -1, -1, -1, -1}, -1};

inline int8_t hijriLastDay = -1;
inline unsigned long lastSecondMs = 0;
inline int8_t cachedTemp = -99;
inline char fmtBuf[32];

// String caches (sized for UTF-8 Arabic names)
inline char cachedDayName[16] = {0};
inline int8_t cachedDayIndex = -1;
inline char cachedMonthName[26] = {0};
inline int8_t cachedMonthIndex = -1;

// Prayer indices constant
inline const int8_t PRAYER_INDICES[6] = {0, 1, 2, 3, 4, 6};

// Eqama cache refresh function
inline void refreshEqamaCache() {
  if (eqamaCache.times[0] == config.eqamaFajr &&
      eqamaCache.sleepTime == config.sleepPrayerTime)
    return;
  eqamaCache.times[0] = config.eqamaFajr;
  eqamaCache.times[1] = 0;
  eqamaCache.times[2] = config.eqamaZohr;
  eqamaCache.times[3] = config.eqamaAsr;
  eqamaCache.times[4] = config.eqamaMagrb;
  eqamaCache.times[5] = 0;
  eqamaCache.times[6] = config.eqamaEsha;
  eqamaCache.sleepTime = config.sleepPrayerTime;
}

#endif
