#include "SmartText.h"
#include "ArabicShaper.h"
#include "TimeMath.h"
#include "CardScheduler.h"
#include "constants.h"
#include "globals.h"
#include "PrayerMethodConfig.h"
#include "CivilTimeContract.h"
#include "texts.h"
#include "OtaPublicKey.h"
#include <Arduino.h>
#include <DMDESP.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PrayerTimes.h>
#include <RTClib.h>
#include <Wire.h>
#include <Updater.h>
#include <BearSSLHelpers.h>
#include <bearssl/bearssl.h>
#include <fonts/BasitFont16.h>
#include <fonts/BasitFont7.h>
#include <fonts/Tahoma16.h>
#include <fonts/Tahoma7.h>
#include <fonts/BasitFont7Indic.h>
#include <fonts/BasitFont16Indic.h>
#include <fonts/Tahoma7Indic.h>
#include <fonts/Tahoma16Indic.h>

// PrayerTimes.cpp defines these setters but its legacy header omits them.
void set_maghrib_minutes(double minutes);
void set_isha_minutes(double minutes);

ESP8266WebServer server(80);
RTC_DS3231 rtc;
DMDESP dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);
SmartText smartText(dmd, DISPLAY_PROFILE.width);
static bool v2Request = false;
static char v2Body[API_V2_BODY_SIZE];
static bool displayFrameDirty = true;
static bool displayReady = false;

static uint16_t cardEnabledMask = (uint16_t(1) << CARD_COUNT) - 1;
static uint8_t cardOrder[CARD_COUNT] = {0, 1, 2, 3, 4, 5,
                                        6, 7, 8, 9, 10, 11};
static uint8_t priorityResumeCard = 0;
static bool priorityDisplayActive = false;

inline void markDisplayDirty()
{
  displayFrameDirty = true;
}

inline void presentDisplayFrame()
{
  if (!dmd.doubleBuffer())
  {
    displayFrameDirty = false;
    return;
  }
  if (displayFrameDirty)
  {
    dmd.swapBuffersAndCopy();
    displayFrameDirty = false;
  }
}

void updateRtcBootIndicator(unsigned long now)
{
  static unsigned long lastToggle = 0;
  static bool visible = false;
  static bool touched = false;
  const int x = DISPLAY_PROFILE.width - 1;
  const int y = DISPLAY_PROFILE.height - 1;

  if (!sysFlags.splashScreen())
    return;
  if (sysFlags.rtcOk())
  {
    if (touched)
    {
      dmd.setPixel(x, y, Bitmap::Black);
      markDisplayDirty();
      touched = false;
    }
    return;
  }
  if (now - lastToggle < 500UL)
    return;
  lastToggle = now;
  visible = !visible;
  dmd.setPixel(x, y, visible ? Bitmap::White : Bitmap::Black);
  markDisplayDirty();
  touched = true;
}

inline void safeCopy(char *dst, const char *src, size_t len)
{
  strncpy(dst, src, len - 1);
  dst[len - 1] = '\0';
}
inline void safeCopyP(char *dst, PGM_P src, size_t len)
{
  strncpy_P(dst, src, len - 1);
  dst[len - 1] = '\0';
}

inline void sleeping()
{
  dmd.clear();
  dmd.setPixel(0, 0, Bitmap::White);
  markDisplayDirty();
}

inline PGM_P prayerName(uint8_t i)
{
  return config.language == LANG_EN ? PRAYER_NAME_EN[i] : PRAYER_NAME_AR[i];
}
inline PGM_P dayName()
{
  return config.language == LANG_EN ? DAY_NAME_EN[timeData.dow]
                                    : DAY_NAME_AR[timeData.dow];
}
inline PGM_P hijriMonthName(int8_t m)
{
  return config.language == LANG_EN ? HIJRI_MONTH_EN[m - 1]
                                    : HIJRI_MONTH_AR[m - 1];
}

void blur(const char *text, char *out, size_t outSize)
{
  size_t len = 0;
  for (size_t i = 0; text[i] && len + 4 < outSize; ++i)
  {
    if (i > 0)
      out[len++] = '-';
    uint8_t val = (uint8_t)text[i] + 37;
    if (val >= 100)
    {
      out[len++] = '0' + (val / 100);
      val %= 100;
      out[len++] = '0' + (val / 10);
    }
    else if (val >= 10)
      out[len++] = '0' + (val / 10);
    out[len++] = '0' + (val % 10);
  }
  out[len] = '\0';
}
bool unBlur(const char *text, char *out, size_t outSize)
{
  if (!text || !text[0] || outSize == 0)
    return false;
  size_t len = 0;
  const char *p = text;
  while (*p)
  {
    if (len + 1 >= outSize || *p < '0' || *p > '9')
      return false;
    int val = 0;
    uint8_t digits = 0;
    while (*p >= '0' && *p <= '9')
    {
      if (++digits > 3)
        return false;
      val = val * 10 + (*p++ - '0');
    }
    if (val > 255)
      return false;
    out[len++] = (char)((val - 37 + 256) & 0xFF);
    if (*p == '-')
    {
      ++p;
      if (!*p)
        return false;
    }
    else if (*p)
      return false;
  }
  out[len] = '\0';
  return true;
}
void xorConfig(Configuration &c)
{
  uint8_t *data = reinterpret_cast<uint8_t *>(&c);
  size_t offset = sizeof(c.eepromVersion);
  char key[16];
  safeCopyP(key, ENCRYPT_KEY, sizeof(key));
  size_t keyLen = strlen(key);
  for (size_t i = 0; i < sizeof(Configuration) - offset; ++i)
    data[offset + i] ^= key[i % keyLen];
}

struct ConfigMeta
{
  uint32_t magic;
  uint32_t crc;
};

struct CardPrefsStorage
{
  uint32_t magic;
  uint16_t enabledMask;
  uint8_t order[CARD_COUNT];
  uint32_t crc;
};

struct PrayerPrefsStorage
{
  uint8_t magic;
  uint8_t makkahIshaMode;
  uint16_t crc;
};

static const uint32_t CONFIG_META_MAGIC = 0x42534333UL; // "BSC3"
static const uint32_t CARD_PREFS_MAGIC = 0x42534350UL; // "BSCP"
static const uint8_t PRAYER_PREFS_MAGIC = 0xA7;
static constexpr size_t PRAYER_PREFS_OFFSET = sizeof(Configuration) + sizeof(ConfigMeta);
static constexpr size_t CARD_PREFS_OFFSET = 464;
static_assert(sizeof(Configuration) + sizeof(ConfigMeta) <= 512,
              "Configuration exceeds EEPROM allocation");
static_assert(CARD_PREFS_OFFSET + sizeof(CardPrefsStorage) <= 512,
              "Card preferences exceed EEPROM allocation");

static_assert(offsetof(Configuration, digitStyle) == 448,
              "v4 migration requires the original 448-byte prefix");
static_assert(sizeof(Configuration) == 452,
              "Card preferences offset assumes the existing 452-byte config");
static_assert(sizeof(PrayerPrefsStorage) == 4 &&
                  PRAYER_PREFS_OFFSET + sizeof(PrayerPrefsStorage) == CARD_PREFS_OFFSET,
              "Prayer preference sidecar must fit the EEPROM gap");
uint32_t bytesCrc32(const uint8_t *bytes, size_t size)
{
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < size; ++i)
  {
    crc ^= bytes[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320UL & -(crc & 1));
  }
  return ~crc;
}
uint32_t configCrc32(const Configuration &source, size_t size = sizeof(Configuration))
{
  return bytesCrc32(reinterpret_cast<const uint8_t *>(&source), size);
}

uint16_t prayerPrefsCrc(uint8_t mode)
{
  uint16_t crc = 0xFFFF;
  crc ^= mode;
  for (uint8_t bit = 0; bit < 8; ++bit)
    crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  return crc;
}

bool savePrayerPrefs(uint8_t mode)
{
  if (!validMakkahIshaMode(mode))
    return false;
  PrayerPrefsStorage storage = {PRAYER_PREFS_MAGIC, mode, prayerPrefsCrc(mode)};
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&storage);
  for (size_t i = 0; i < sizeof(storage); ++i)
    if (EEPROM.read(PRAYER_PREFS_OFFSET + i) != bytes[i])
    {
      EEPROM.put(PRAYER_PREFS_OFFSET, storage);
      return EEPROM.commit();
    }
  return true;
}

bool loadPrayerPrefs()
{
  PrayerPrefsStorage storage;
  EEPROM.get(PRAYER_PREFS_OFFSET, storage);
  if (storage.magic != PRAYER_PREFS_MAGIC ||
      !validMakkahIshaMode(storage.makkahIshaMode) ||
      storage.crc != prayerPrefsCrc(storage.makkahIshaMode))
  {
    makkahIshaMode = 0;
    return false;
  }
  makkahIshaMode = storage.makkahIshaMode;
  return true;
}

uint32_t cardPrefsCrc(uint16_t mask, const uint8_t order[CARD_COUNT])
{
  uint8_t data[2 + CARD_COUNT];
  data[0] = uint8_t(mask & 0xFF);
  data[1] = uint8_t(mask >> 8);
  memcpy(data + 2, order, CARD_COUNT);
  return bytesCrc32(data, sizeof(data));
}

void resetCardPrefsRuntime()
{
  cardEnabledMask = (uint16_t(1) << CARD_COUNT) - 1;
  for (uint8_t i = 0; i < CARD_COUNT; ++i)
    cardOrder[i] = i;
}

bool saveCardPrefs()
{
  CardPrefsStorage storage;
  memset(&storage, 0, sizeof(storage));
  storage.magic = CARD_PREFS_MAGIC;
  storage.enabledMask = cardEnabledMask;
  memcpy(storage.order, cardOrder, sizeof(cardOrder));
  storage.crc = cardPrefsCrc(storage.enabledMask, storage.order);
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&storage);
  bool changed = false;
  for (size_t i = 0; i < sizeof(storage); ++i)
  {
    if (EEPROM.read(CARD_PREFS_OFFSET + i) != bytes[i])
    {
      changed = true;
      break;
    }
  }
  if (!changed)
    return true;
  EEPROM.put(CARD_PREFS_OFFSET, storage);
  return EEPROM.commit();
}

bool loadCardPrefs()
{
  CardPrefsStorage storage;
  EEPROM.get(CARD_PREFS_OFFSET, storage);
  const uint16_t validMask = (uint16_t(1) << CARD_COUNT) - 1;
  if (storage.magic != CARD_PREFS_MAGIC || storage.enabledMask == 0 ||
      (storage.enabledMask & ~validMask) ||
      !CardScheduler::validOrder(storage.order) ||
      storage.crc != cardPrefsCrc(storage.enabledMask, storage.order))
  {
    resetCardPrefsRuntime();
    return false;
  }
  cardEnabledMask = storage.enabledMask;
  memcpy(cardOrder, storage.order, sizeof(cardOrder));
  return true;
}

bool saveConfig(const Configuration &source)
{
  Configuration tmp;
  memcpy(&tmp, &source, sizeof(Configuration));
  const ConfigMeta meta = {CONFIG_META_MAGIC, configCrc32(source)};
  xorConfig(tmp);
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&tmp);
  bool changed = false;
  for (size_t i = 0; i < sizeof(Configuration); ++i)
  {
    if (EEPROM.read(i) != bytes[i])
    {
      changed = true;
      break;
    }
  }
  const uint8_t *metaBytes = reinterpret_cast<const uint8_t *>(&meta);
  for (size_t i = 0; !changed && i < sizeof(ConfigMeta); ++i)
    changed = EEPROM.read(sizeof(Configuration) + i) != metaBytes[i];
  if (!changed)
    return true;
  EEPROM.put(0, tmp);
  EEPROM.put(sizeof(Configuration), meta);
  return EEPROM.commit();
}
bool saveConfig() { return saveConfig(config); }
bool loadConfig()
{
  Configuration tmp;
  ConfigMeta meta;
  EEPROM.get(0, tmp);
  if (tmp.eepromVersion == 0xFFFF)
  {
    DBGLN(F("EEPROM empty"));
    return false;
  }
  xorConfig(tmp);
  if (tmp.eepromVersion != EEPROM_VERSION || !tmp.isValid())
  {
    if (tmp.eepromVersion == 4 || tmp.eepromVersion == 5)
    {
      const size_t oldSize = tmp.eepromVersion == 4 ? 448 : 452;
      EEPROM.get(oldSize, meta);
      if (meta.magic != CONFIG_META_MAGIC || meta.crc != configCrc32(tmp, oldSize))
        return false;
      if (tmp.eepromVersion == 4) tmp.digitStyle = 0;
      if (tmp.eqamaShrouq == 0) tmp.eqamaShrouq = 10;
      tmp.eepromVersion = EEPROM_VERSION;
      tmp.hour24 = 0;
      if (!tmp.isValid() || !saveConfig(tmp))
        return false;
      config = tmp;
      DBGLN(F("EEPROM migrated: settings preserved"));
      return true;
    }
    DBGLN(F("EEPROM invalid"));
    return false;
  }
  EEPROM.get(sizeof(Configuration), meta);
  if (meta.magic == CONFIG_META_MAGIC && meta.crc != configCrc32(tmp))
  {
    DBGLN(F("EEPROM CRC invalid"));
    return false;
  }
  memcpy(&config, &tmp, sizeof(Configuration));
  // Keep EEPROM compatible with firmware that briefly exposed Arial Bold;
  // the removed family falls back to Basit without resetting other settings.
  if (config.fontFamily > 1)
  {
    config.fontFamily = 0;
    saveConfig();
    DBGLN(F("Removed font normalized to Basit"));
  }
  if (config.eqamaShrouq == 0)
  {
    config.eqamaShrouq = 10;
    saveConfig();
    DBGLN(F("Eqama sunrise default enabled"));
  }
  if (meta.magic != CONFIG_META_MAGIC)
  {
    DBGLN(F("EEPROM CRC initialized"));
    saveConfig();
  }
  return true;
}
void resetConfig()
{
  DBGLN(F("Reset defaults"));
  config = Configuration();
  config.setDefaults();
  safeCopy(config.masjedName, "0", sizeof(config.masjedName));
  safeCopy(config.message, "0", sizeof(config.message));
  saveConfig();
  makkahIshaMode = 0;
  savePrayerPrefs(makkahIshaMode);
  resetCardPrefsRuntime();
  saveCardPrefs();
}

bool isDST(int y, int m, int d)
{
  return CivilTime::isDstDate(y, m, d);
}
void calcHijri()
{
  if (sameCalendarDate(timeData.year, timeData.month, timeData.day,
                       hijriLastYear, hijriLastMonth, hijriLastDay))
    return;
  hijriLastYear = timeData.year;
  hijriLastMonth = timeData.month;
  hijriLastDay = timeData.day;

  int64_t a = (14 - timeData.month) / 12;
  int64_t y = (int64_t)timeData.year + 4800 - a;
  int64_t m = (int64_t)timeData.month + 12 * a - 3;
  int64_t jd = (int64_t)timeData.day + ((153 * m + 2) / 5) + 365 * y + y / 4 -
               y / 100 + y / 400 - 32045 + config.hijriDayCorrection;

  int64_t l = jd - 1948440 + 10632;
  int64_t n = (l - 1) / 10631;
  l = l - 10631 * n + 354;
  int64_t j = ((10985 - l) / 5316) * ((50 * l) / 17719) +
              (l / 5670) * ((43 * l) / 15238);
  l = l - ((30 - j) / 15) * ((17719 * j) / 50) - (j / 16) * ((15238 * j) / 43) +
      29;

  timeData.hMonth = (int8_t)((24 * l) / 709);
  timeData.hDay = (int8_t)(l - (709 * timeData.hMonth) / 24);
  timeData.hYear = (int16_t)(30 * n + j - 30);
}

void calcPrayers()
{
  if (prayerCache.valid &&
      sameCalendarDate(timeData.year, timeData.month, timeData.day,
                       prayerCache.lastYear, prayerCache.lastMonth,
                       prayerCache.lastDay))
    return;

  set_high_lats_adjust_method(AngleBased);

  const PrayerMethodConfig method =
      prayerMethodConfigForMode(config.calcMethod, makkahIshaMode);
  // The library's setters intentionally select Custom. Configure all three
  // twilight rules explicitly so no stale or zero-initialized field survives.
  set_fajr_angle(method.fajrAngle);
  if (method.maghribInMinutes)
    set_maghrib_minutes(method.maghribValue);
  else
    set_maghrib_angle(method.maghribValue);
  if (method.ishaInMinutes)
    set_isha_minutes(method.ishaValue);
  else
    set_isha_angle(method.ishaValue);
  set_asr_method(config.wayMethod == 0 ? Shafii : Hanafi);

  int dst =
      config.dst + (config.summerTime == 1 &&
                            isDST(timeData.year, timeData.month, timeData.day)
                        ? 1
                        : 0);

  double times[8];
  get_prayer_times(timeData.year, timeData.month, timeData.day, config.latitude,
                   config.longitude, dst, times);

  for (uint8_t i = 0; i <= 6; ++i)
  {
    uint8_t h, m;
    get_float_time_parts(times[i], h, m);
    prayerCache.times[i] = h * 60 + m;
  }
  prayerCache.lastYear = timeData.year;
  prayerCache.lastMonth = timeData.month;
  prayerCache.lastDay = timeData.day;
  prayerCache.valid = true;
  prayerCache.activeStart = prayerCache.activeEnd = INT16_MIN;
}

void updatePrayerWindow(int16_t nowMins)
{
  refreshEqamaCache();

  prayerCache.activeStart = INT16_MIN;
  prayerCache.activeEnd = INT16_MIN;

  for (uint8_t j = 0; j < 6; ++j)
  {
    uint8_t i = PRAYER_INDICES[j];
    int16_t pt = prayerCache.times[i];

    if (pt < 0 || pt >= 1440)
      continue;

    int16_t start = pt - 3;
    int16_t end;
    if (i == 2 && timeData.dow == 5)
      end = pt + config.sleepGomaaTime + 2;
    else
      end = pt + eqamaCache.times[i] + eqamaCache.sleepTime + 1;

    if (minuteInWindow(nowMins, start, end))
    {
      prayerCache.activeStart = start;
      prayerCache.activeEnd = end;
      return;
    }
  }
}

void syncRTC()
{
  if (!sysFlags.rtcOk())
  {
    return;
  }

  DateTime now = rtc.now();
  if (!now.isValid() || now.year() < MIN_VALID_YEAR || now.year() > 2099)
  {
    DBGLN(F("RTC read invalid"));
    sysFlags.setRtc(false);
    return;
  }
  bool dst = config.summerTime == 1 &&
             isDST(now.year(), now.month(), now.day());
  DateTime dt = dst ? now + TimeSpan(0, 1, 0, 0) : now;

  bool dayChanged = (dt.day() != timeData.day);
  timeData.year = dt.year();
  timeData.month = dt.month();
  timeData.day = dt.day();
  timeData.hour = dt.hour();
  timeData.min = dt.minute();
  timeData.sec = dt.second();
  timeData.dow = dt.dayOfTheWeek();
  lastRtcSyncMs = millis();

  if (dayChanged)
  {
    calcHijri();
    calcPrayers();
  }
}

void configureRTC()
{
  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
}

// Recover a slave-held I2C bus without waiting or restarting the ESP8266.
void recoverI2CBus()
{
  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
  if (digitalRead(SDA) == HIGH && digitalRead(SCL) == HIGH)
    return;

  pinMode(SCL, OUTPUT_OPEN_DRAIN);
  digitalWrite(SCL, HIGH);
  for (uint8_t pulse = 0; pulse < 9 && digitalRead(SDA) == LOW; ++pulse)
  {
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
  }

  pinMode(SDA, OUTPUT_OPEN_DRAIN);
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(SCL, HIGH);
  delayMicroseconds(5);
  digitalWrite(SDA, HIGH);
  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
  Wire.begin();
  Wire.setClock(100000);
}

bool beginRTCDevice(bool allowBusRecovery)
{
  if (!rtc.begin())
  {
    if (!allowBusRecovery)
      return false;
    recoverI2CBus();
    if (!rtc.begin())
      return false;
  }

  Wire.setClock(100000);
  configureRTC();
  if (rtc.lostPower())
    DBGLN(F("RTC lost power"));
  return true;
}

bool startRTC(bool allowBusRecovery)
{
  if (!beginRTCDevice(allowBusRecovery))
    return false;
  sysFlags.setRtc(true);
  syncRTC();
  return sysFlags.rtcOk();
}

void serviceRTC(unsigned long now)
{
  static unsigned long lastAttempt = 0;
  if (sysFlags.rtcOk() || now - lastAttempt < MS_RTC_RETRY)
    return;
  lastAttempt = now;
  if (startRTC(true))
  {
    DBGLN(F("RTC recovered"));
    refreshEqamaCache();
    invalidateCaches(prayerCache, hijriLastYear, hijriLastMonth,
                     hijriLastDay);
    calcHijri();
    calcPrayers();
    updatePrayerWindow(timeData.hour * 60 + timeData.min);
  }
}

void advanceSoftwareDate()
{
  if (timeData.year < MIN_VALID_YEAR || timeData.month < 1 || timeData.month > 12)
    return;
  timeData.dow = (timeData.dow + 1) % 7;
  if (++timeData.day <= daysInMonth(timeData.year, timeData.month))
    return;
  timeData.day = 1;
  if (++timeData.month <= 12)
    return;
  timeData.month = 1;
  ++timeData.year;
}

void tickSecond()
{
  static unsigned long lastSync = 0;
  if (++timeData.sec <= 59)
    return;
  timeData.sec = 0;
  if (++timeData.min <= 59)
  {
    if (millis() - lastSync >= MS_RTC_SYNC)
    {
      syncRTC();
      lastSync = millis();
    }
    return;
  }
  timeData.min = 0;
  bool dayRolled = ++timeData.hour > 23;
  if (dayRolled)
    timeData.hour = 0;
  syncRTC();
  if (dayRolled && !sysFlags.rtcOk())
  {
    advanceSoftwareDate();
    invalidateCaches(prayerCache, hijriLastYear, hijriLastMonth,
                     hijriLastDay);
    calcHijri();
    calcPrayers();
  }
  lastSync = millis();
}

void nextCard()
{
  const uint8_t current = dispState.cardNum > 0 ? dispState.cardNum - 1 : 0;
  dispState.cardNum = CardScheduler::nextIndex(cardOrder, cardEnabledMask,
                                               current) + 1;
  dispState.setFirstShow(true);
  dispState.cardTimer = millis() + MS_CARD_DURATION;
  smartText.reset();
}

void restoreCardAfterPriority()
{
  if (!priorityDisplayActive)
    return;
  priorityDisplayActive = false;
  if (priorityResumeCard >= 1 && priorityResumeCard <= CARD_COUNT)
  {
    dispState.cardNum = priorityResumeCard;
    dispState.setFirstShow(true);
    smartText.reset();
  }
}

static inline bool englishLetter(unsigned char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool asciiDigit(unsigned char c)
{
  return c >= '0' && c <= '9';
}

static int16_t displayTextWidth(const char *text)
{
  int16_t width = dmd.textWidth(text);
  for (size_t i = 1; text[i]; ++i)
  {
    const unsigned char previous = static_cast<unsigned char>(text[i - 1]);
    const unsigned char current = static_cast<unsigned char>(text[i]);
    const bool gap = config.language == LANG_EN
                         ? englishLetter(previous) && englishLetter(current)
                         : asciiDigit(previous) && asciiDigit(current);
    if (gap)
      ++width;
  }
  return width;
}

static void drawDisplayText(int16_t x, int8_t y, const char *text)
{
  if (config.language != LANG_EN)
  {
    dmd.drawText(x, y, text);
    return;
  }
  for (size_t i = 0; text[i]; ++i)
  {
    int charWidth = dmd.drawChar(x, y, text[i]);
    if (charWidth <= 0)
      charWidth = 1;
    x += charWidth;
    const unsigned char current = static_cast<unsigned char>(text[i]);
    const unsigned char next = static_cast<unsigned char>(text[i + 1]);
    const bool gap = config.language == LANG_EN
                         ? englishLetter(current) && englishLetter(next)
                         : asciiDigit(current) && asciiDigit(next);
    if (next && gap)
      ++x;
  }
}

void drawTopClock()
{
  static int8_t lastSec = -1;
  if (lastSec == timeData.sec)
    return;
  lastSec = timeData.sec;
  const int16_t clockArea = DISPLAY_PROFILE.clockAreaWidth;
  const bool showPeriod = !config.hour24 && clockArea >= 64;
  static char shapedPeriod[8] = {0};
  if (showPeriod)
  {
    if (config.language == LANG_AR)
      ArabicShaper::shape(timeData.hour < 12 ? "ص" : "م", shapedPeriod,
                          sizeof(shapedPeriod));
    else
      safeCopy(shapedPeriod, timeData.hour < 12 ? "AM" : "PM",
               sizeof(shapedPeriod));
  }
  char *p = fmtBuf;
  // DMDESP draws this line directly; the Arabic period marker is pre-shaped.
  if (showPeriod)
  {
    for (const char *q = shapedPeriod; *q && (p - fmtBuf) < BUF_SMALL - 2;
         ++q)
      *p++ = *q;
    *p++ = ' ';
  }
  int2str2(p, displayHour(timeData.hour, config.hour24));
  p += 2;
  *p++ = ':';
  int2str2(p, timeData.min);
  p += 2;
  *p++ = ':';
  int2str2(p, timeData.sec);
  p += 2;
  *p = '\0';
  const int16_t textWidth = displayTextWidth(fmtBuf);
  int16_t x = (clockArea - textWidth) / 2;
  if (x < 0)
    x = 0;
  dmd.drawFilledRect(0, 0, clockArea - 1,
                     DISPLAY_PROFILE.topLineHeight - 1, Bitmap::Black);
  drawDisplayText(x, 0, fmtBuf);
  markDisplayDirty();
}
void drawTopInfo()
{
  // A 1x1/2x1 panel has no usable separate right-hand information area.
  if (!DISPLAY_PROFILE.hasTopInfo)
    return;
  static int8_t last5 = -1;
  static int16_t cachedWidth = -1;
  static char cachedText[32] = {0};
  static char shapedText[64] = {0};
  static int8_t cachedLanguage = -1;
  if (cachedLanguage != config.language)
  {
    cachedLanguage = config.language;
    cachedDayIndex = -1;
    cachedMonthIndex = -1;
    cachedText[0] = '\0';
    last5 = -1;
  }
  int8_t sec5 = timeData.sec / 5;
  if (sec5 == last5)
    return;
  last5 = sec5;
  switch (sec5 & 3)
  {
  case 0:
    if (cachedDayIndex != timeData.dow)
    {
      safeCopyP(cachedDayName, dayName(), sizeof(cachedDayName));
      cachedDayIndex = timeData.dow;
    }
    safeCopy(fmtBuf, cachedDayName, sizeof(fmtBuf));
    break;
  case 1:
    snprintf(fmtBuf, sizeof(fmtBuf), "%04d-%02d-%02d", timeData.year,
             timeData.month, timeData.day);
    break;
  case 2:
  {
    if (timeData.hMonth < 1 || timeData.hMonth > 12 || timeData.hDay < 1)
    {
      safeCopy(fmtBuf, "--", sizeof(fmtBuf));
      break;
    }
    int8_t monthIdx = timeData.hMonth - 1;
    if (cachedMonthIndex != monthIdx)
    {
      safeCopyP(cachedMonthName, hijriMonthName(timeData.hMonth),
                sizeof(cachedMonthName));
      cachedMonthIndex = monthIdx;
    }
    // Build the complete date first; the fixed top zone below removes the
    // year/month detail only when the shaped text does not fit.
    if (timeData.hYear >= 1)
      snprintf(fmtBuf, sizeof(fmtBuf), "%d %s %d", timeData.hDay,
               cachedMonthName, timeData.hYear);
    else
      snprintf(fmtBuf, sizeof(fmtBuf), "%d %s", timeData.hDay,
               cachedMonthName);
    break;
  }
  case 3:
    safeCopyP(fmtBuf,
              config.language == LANG_EN ? MSG_PHONE_SILENT_EN
                                          : MSG_PHONE_SILENT_AR,
              sizeof(fmtBuf));
    break;
  }
  // Fixed top info must never overlap the clock. Prefer the year, then the
  // full month, then the compact month when space runs out.
  if ((sec5 & 3) == 2 && timeData.hMonth >= 1 && timeData.hMonth <= 12)
  {
    const int16_t infoWidth = DISPLAY_PROFILE.width -
                               DISPLAY_PROFILE.infoStart -
                               DISPLAY_PROFILE.edgeMargin;
    ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
    cachedWidth = displayTextWidth(shapedText);
    if (cachedWidth > infoWidth)
    {
      char shortMonth[24];
      safeCopyP(shortMonth,
                config.language == LANG_AR
                    ? HIJRI_MONTH_SHORT_AR[timeData.hMonth - 1]
                    : HIJRI_MONTH_SHORT_EN[timeData.hMonth - 1],
                sizeof(shortMonth));
      if (timeData.hYear >= 1)
        snprintf(fmtBuf, sizeof(fmtBuf), "%d %s %d", timeData.hDay,
                 shortMonth, timeData.hYear);
      else
        fmtBuf[0] = '\0';
      if (fmtBuf[0])
      {
        ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
        cachedWidth = displayTextWidth(shapedText);
      }
      if (cachedWidth > infoWidth)
      {
        snprintf(fmtBuf, sizeof(fmtBuf), "%d %s", timeData.hDay,
                 cachedMonthName);
        ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
        cachedWidth = displayTextWidth(shapedText);
      }
      if (cachedWidth > infoWidth)
      {
        snprintf(fmtBuf, sizeof(fmtBuf), "%d %s", timeData.hDay,
                 shortMonth);
      }
    }
  }
  if (strcmp(cachedText, fmtBuf) != 0)
  {
    safeCopy(cachedText, fmtBuf, sizeof(cachedText));
    ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
    cachedWidth = displayTextWidth(shapedText);
    const int16_t infoStart = DISPLAY_PROFILE.infoStart;
    int16_t x = DISPLAY_PROFILE.width - cachedWidth - DISPLAY_PROFILE.edgeMargin;
    if (x < infoStart)
      x = infoStart;
    dmd.drawFilledRect(infoStart, 0, DISPLAY_PROFILE.width - 1,
                       DISPLAY_PROFILE.topLineHeight - 1,
                       Bitmap::Black);
    drawDisplayText(x, 0, shapedText);
    markDisplayDirty();
  }
}
void drawTopLine()
{
  if (!dispParams.isSmall)
    return;
  drawTopClock();
  drawTopInfo();
}
void displayText(const char *text1, const char *text2 = nullptr,
                 bool flash = false, bool autoNext = true,
                 unsigned long now = 0)
{
  static int8_t lastAlt5 = -1;
  static bool showFirst = true;

  if (dispParams.isSmall)
    drawTopLine();

  const char *text = text1;
  if (!dispParams.isSmall && text2)
  {
    int8_t sec5 = (now / 1000) / 5;
    if (sec5 != lastAlt5)
    {
      lastAlt5 = sec5;
      showFirst = !showFirst;
      smartText.reset();
    }
    text = showFirst ? text1 : text2;
  }

  smartText.setArea(dispParams.areaY, dispParams.areaH);
  smartText.setSpeed(dispState.scrollSpeed);
  smartText.setRightToLeft(config.language == LANG_EN);

  bool done = flash ? smartText.drawFlash(text, dispParams.yPos, now)
                    : smartText.draw(text, dispParams.yPos, now);
  if (smartText.consumeDirty())
    markDisplayDirty();

  if (autoNext &&
      (done || (smartText.isStatic() &&
                static_cast<int32_t>(now - dispState.cardTimer) >= 0)))
    nextCard();
}

void checkPrayers(unsigned long now, int16_t nowMins)
{
  if (prayerCache.activeStart == INT16_MIN ||
      !minuteInWindow(nowMins, prayerCache.activeStart, prayerCache.activeEnd))
  {
    restoreCardAfterPriority();
    dispState.setStopCards(false);
    return;
  }
  if (!priorityDisplayActive)
  {
    priorityResumeCard = dispState.cardNum;
    priorityDisplayActive = true;
  }
  dispState.setStopCards(false);
  refreshEqamaCache();
  const bool friday = (timeData.dow == 5);
  const bool english = config.language == LANG_EN;
  const int8_t sleepTime = eqamaCache.sleepTime;
  char prayerNameBuf[16];
  for (uint8_t j = 0; j < 6; ++j)
  {
    uint8_t i = PRAYER_INDICES[j];
    const int16_t pt = prayerCache.times[i];
    if (pt < 0 || pt >= 1440)
      continue;
    const int8_t eq = eqamaCache.times[i];
    const int16_t since = signedMinuteDelta(nowMins, pt);
    const int16_t delta = -since;
    safeCopyP(prayerNameBuf, prayerName(i), sizeof(prayerNameBuf));
    if ((!friday && i != 2) && (delta > 5 || since > 30)) 
      continue;
    const bool skipEq = (i == 2 && friday);
    const PrayerPhase phase =
        skipEq ? PrayerPhase::Outside : prayerPhase(since, eq, sleepTime);
    // 1. Pre-Prayer (3 mins before: delta 3, 2, 1)
    if (phase == PrayerPhase::PreAzan)
    {
      dispState.setStopCards(true);
      uint8_t h, m;
      minToHM(pt, h, m);
      snprintf(dispState.runText, sizeof(dispState.runText), "%d:%02d  %s",
               displayHour(h, config.hour24), m, prayerNameBuf);
      snprintf(dispState.tempBuf, sizeof(dispState.tempBuf), "%d : %02d : %02d",
               displayHour(timeData.hour, config.hour24), timeData.min, timeData.sec);
      displayText(dispState.runText, dispState.tempBuf, false, true, now);
      return;
    }
    // 2. Azan (Now + 1 min: since 0, 1)
    if (phase == PrayerPhase::Azan)
    {
      dispState.setStopCards(true);
      if (i == 1)
      {
        displayText(prayerNameBuf, nullptr, true, true, now);
      }
      else
      {
        if (i == 2 && friday)
          safeCopyP(prayerNameBuf, english ? MSG_JUMAA_EN : MSG_JUMAA,
                    sizeof(prayerNameBuf));
        snprintf_P(dispState.runText, sizeof(dispState.runText),
                   english ? MSG_AZAN_EN : MSG_AZAN,
                   prayerNameBuf);
        displayText(dispState.runText, nullptr, true, true, now);
      }
      return;
    }
    // Gomaa Sleeping (Since 2 mins after Dohr for configurable duration)
    if (i == 2 && friday && config.sleepGomaaTime > 0 && since >= 2 &&
        since < config.sleepGomaaTime + 2)
    {
      dispState.setStopCards(true);
      sleeping();
      return;
    }
    if (phase == PrayerPhase::EqamaCountdown)
    {
      dispState.setStopCards(true);
      snprintf_P(dispState.runText, sizeof(dispState.runText),
                 english ? MSG_EQAMA_IN_EN : MSG_EQAMA_IN,
                 forwardMinuteDistance(nowMins, pt + eq));
      safeCopyP(dispState.tempBuf, english ? MSG_EQAMA_EN : MSG_EQAMA,
                sizeof(dispState.tempBuf));
      displayText(dispState.runText, dispState.tempBuf, false, true, now);
      return;
    }
    if (phase == PrayerPhase::EqamaSeconds)
    {
      dispState.setStopCards(true);
      snprintf_P(dispState.runText, sizeof(dispState.runText),
                 english ? MSG_EQAMA_SEC_EN : MSG_EQAMA_SEC,
                 60 - timeData.sec);
      displayText(dispState.runText, nullptr, false, true, now);
      return;
    }
    if (phase == PrayerPhase::Eqama)
    {
      dispState.setStopCards(true);
      safeCopyP(dispState.runText, english ? MSG_EQAMA_EN : MSG_EQAMA,
                sizeof(dispState.runText));
      displayText(dispState.runText, nullptr, true, true, now);
      return;
    }
    if (phase == PrayerPhase::Sleep)
    {
      dispState.setStopCards(true);
      sleeping();
      return;
    }
    if (phase == PrayerPhase::Azkar)
    {
      dispState.setStopCards(true);
      safeCopyP(dispState.runText,
                english ? AZKAR_AFTER_EN : AZKAR_AFTER,
                sizeof(dispState.runText));
      safeCopyP(dispState.tempBuf,
                english ? AZKAR_AFTER2_EN : AZKAR_AFTER2,
                sizeof(dispState.tempBuf));
      displayText(dispState.runText, dispState.tempBuf, false, true, now);
      return;
    }
  }
}

void drawCard(const char *text, bool checkZero = false)
{
  if (!text || !text[0] || (checkZero && text[0] == '0'))
  {
    nextCard();
    return;
  }

  unsigned long now = millis();
  if (dispState.firstShow())
  {
    dispState.setFirstShow(false);
    safeCopy(dispState.runText, text, sizeof(dispState.runText));
    dispState.cardTimer = now + MS_CARD_DURATION;
    smartText.reset();
  }
  displayText(dispState.runText, nullptr, false, true, now);
}

void cardZekr(PGM_P arabic, PGM_P english)
{
  char buf[150];
  safeCopyP(buf, config.language == LANG_EN ? english : arabic, sizeof(buf));
  drawCard(buf);
}
void cardMasjedName() { drawCard(config.masjedName, true); }
void cardMessage() { drawCard(config.message, true); }
void cardBigFontOnly(uint8_t type)
{
  static int8_t cachedLanguage = -1;
  if (cachedLanguage != config.language)
  {
    cachedLanguage = config.language;
    cachedDayIndex = -1;
    cachedMonthIndex = -1;
  }
  if (dispParams.isSmall &&
      !DISPLAY_PROFILE.bottomInfoCard(uint8_t(type + 2)))
  {
    nextCard();
    return;
  }
  switch (type)
  {
  case 0:
    if (cachedDayIndex != timeData.dow)
    {
      safeCopyP(cachedDayName, dayName(), sizeof(cachedDayName));
      cachedDayIndex = timeData.dow;
    }
    safeCopy(fmtBuf, cachedDayName, sizeof(fmtBuf));
    break;
  case 1:
    snprintf(fmtBuf, sizeof(fmtBuf), "%04d-%02d-%02d", timeData.year,
             timeData.month, timeData.day);
    break;
  case 2:
    if (timeData.hMonth < 1 || timeData.hMonth > 12 || timeData.hDay < 1 ||
        timeData.hYear < 1)
    {
      nextCard();
      return;
    }
    int8_t monthIdx = timeData.hMonth - 1;
    if (cachedMonthIndex != monthIdx)
    {
      safeCopyP(cachedMonthName, hijriMonthName(timeData.hMonth),
                sizeof(cachedMonthName));
      cachedMonthIndex = monthIdx;
    }
    // Keep the Arabic reading order (day, month, year); the shaper then
    // places the day at the right edge of the physical display.
    snprintf(fmtBuf, sizeof(fmtBuf), "%d %s %d", timeData.hDay,
             cachedMonthName, timeData.hYear);
    break;
  }
  drawCard(fmtBuf);
}
void cardRamadan()
{
  if (timeData.hMonth < 6 || timeData.hMonth >= 9)
  {
    nextCard();
    return;
  }
  int days = ((timeData.hMonth & 1) ? 30 : 29) - timeData.hDay;
  for (int8_t i = timeData.hMonth + 1; i < 9; i++)
    days += (i & 1) ? 30 : 29;
  if (days <= 0)
  {
    nextCard();
    return;
  }
  if (config.language == LANG_EN)
    snprintf_P(fmtBuf, sizeof(fmtBuf), CARD_RAMADAN_EN, days);
  else
    snprintf(fmtBuf, sizeof(fmtBuf), "باقي %d يوم على رمضان", days);
  drawCard(fmtBuf);
}
void cardEvent()
{
  PGM_P event = nullptr;
  PGM_P eventEn = nullptr;
  const int8_t d = timeData.hDay, m = timeData.hMonth;
  if (d >= 13 && d <= 15 && m != 9)
  {
    event = SYAM_AYAM_WHITE;
    eventEn = SYAM_AYAM_WHITE_EN;
  }
  else if ((m == 12 && d >= 29) || (m == 1 && d == 1))
  {
    event = NEW_HIJRI_YEAR;
    eventEn = NEW_HIJRI_YEAR_EN;
  }
  else if (m == 1 && d >= 8 && d <= 10)
  {
    event = SYAM_ASHORA;
    eventEn = SYAM_ASHORA_EN;
  }
  else if (m == 9)
  {
    event = RAMADAN_HADITH;
    eventEn = RAMADAN_HADITH_EN;
  }
  else if (m == 10 && d == 1)
  {
    event = EID_EL_FITR;
    eventEn = EID_EL_FITR_EN;
  }
  else if (m == 12 && d >= 7 && d <= 9)
  {
    event = SYAM_YOUM_ARAFA;
    eventEn = SYAM_YOUM_ARAFA_EN;
  }
  else if (m == 12 && d >= 10 && d <= 13)
  {
    event = EID_AL_ADHAA;
    eventEn = EID_AL_ADHAA_EN;
  }
  if (!event)
  {
    nextCard();
    return;
  }
  char buf[150];
  safeCopyP(buf, config.language == LANG_EN ? eventEn : event, sizeof(buf));
  drawCard(buf);
}
void cardTemp()
{
  if (!sysFlags.rtcOk())
  {
    nextCard();
    return;
  }
  if (dispState.firstShow())
  {
    cachedTemp = (int)rtc.getTemperature() - 2;
    if (config.language == LANG_EN)
      snprintf_P(fmtBuf, sizeof(fmtBuf), CARD_TEMP_EN, cachedTemp);
    else
      snprintf(fmtBuf, sizeof(fmtBuf), "الحرارة %d درجة", cachedTemp);
  }
  drawCard(fmtBuf);
}
void cardClock()
{
  if (dispParams.isSmall)
  {
    nextCard();
    return;
  }
  if (dispState.firstShow())
  {
    dispState.setFirstShow(false);
    dispState.cardTimer = millis() + MS_SPLASH;
    smartText.reset();
  }
  // Fast clock formatting
  char *p = dispState.runText;
  uint8_t h12 = displayHour(timeData.hour, config.hour24);
  if (h12 >= 10)
    *p++ = '0' + h12 / 10;
  *p++ = '0' + h12 % 10;
  *p++ = ' ';
  *p++ = ':';
  *p++ = ' ';
  int2str2(p, timeData.min);
  p += 2;
  *p++ = ' ';
  *p++ = ':';
  *p++ = ' ';
  int2str2(p, timeData.sec);
  p += 2;
  *p = '\0';
  displayText(dispState.runText, nullptr, false, true, millis());
}
void cardPrayers()
{
  if (dispState.firstShow())
  {
    char *p = dispState.runText;
    // Logical order is Fajr -> Sunrise -> Dhuhr -> Asr -> Maghrib -> Isha.
    // ArabicShaper performs the visual RTL reversal, so this puts Fajr at
    // the right edge and Isha at the left edge on the physical panel.
    for (uint8_t i = 0; i <= 6; ++i)
    {
      if (i == 5)
        continue;
      // Name first in logical text makes the visual RTL order "name time".
      char nameBuf[16];
      safeCopyP(nameBuf, prayerName(i), sizeof(nameBuf));
      const char *name = nameBuf;
      while (*name)
        *p++ = *name++;
      *p++ = ' ';
      uint8_t h, m;
      minToHM(prayerCache.times[i], h, m);
      p += sprintf(p, "%d:", displayHour(h, config.hour24));
      int2str2(p, m);
      p += 2;
      *p++ = ' ';
      *p++ = ' ';
      *p++ = ' ';
    }
    *p = '\0';
    dispState.setFirstShow(false);
    dispState.cardTimer = millis() + MS_CARD_DURATION;
    smartText.reset();
  }
  displayText(dispState.runText, nullptr, false, true, millis());
}
void cardZekrAllah() { cardZekr(ZEKR_ALLAH, ZEKR_ALLAH_EN); }
void cardZekrMohamed() { cardZekr(ZEKR_MOHAMED, ZEKR_MOHAMED_EN); }
// void cardZekrIslam() { cardZekr(MN_HOSN_ISLAM); }
void cardDayName() { cardBigFontOnly(0); }
void cardDateFull() { cardBigFontOnly(1); }
void cardHijriDate() { cardBigFontOnly(2); }
typedef void (*CardFn)();
const CardFn CARDS[] PROGMEM = {
    cardMasjedName, cardMessage, cardDayName, cardDateFull, cardHijriDate,
    cardRamadan, cardEvent, cardClock, cardPrayers, cardZekrAllah,
    cardZekrMohamed, cardTemp};
void handleCards()
{
  if (dispState.cardNum < 1 || dispState.cardNum > CARD_COUNT)
  {
    dispState.cardNum = CardScheduler::firstIndex(cardOrder, cardEnabledMask) + 1;
    dispState.setFirstShow(true);
  }
  const uint8_t current = dispState.cardNum - 1;
  if (!(cardEnabledMask & (uint16_t(1) << current)))
  {
    dispState.cardNum = CardScheduler::firstIndex(cardOrder, cardEnabledMask) + 1;
    dispState.setFirstShow(true);
  }
  CardFn fn = (CardFn)pgm_read_ptr(&CARDS[dispState.cardNum - 1]);
  fn();
}

void initAP()
{
  WiFi.mode(WIFI_AP);

  IPAddress ip(192, 168, 1, 177), mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, mask);

  char ssid[30];
  snprintf(ssid, sizeof(ssid), AP_SSID_PREFIX "%u", ESP.getChipId());
  WiFi.softAP(ssid, config.apPassword);
}

void serviceWiFi(unsigned long now)
{
  static unsigned long lastCheck = 0;
  if (now - lastCheck < 60000UL)
    return;
  lastCheck = now;
  WiFiMode_t mode = WiFi.getMode();
  if (mode == WIFI_AP || mode == WIFI_AP_STA)
    return;
  DBGLN(F("WiFi AP recovering"));
  initAP();
}

void serviceFactoryReset(unsigned long now)
{
  static unsigned long pressedAt = 0;
  static bool completed = false;
  if (digitalRead(FACTORY_RESET_PIN) != LOW)
  {
    pressedAt = 0;
    completed = false;
    return;
  }
  if (!pressedAt)
    pressedAt = now;
  if (!completed && now - pressedAt >= MS_FACTORY_RESET_HOLD)
  {
    completed = true;
    resetConfig();
    dmd.clear();
    dmd.setPixel(0, 0, Bitmap::White);
    markDisplayDirty();
    sysFlags.setRestart(true);
    sysFlags.restartAt = now + MS_RESTART_DELAY;
  }
}

bool validV2Body(const char *body)
{
  if (!body[0])
    return true;
  char keys[16][16];
  uint8_t keyCount = 0;
  const char *line = body;
  while (*line)
  {
    const char *end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);
    const char *eq = (const char *)memchr(line, '=', end - line);
    size_t keyLen = eq ? (size_t)(eq - line) : 0;
    if (!eq || keyLen == 0 || keyLen >= sizeof(keys[0]) || keyCount >= 16)
      return false;
    for (size_t i = 0; i < keyLen; ++i)
      if (!isalnum((unsigned char)line[i]) && line[i] != '_')
        return false;
    for (uint8_t i = 0; i < keyCount; ++i)
      if (strlen(keys[i]) == keyLen && !memcmp(keys[i], line, keyLen))
        return false;
    memcpy(keys[keyCount], line, keyLen);
    keys[keyCount++][keyLen] = '\0';
    for (const char *p = eq + 1; p < end; ++p)
      if (*p == '\\' && (++p >= end || (*p != '\\' && *p != 'n')))
        return false;
    if (!*end)
      break;
    line = end + 1;
    if (!*line)
      return false;
  }
  return true;
}

bool findV2Value(const char *key, char *out, size_t outSize)
{
  const size_t wanted = strlen(key);
  const char *line = v2Body;
  while (*line)
  {
    const char *end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);
    const char *eq = (const char *)memchr(line, '=', end - line);
    if (eq && (size_t)(eq - line) == wanted && !memcmp(line, key, wanted))
    {
      size_t len = 0;
      for (const char *p = eq + 1; p < end; ++p)
      {
        char c = *p;
        if (c == '\\')
          c = *++p == 'n' ? '\n' : '\\';
        if (len + 1 >= outSize)
          return false;
        out[len++] = c;
      }
      out[len] = '\0';
      return true;
    }
    line = *end ? end + 1 : end;
  }
  return false;
}

bool hasV2Key(const char *key)
{
  const size_t wanted = strlen(key);
  const char *line = v2Body;
  while (*line)
  {
    const char *end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);
    const char *eq = (const char *)memchr(line, '=', end - line);
    if (eq && (size_t)(eq - line) == wanted && !memcmp(line, key, wanted))
      return true;
    line = *end ? end + 1 : end;
  }
  return false;
}

bool hasRequestArg(const char *arg)
{
  return v2Request ? hasV2Key(arg) : server.hasArg(arg);
}

void sendV2(int status, const char *body)
{
  server.send(status, "text/plain; charset=utf-8", body);
  server.client().flush();
}

void sendV2Error(int status, const char *code)
{
  char body[64];
  snprintf(body, sizeof(body), "v=2\nok=0\nerror=%s", code);
  if (v2Request)
    sendV2(status, body);
  else
    server.send(status, "text/plain", body);
}

bool beginV2(const char *path, bool allowedBeforePasswordChange)
{
  (void)path;
  (void)allowedBeforePasswordChange; // kept for source/API compatibility
  v2Request = false;
  String body = server.arg("plain");
  if (body.length() >= sizeof(v2Body))
  {
    sendV2Error(400, "body_too_large");
    return false;
  }
  body.toCharArray(v2Body, sizeof(v2Body));
  if (!validV2Body(v2Body))
  {
    sendV2Error(400, "bad_body");
    return false;
  }
  // The AP password is the access-control boundary. Keep the compact v2
  // body format without a second application authentication layer.
  v2Request = true;
  return true;
}

void endV2() { v2Request = false; }

bool getArg(const char *arg, char *buf, size_t len)
{
  if (!hasRequestArg(arg))
    return false;
  if (v2Request)
    return findV2Value(arg, buf, len);
  return unBlur(server.arg(arg).c_str(), buf, len);
}

bool parseLongValue(const char *text, long minValue, long maxValue,
                    long &value)
{
  char *end = nullptr;
  long parsed = strtol(text, &end, 10);
  if (end == text || *end != '\0' || parsed < minValue || parsed > maxValue)
    return false;
  value = parsed;
  return true;
}

bool parseFloatValue(const char *text, float minValue, float maxValue,
                     float &value)
{
  char *end = nullptr;
  double parsed = strtod(text, &end);
  if (end == text || *end != '\0' || !isfinite(parsed) || parsed < minValue ||
      parsed > maxValue)
    return false;
  value = static_cast<float>(parsed);
  return true;
}

template <typename T>
bool readIntArg(const char *arg, T &value, long minValue, long maxValue)
{
  if (!hasRequestArg(arg))
    return true;
  long parsed;
  char buf[24];
  if (v2Request)
  {
    if (!findV2Value(arg, buf, sizeof(buf)))
      return false;
    if (!parseLongValue(buf, minValue, maxValue, parsed))
      return false;
  }
  else if (!parseLongValue(server.arg(arg).c_str(), minValue, maxValue, parsed))
  {
    if (!getArg(arg, buf, sizeof(buf)) ||
        !parseLongValue(buf, minValue, maxValue, parsed))
      return false;
  }
  value = static_cast<T>(parsed);
  return true;
}

bool readFloatArg(const char *arg, float &value, float minValue, float maxValue)
{
  if (!hasRequestArg(arg))
    return true;
  char buf[24];
  if (v2Request)
  {
    if (!findV2Value(arg, buf, sizeof(buf)))
      return false;
    return parseFloatValue(buf, minValue, maxValue, value);
  }
  if (parseFloatValue(server.arg(arg).c_str(), minValue, maxValue, value))
    return true;
  return getArg(arg, buf, sizeof(buf)) &&
         parseFloatValue(buf, minValue, maxValue, value);
}

bool isValidUtf8(const char *text)
{
  const uint8_t *p = reinterpret_cast<const uint8_t *>(text);
  while (*p)
  {
    if (*p < 0x80)
    {
      ++p;
      continue;
    }
    uint32_t codepoint;
    uint8_t continuation;
    if ((*p & 0xE0) == 0xC0)
    {
      codepoint = *p & 0x1F;
      continuation = 1;
      if (codepoint < 2)
        return false;
    }
    else if ((*p & 0xF0) == 0xE0)
    {
      codepoint = *p & 0x0F;
      continuation = 2;
    }
    else if ((*p & 0xF8) == 0xF0)
    {
      codepoint = *p & 0x07;
      continuation = 3;
    }
    else
      return false;
    ++p;
    for (uint8_t i = 0; i < continuation; ++i, ++p)
    {
      if ((*p & 0xC0) != 0x80)
        return false;
      codepoint = (codepoint << 6) | (*p & 0x3F);
    }
    if ((continuation == 2 && codepoint < 0x800) ||
        (continuation == 3 && codepoint < 0x10000) || codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF))
      return false;
  }
  return true;
}

bool readTextArg(const char *arg, char *value, size_t valueSize)
{
  if (!hasRequestArg(arg))
    return true;
  if (!v2Request && server.arg(arg).length() == 0 && valueSize > 0)
  {
    value[0] = '\0';
    return true;
  }
  return getArg(arg, value, valueSize) && isValidUtf8(value);
}
void respond(const char *msg)
{
  if (v2Request)
  {
    char body[96];
    const bool ok = strcmp(msg, "Error") != 0;
    snprintf(body, sizeof(body), "v=2\nok=%u\nresult=%s", ok ? 1 : 0, msg);
    sendV2(ok ? 200 : 422, body);
    return;
  }
  char buf[128];
  blur(msg, buf, sizeof(buf));
  server.send(200, "text/plain", buf);
}
uint8_t effectiveBrightness();
void applySettings()
{
  dispParams.isSmall = (config.isSmallFont == 0);
  dispParams.yPos = dispParams.isSmall ? DISPLAY_PROFILE.smallTextY : 0;
  dispParams.areaY = dispParams.isSmall ? DISPLAY_PROFILE.smallAreaY : 0;
  dispParams.areaH = dispParams.isSmall ? DISPLAY_PROFILE.smallAreaH
                                        : DISPLAY_PROFILE.height;

  smartText.setShaping(config.language == LANG_AR);
  const uint8_t *smallFont = BasitFont7;
  const uint8_t *largeFont = BasitFont16;
  if (config.fontFamily == 1)
  {
    smallFont = Tahoma7;
    largeFont = Tahoma16;
  }
  dmd.setFont(dispParams.isSmall ? smallFont : largeFont);
  if (config.digitStyle == 1)
  {
    smallFont = config.fontFamily == 1 ? Tahoma7Indic : BasitFont7Indic;
    largeFont = config.fontFamily == 1 ? Tahoma16Indic : BasitFont16Indic;
    dmd.setFont(dispParams.isSmall ? smallFont : largeFont);
  }
  dmd.setBrightness(effectiveBrightness());
  smartText.reset();
  static const uint16_t speeds[] = {70, 60, 50, 40, 30, 20};
  dispState.scrollSpeed =
      (config.scrollingSpeed <= 5) ? speeds[config.scrollingSpeed] : 40;
}

bool installConfig(const Configuration &next, bool refreshDisplay)
{
  if (!next.isValid() || !saveConfig(next))
    return false;
  config = next;
  if (refreshDisplay)
    applySettings();
  return true;
}

bool installSettings(const Configuration &next, uint8_t nextMakkahIshaMode)
{
  if (!next.isValid() || !validMakkahIshaMode(nextMakkahIshaMode) ||
      !saveConfig(next) || !savePrayerPrefs(nextMakkahIshaMode))
    return false;
  config = next;
  makkahIshaMode = nextMakkahIshaMode;
  return true;
}

void refreshAfterConfigChange()
{
  invalidateCaches(prayerCache, hijriLastYear, hijriLastMonth,
                   hijriLastDay);
  calcHijri();
  calcPrayers();
  updatePrayerWindow(timeData.hour * 60 + timeData.min);
  dispState.setFirstShow(true);
  dmd.clear();
  markDisplayDirty();
}

void saveRespond(const Configuration &next, const char *ok,
                 bool refreshDisplay = false)
{
  if (!installConfig(next, refreshDisplay))
  {
    respond("Error");
    return;
  }
  refreshAfterConfigChange();
  respond(ok);
}

// Auto power-save: dim to half brightness after Isha + powerSaveAfterIsha min,
// exit before Fajr - powerSaveBeforeFajr min.
uint8_t effectiveBrightness()
{
  if (!sysFlags.rtcOk() || !prayerCache.valid)
    return config.brightness;
  int nowMin = timeData.hour * 60 + timeData.min;
  int isha = prayerCache.times[6]; // Isha (minutes)
  int fajr = prayerCache.times[0]; // Fajr (minutes)
  int start = isha + config.powerSaveAfterIsha;
  while (start >= 1440)
    start -= 1440;
  int end = fajr - config.powerSaveBeforeFajr;
  while (end <= start)
    end += 1440;
  bool powerSave = minuteInWindow(nowMin, start, end);
  return powerSave && config.brightness > 1 ? (config.brightness / 2)
                                             : config.brightness;
}

void handleNotFound() { server.send(404, "text/plain", "404"); }
void handleRoot() { server.send(200, "text/plain", "Basit"); }
void handleCheckUpdate()
{
  server.send(200, "text/plain", "update,P,104,6,good");
}
void handleTestMode()
{
  sysFlags.setTest(true);
  respond("testModeOk");
}

bool appendV2Field(char *body, size_t bodySize, const char *key,
                   const char *value)
{
  size_t len = strlen(body);
  int written = snprintf(body + len, bodySize - len, "\n%s=", key);
  if (written < 0 || (size_t)written >= bodySize - len)
    return false;
  len += written;
  while (*value)
  {
    const char c = *value++;
    const char *escaped = c == '\n' ? "\\n" : (c == '\\' ? "\\\\" : nullptr);
    const size_t add = escaped ? 2 : 1;
    if (len + add >= bodySize)
      return false;
    if (escaped)
    {
      body[len++] = escaped[0];
      body[len++] = escaped[1];
    }
    else
      body[len++] = c;
  }
  body[len] = '\0';
  return true;
}

void handleGetData()
{
  static char buf[640], out[2400];
  if (v2Request)
  {
    snprintf(
        buf, sizeof(buf),
        "v=2\ndeviceId=%u\nprofile=%ux1\nmustChangePassword=%u\nfont=%u\nlang=%d\nhjc=%d\nf=%d\ns=%d\nz=%d\na=%d\nm=%d\ne=%d\ncountry=%d\ngovernment=%d\nlat=%.4f\nlon=%.4f\ncm=%d\nwm=%d\ndst=%d\nsummerTime=%d\nspt=%d\nsgt=%d\nbri=%u\ntS=%u\nfS=%u\npsai=%d\npsbf=%d\ncity=%d",
        ESP.getChipId(), DISPLAYS_WIDE, config.mustChangePassword,
        config.fontFamily, config.language, config.hijriDayCorrection,
        config.eqamaFajr, config.eqamaShrouq, config.eqamaZohr, config.eqamaAsr,
        config.eqamaMagrb, config.eqamaEsha, config.country, config.government,
        config.latitude, config.longitude, config.calcMethod, config.wayMethod,
        config.dst, config.summerTime,
        config.sleepPrayerTime, config.sleepGomaaTime, config.brightness,
        config.scrollingSpeed, config.isSmallFont,
        config.powerSaveAfterIsha, config.powerSaveBeforeFajr, config.city);
    safeCopy(out, buf, sizeof(out));
    if (!appendV2Field(out, sizeof(out), "hour24", config.hour24 ? "1" : "0") ||
        !appendV2Field(out, sizeof(out), "digits", config.digitStyle ? "1" : "0") ||
        !appendV2Field(out, sizeof(out), "makkahIsha",
                       makkahIshaMode ? "1" : "0"))
    {
      sendV2Error(500, "response_too_large");
      return;
    }
    if (!appendV2Field(out, sizeof(out), "masjedName", config.masjedName) ||
        !appendV2Field(out, sizeof(out), "message", config.message))
      sendV2Error(500, "response_too_large");
    else
      sendV2(200, out);
    dispState.setFirstShow(true);
    return;
  }
  snprintf(
      buf, sizeof(buf),
      "Data,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
      config.hijriDayCorrection, config.eqamaFajr, config.eqamaZohr,
      config.eqamaAsr, config.eqamaMagrb, config.eqamaEsha, config.country,
      config.government, config.latitude, config.longitude, config.calcMethod,
      config.wayMethod, config.dst, config.sleepPrayerTime,
      config.sleepGomaaTime, config.brightness, config.wifiPassword,
      config.scrollingSpeed, config.summerTime, config.isSmallFont,
      config.powerSaveAfterIsha, config.powerSaveBeforeFajr, config.city);
  blur(buf, out, sizeof(out));
  server.send(200, "text/plain", out);
  dispState.setFirstShow(true);
}

void handleGetCardSettings()
{
  char order[CARD_COUNT + 1];
  for (uint8_t i = 0; i < CARD_COUNT; ++i)
    order[i] = char('A' + cardOrder[i]);
  order[CARD_COUNT] = '\0';
  char body[64];
  snprintf(body, sizeof(body), "Cards,%u,%s", cardEnabledMask, order);
  respond(body);
}

void handleSetCardSettings()
{
  uint16_t mask = cardEnabledMask;
  char orderText[CARD_COUNT + 1];
  for (uint8_t i = 0; i < CARD_COUNT; ++i)
    orderText[i] = char('A' + cardOrder[i]);
  orderText[CARD_COUNT] = '\0';
  if (!readIntArg("mask", mask, 1, (1 << CARD_COUNT) - 1) ||
      !readTextArg("ord", orderText, sizeof(orderText)) ||
      strlen(orderText) != CARD_COUNT)
  {
    respond("Error");
    return;
  }
  uint8_t order[CARD_COUNT];
  for (uint8_t i = 0; i < CARD_COUNT; ++i)
  {
    if (orderText[i] < 'A' || orderText[i] >= 'A' + CARD_COUNT)
    {
      respond("Error");
      return;
    }
    order[i] = uint8_t(orderText[i] - 'A');
  }
  if (!CardScheduler::validOrder(order))
  {
    respond("Error");
    return;
  }
  cardEnabledMask = mask;
  memcpy(cardOrder, order, sizeof(cardOrder));
  if (!saveCardPrefs())
  {
    respond("Error");
    return;
  }
  dispState.cardNum = CardScheduler::firstIndex(cardOrder, cardEnabledMask) + 1;
  dispState.setFirstShow(true);
  smartText.reset();
  dmd.clear();
  markDisplayDirty();
  respond("cardSettingsOk");
}

void handleSetDateTime()
{
  if (!sysFlags.rtcOk() && !beginRTCDevice(true))
  {
    respond("Error");
    return;
  }
  Configuration next = config;
  int year = timeData.year >= MIN_VALID_YEAR ? timeData.year : MIN_VALID_YEAR;
  int month = timeData.month >= 1 ? timeData.month : 1;
  int day = timeData.day >= 1 ? timeData.day : 1;
  int hour = timeData.hour;
  int minute = timeData.min;
  int second = timeData.sec;
  if (!readIntArg("y", year, MIN_VALID_YEAR, 2099) ||
      !readIntArg("m", month, 1, 12) || !readIntArg("d", day, 1, 31) ||
      !readIntArg("h", hour, 0, 23) || !readIntArg("mi", minute, 0, 59) ||
      !readIntArg("s", second, 0, 59) ||
      !readIntArg("hjc", next.hijriDayCorrection, -3, 3) ||
      !readIntArg("st", next.summerTime, 0, 1) ||
      !readIntArg("dst", next.dst, -12, 14) || day > daysInMonth(year, month) ||
      !next.isValid())
  {
    respond("Error");
    return;
  }
  DateTime dt(year, month, day, hour, minute, second);
  if (next.summerTime == 1 && isDST(dt.year(), dt.month(), dt.day()))
    dt = dt - TimeSpan(0, 1, 0, 0);
  if (!installConfig(next, false))
  {
    respond("Error");
    return;
  }
  sysFlags.setRtc(true);
  rtc.adjust(dt);
  syncRTC();
  refreshAfterConfigChange();
  respond("dateTimeOk");
}
void handleSetEqama()
{
  Configuration next = config;
  if (!readIntArg("f", next.eqamaFajr, 0, 120) ||
      !readIntArg("s", next.eqamaShrouq, 0, 120) ||
      !readIntArg("z", next.eqamaZohr, 0, 120) ||
      !readIntArg("a", next.eqamaAsr, 0, 120) ||
      !readIntArg("m", next.eqamaMagrb, 0, 120) ||
      !readIntArg("e", next.eqamaEsha, 0, 120))
  {
    respond("Error");
    return;
  }
  saveRespond(next, "eqamaOk");
}
void handleSetLocation()
{
  Configuration next = config;
  if (!readIntArg("c", next.country, INT16_MIN, INT16_MAX) ||
      !readIntArg("g", next.government, INT16_MIN, INT16_MAX) ||
      !readIntArg("ct", next.city, INT16_MIN, INT16_MAX) ||
      !readFloatArg("lat", next.latitude, -90.0f, 90.0f) ||
      !readFloatArg("lon", next.longitude, -180.0f, 180.0f))
  {
    respond("Error");
    return;
  }
  saveRespond(next, "locationOk");
}
void handleSetSettings()
{
  Configuration next = config;
  uint8_t nextMakkahIshaMode = makkahIshaMode;
  if (!readIntArg("cm", next.calcMethod, 0, 4) ||
      !readIntArg("wm", next.wayMethod, 0, 1) ||
      !readIntArg("spt", next.sleepPrayerTime, 0, 120) ||
      !readIntArg("sgt", next.sleepGomaaTime, 0, 120) ||
      !readIntArg("bri", next.brightness, 0, 255) ||
      !readIntArg("psai", next.powerSaveAfterIsha, 0, 720) ||
      !readIntArg("psbf", next.powerSaveBeforeFajr, 0, 720) ||
      !readIntArg("digits", next.digitStyle, 0, 1) ||
      !readIntArg("hour24", next.hour24, 0, 1) ||
      !readIntArg("lang", next.language, LANG_AR, LANG_EN) ||
      !readIntArg("makkahIsha", nextMakkahIshaMode, 0, 1))
  {
    respond("Error");
    return;
  }
  if (!installSettings(next, nextMakkahIshaMode))
  {
    respond("Error");
    return;
  }
  applySettings();
  refreshAfterConfigChange();
  respond("settingsOk");
}
void handleSetMessage()
{
  Configuration next = config;
  if (!readIntArg("tS", next.scrollingSpeed, 0, 5) ||
      !readIntArg("fS", next.isSmallFont, 0, 1) ||
      !readTextArg("sms", next.message, sizeof(next.message)) ||
      !readTextArg("mN", next.masjedName, sizeof(next.masjedName)))
  {
    respond("Error");
    return;
  }
  saveRespond(next, "messageOk", true);
}

void handleSetFont()
{
  Configuration next = config;
  if (!readIntArg("font", next.fontFamily, 0, 1) ||
      !readIntArg("digits", next.digitStyle, 0, 1) ||
      !readIntArg("lang", next.language, LANG_AR, LANG_EN) ||
      !installConfig(next, true))
  {
    respond("Error");
    return;
  }
  dispState.setFirstShow(true);
  smartText.reset();
  dmd.clear();
  markDisplayDirty();
  respond("fontOk");
}

bool isValidApPassword(const char *password)
{
  const size_t len = strlen(password);
  if (len < 8 || len > 63)
    return false;
  for (size_t i = 0; i < len; ++i)
    if ((uint8_t)password[i] < 0x20 || (uint8_t)password[i] > 0x7E)
      return false;
  return true;
}

void handleV2SetNetwork()
{
  if (!beginV2("/v2/setNetwork", true))
    return;
  char password[64];
  if (!findV2Value("password", password, sizeof(password)) ||
      !isValidApPassword(password))
  {
    sendV2Error(422, "password");
    endV2();
    return;
  }
  Configuration next = config;
  safeCopy(next.apPassword, password, sizeof(next.apPassword));
  next.mustChangePassword = 0;
  if (!installConfig(next, false))
  {
    sendV2Error(500, "storage");
    endV2();
    return;
  }
  sendV2(200, "v=2\nok=1\nresult=networkOk\nrestart=1");
  // Ensure the authenticated response leaves the socket before the AP restarts.
  server.client().flush();
  yield();
  endV2();
  memset(password, 0, sizeof(password));
  sysFlags.setRestart(true);
  sysFlags.restartAt = millis() + MS_RESTART_DELAY;
}

void handleV2Health()
{
  if (!beginV2("/v2/health", true))
    return;
  const unsigned long nowMs = millis();
  const unsigned long syncAge = lastRtcSyncMs ? (nowMs - lastRtcSyncMs) / 1000UL : 0xFFFFFFFFUL;
  char body[384];
  snprintf(body, sizeof(body),
           "v=2\nok=1\nfw=%s\nconfig=%u\nreset=%s\nresetInfo=%s\nuptime=%lu\nheap=%u\nmaxBlock=%u\nfragmentation=%u\nrtc=%u\nrssi=%d\napClients=%u\nsyncAge=%lu\nprofile=%ux1",
           FIRMWARE_VERSION, EEPROM_VERSION, resetReason, resetInfo,
           nowMs / 1000UL,
           ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
           ESP.getHeapFragmentation(), sysFlags.rtcOk(),
           WiFi.RSSI(), WiFi.softAPgetStationNum(), syncAge, DISPLAYS_WIDE);
  sendV2(200, body);
  endV2();
}

void handleSelfTest()
{
  if (!beginV2("/v2/selfTest", true))
    return;
  const uint16_t heap = ESP.getFreeHeap();
  const bool configOk = config.isValid();
  const bool rtcOk = sysFlags.rtcOk();
  const bool wifiOk = WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA;
  const bool heapOk = heap >= 12000;
  const bool ok = configOk && rtcOk && displayReady && wifiOk && heapOk;
  char body[256];
  snprintf(body, sizeof(body),
           "v=2\nok=%u\nconfig=%u\nrtc=%u\ndisplay=%u\nwifi=%u\nheap=%u\nprofile=%ux1\nreset=%s",
           ok, configOk, rtcOk, displayReady, wifiOk, heap, DISPLAYS_WIDE,
           resetReason);
  // Keep the transport successful so the factory app can display every failed
  // check; the semantic result is carried by the compact `ok` field.
  sendV2(200, body);
  endV2();
}

void runV2(const char *path, void (*handler)(), bool allowBeforeChange = false)
{
  if (!beginV2(path, allowBeforeChange))
    return;
  handler();
  endV2();
}
void handleV2GetData() { runV2("/v2/getData", handleGetData, true); }
void handleV2SetDateTime() { runV2("/v2/setDateTime", handleSetDateTime); }
void handleV2SetEqama() { runV2("/v2/setEqama", handleSetEqama); }
void handleV2SetLocation() { runV2("/v2/setLocation", handleSetLocation); }
void handleV2SetSettings() { runV2("/v2/setSettings", handleSetSettings); }
void handleV2SetMessage() { runV2("/v2/setMessage", handleSetMessage); }
void handleV2GetCardSettings() { runV2("/v2/getCardSettings", handleGetCardSettings, true); }
void handleV2SetCardSettings() { runV2("/v2/setCardSettings", handleSetCardSettings); }
void handleV2SetFont() { runV2("/v2/setFont", handleSetFont); }
void handleV2TestMode() { runV2("/v2/testMode", handleTestMode); }
void handleV2SelfTest() { handleSelfTest(); }

static bool otaV2Upload = false;
static bool otaRejected = false;
static uint32_t otaExpectedSize = 0;
static uint32_t otaReceivedSize = 0;
static char otaError[24] = "ota_error";
static BearSSL::PublicKey otaPublicKey(OTA_VENDOR_PUBLIC_KEY_PEM);
static BearSSL::HashSHA256 otaSignatureHash;
static BearSSL::SigningVerifier otaSignatureVerifier(&otaPublicKey);
static bool otaSuccessPendingRestart = false;
static bool otaSuccessFrameReady = false;
static char otaSuccessText[BUF_SMALL];
void drawOtaLine(PGM_P text, int8_t y, const char *fallback = nullptr)
{
  char logical[BUF_SMALL];
  safeCopyP(logical, text, sizeof(logical));
  ArabicShaper::shape(logical, otaSuccessText, sizeof(otaSuccessText));
  if (fallback && displayTextWidth(otaSuccessText) > DISPLAY_PROFILE.width)
    safeCopy(otaSuccessText, fallback, sizeof(otaSuccessText));
  drawDisplayText(max(0, (DISPLAY_PROFILE.width - displayTextWidth(otaSuccessText)) / 2),
                  y, otaSuccessText);
}

void otaFail(const char *error)
{
  otaRejected = true;
  otaSuccessPendingRestart = false;
  otaSuccessFrameReady = false;
  safeCopy(otaError, error, sizeof(otaError));
  if (sysFlags.updateNow())
    Update.end();
  dmd.resume();
  sysFlags.setUpdate(false);
  dispState.setStopCards(false);
  dispState.setFirstShow(true);
  applySettings();
  dmd.clear();
  markDisplayDirty();
}

bool validHexSha256(const char *text)
{
  if (!text || strlen(text) != 64)
    return false;
  for (uint8_t i = 0; i < 64; ++i)
    if (!isxdigit((unsigned char)text[i]))
      return false;
  return true;
}

bool authorizeOtaV2()
{
  if (!server.hasArg("profile") || !server.hasArg("version") ||
      !server.hasArg("size") || !server.hasArg("sha256"))
  {
    otaFail("manifest");
    return false;
  }
  String profile = server.arg("profile");
  String version = server.arg("version");
  String size = server.arg("size");
  String hash = server.arg("sha256");
  char expectedProfile[12];
  snprintf(expectedProfile, sizeof(expectedProfile), "%ux1", DISPLAYS_WIDE);
  long parsedVersion = 0;
  long parsedSize = 0;
  if (profile != expectedProfile ||
      !parseLongValue(version.c_str(), 1, LONG_MAX, parsedVersion) ||
      !parseLongValue(size.c_str(), 1, LONG_MAX, parsedSize) ||
      static_cast<unsigned long>(parsedSize) >
          (ESP.getFreeSketchSpace() - 0x1000) ||
      !validHexSha256(hash.c_str()))
  {
    otaFail("manifest");
    return false;
  }

  // AP password is the only access control; image signature still protects
  // firmware integrity and authenticity.
  v2Request = true;
  otaExpectedSize = parsedSize;
  otaReceivedSize = 0;
  return true;
}

bool otaSucceeded = false;

void renderOtaSuccess()
{
  if (!otaSuccessFrameReady)
  {
    dmd.setFont(BasitFont7);
    dmd.clear();
    drawOtaLine(config.language == LANG_EN ? OTA_SUCCESS_EN : OTA_SUCCESS_AR, 4);
    if (displayTextWidth(otaSuccessText) > DISPLAY_PROFILE.width)
    {
      dmd.clear();
      drawOtaLine(config.language == LANG_EN ? OTA_PROGRESS_EN : OTA_SUCCESS_SHORT_AR,
                  0, "OTA");
      if (config.language == LANG_EN)
        drawDisplayText((DISPLAY_PROFILE.width - displayTextWidth("OK")) / 2,
                        DISPLAY_PROFILE.smallTextY, "OK");
      else
        drawOtaLine(OTA_SUCCESS_SECOND_AR, DISPLAY_PROFILE.smallTextY);
    }
    markDisplayDirty();
    otaSuccessFrameReady = true;
  }
}

void handleOTAComplete()
{
  if (otaV2Upload)
  {
    bool ok = otaSucceeded && !otaRejected && !Update.hasError();
    if (ok)
    {
      sendV2(200, "v=2\nok=1\nresult=updateOk\nrestart=1");
      endV2();
      otaV2Upload = false;
      otaSucceeded = false;
      otaSuccessPendingRestart = true;
      otaSuccessFrameReady = false;
      dispState.setStopCards(true);
      dmd.resume();
      sysFlags.setRestart(true);
      sysFlags.restartAt = millis() + MS_RESTART_DELAY;
    }
    else
    {
      if (v2Request)
        sendV2Error(422, otaError);
      else
        server.send(422, "text/plain", otaError);
      endV2();
      otaV2Upload = false;
      otaSucceeded = false;
    }
    return;
  }
  bool ok = otaSucceeded && !Update.hasError();
  server.send(200, "text/plain",
              ok ? "UpdateOk" : "UpdateError");
  sysFlags.setUpdate(false);
  otaSucceeded = false;
  if (ok)
  {
    otaSuccessPendingRestart = true;
    otaSuccessFrameReady = false;
    dispState.setStopCards(true);
    dmd.resume();
    sysFlags.setRestart(true);
    sysFlags.restartAt = millis() + MS_RESTART_DELAY;
  }
  else
  {
    otaSuccessPendingRestart = false;
    otaSuccessFrameReady = false;
    dispState.setStopCards(false);
    dispState.setFirstShow(true);
    dmd.resume();
    applySettings();
  }
}
void handleOTAUpload()
{
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    otaSucceeded = false;
    otaV2Upload = server.uri() == "/v2/update";
    otaRejected = false;
    otaSuccessPendingRestart = false;
    otaSuccessFrameReady = false;
    otaExpectedSize = otaReceivedSize = 0;
    otaError[0] = '\0';
    if (otaV2Upload && !authorizeOtaV2())
      return;
    if (!otaV2Upload && server.hasArg("size"))
    {
      long size;
      if (!parseLongValue(server.arg("size").c_str(), 1,
                          (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000, size))
      {
        otaFail("size");
        return;
      }
      otaExpectedSize = uint32_t(size);
    }
    DBGLN(F("OTA Start"));
    dispState.setStopCards(true);
    dmd.pause();
    uint32_t updateSize = otaV2Upload
                              ? otaExpectedSize
                              : ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
    if (otaV2Upload)
      Update.installSignature(&otaSignatureHash, &otaSignatureVerifier);
    else
      Update.installSignature(nullptr, nullptr);
    bool started = !otaRejected && Update.begin(updateSize);
    sysFlags.setUpdate(started);
    if (!started)
    {
      Update.printError(Serial);
      dispState.setStopCards(false);
      otaFail("storage");
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (otaRejected || !sysFlags.updateNow())
      return;
    if (otaExpectedSize && upload.currentSize > otaExpectedSize - otaReceivedSize)
    {
      otaFail("size");
      return;
    }
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
    {
      Update.printError(Serial);
      Update.end();
      sysFlags.setUpdate(false);
      otaFail("write");
    }
    else
    {
      otaReceivedSize += upload.currentSize;
    }
    ESP.wdtFeed();
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (otaExpectedSize && otaReceivedSize != otaExpectedSize)
      otaFail("size");
    otaSucceeded = !otaRejected && sysFlags.updateNow() && Update.end(true);
    sysFlags.setUpdate(false);
    if (otaSucceeded)
    {
      DBGLN(F("OTA Success"));
    }
    else
    {
      DBGLN(F("OTA Failed"));
      Update.printError(Serial);
      if (!otaRejected)
        otaFail("verify");
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    DBGLN(F("OTA Aborted"));
    if (sysFlags.updateNow())
      Update.end();
    sysFlags.setUpdate(false);
    otaSucceeded = false;
    otaSuccessPendingRestart = false;
    otaSuccessFrameReady = false;
    dispState.setStopCards(false);
    dispState.setFirstShow(true);
    dmd.resume();
    applySettings();
    dmd.clear();
    markDisplayDirty();
  }
}

void initServer()
{
  server.onNotFound(handleNotFound);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/checkUpdate", HTTP_GET, handleCheckUpdate);
  server.on("/testMode", HTTP_GET, handleTestMode);
  server.on("/getData", HTTP_GET, handleGetData);
  server.on("/getCardSettings", HTTP_GET, handleGetCardSettings);
  server.on("/setDateTime", HTTP_GET, handleSetDateTime);
  server.on("/setEqama", HTTP_GET, handleSetEqama);
  server.on("/setLocation", HTTP_GET, handleSetLocation);
  server.on("/setSettings", HTTP_GET, handleSetSettings);
  server.on("/setCardSettings", HTTP_GET, handleSetCardSettings);
  server.on("/setMessage", HTTP_GET, handleSetMessage);
  server.on("/setFont", HTTP_GET, handleSetFont);
  server.on("/getDisplaySettings", HTTP_GET, []() {
    char body[32];
    snprintf(body, sizeof(body), "Display,1,%u,%u", config.fontFamily,
             config.digitStyle);
    server.send(200, "text/plain", body);
  });
  server.on("/getTimeFormat", HTTP_GET, []() {
    server.send(200,"text/plain",config.hour24 ? "TimeFormat,1,1" : "TimeFormat,1,0");
  });
  server.on("/v2/getData", HTTP_POST, handleV2GetData);
  server.on("/v2/setDateTime", HTTP_POST, handleV2SetDateTime);
  server.on("/v2/setEqama", HTTP_POST, handleV2SetEqama);
  server.on("/v2/setLocation", HTTP_POST, handleV2SetLocation);
  server.on("/v2/setSettings", HTTP_POST, handleV2SetSettings);
  server.on("/v2/setMessage", HTTP_POST, handleV2SetMessage);
  server.on("/v2/getCardSettings", HTTP_POST, handleV2GetCardSettings);
  server.on("/v2/setCardSettings", HTTP_POST, handleV2SetCardSettings);
  server.on("/v2/setNetwork", HTTP_POST, handleV2SetNetwork);
  server.on("/v2/setFont", HTTP_POST, handleV2SetFont);
  server.on("/v2/testMode", HTTP_POST, handleV2TestMode);
  server.on("/v2/health", HTTP_POST, handleV2Health);
  server.on("/v2/selfTest", HTTP_POST, handleV2SelfTest);
  server.on("/v2/update", HTTP_POST, handleOTAComplete, handleOTAUpload);
  server.on("/updateMain", HTTP_POST, handleOTAComplete, handleOTAUpload);
  server.begin();
  DBGLN(F("Web server started"));
}

void testMode()
{
  static unsigned long testStartMs = 0;
  if (!testStartMs)
    testStartMs = millis();

  if (millis() - testStartMs >= 10000)
  {
    testStartMs = 0;
    sysFlags.setTest(false);
    dmd.setBrightness(effectiveBrightness());
    dmd.clear();
    markDisplayDirty();
    return;
  }
  dmd.setBrightness(TEST_BRIGHTNESS);
  dmd.drawFilledRect(0, 0, DISPLAY_PROFILE.width - 1,
                     DISPLAY_PROFILE.height - 1,
                     Bitmap::White);
  markDisplayDirty();
}

void recordLoopTelemetry(uint32_t startedUs, unsigned long now)
{
#if DEBUG
  static uint32_t maxLoopUs = 0;
  static unsigned long lastReport = 0;
  uint32_t elapsedUs = micros() - startedUs;
  if (elapsedUs > maxLoopUs)
    maxLoopUs = elapsedUs;
  if (now - lastReport >= 60000UL)
  {
    lastReport = now;
    Serial.printf("HEAP free=%u max=%u frag=%u%% loopMax=%luus\n",
                  ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                  ESP.getHeapFragmentation(),
                  static_cast<unsigned long>(maxLoopUs));
    maxLoopUs = 0;
  }
#else
  (void)startedUs;
  (void)now;
#endif
}

void splashScreen()
{
  char title[32];
  char shapedTitle[32];
  safeCopyP(title, config.language == LANG_EN ? SPLASH_TITLE_EN : SPLASH_TITLE,
            sizeof(title));
  ArabicShaper::shape(title, shapedTitle, sizeof(shapedTitle));
  dmd.clear();
  dmd.setFont(BasitFont7);
  drawDisplayText((DISPLAY_PROFILE.width - displayTextWidth(shapedTitle)) >> 1,
                  0, shapedTitle);
  drawDisplayText((DISPLAY_PROFILE.width - displayTextWidth("01030007330")) >> 1,
                  DISPLAY_PROFILE.smallTextY, "01030007330");
  markDisplayDirty();
  applySettings();
}

#if DEBUG == 1
void adjustTime()
{
  DBGLN(F("adjustTime"));
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // azan zohr
  // rtc.adjust(DateTime(2022, 2, 11, 12, 4, 48));  // Befor azan
  // rtc.adjust(DateTime(2022, 2, 11, 12, 5, 58));  // azan waiting
  // rtc.adjust(DateTime(2022, 2, 11, 12, 7, 38));  // azan waiting 2
  // rtc.adjust(DateTime(2022, 2, 11, 12, 7, 55));  // azan last Minute
  // rtc.adjust(DateTime(2022, 2, 11, 12, 8, 55));  // azan zohr
  // rtc.adjust(DateTime(2022, 2, 11, 12, 19, 55)); // eqama waiting
  // rtc.adjust(DateTime(2022, 2, 11, 12, 20, 55)); // eqama last minute
  // rtc.adjust(DateTime(2022, 2, 11, 12, 27, 30)); // Sleep last 2minute
  // rtc.adjust(DateTime(2022, 2, 11, 12, 27, 55)); // Sleep last minute
  // rtc.adjust(DateTime(2022, 2, 11, 12, 28, 55)); // Azkar
  // rtc.adjust(DateTime(2022, 2, 11, 12, 40, 40)); // Gomaa Finish
  // azan Esha 18:52
  // rtc.adjust(DateTime(2022, 2, 1, 18, 48, 45)); // Befor azan
  // rtc.adjust(DateTime(2022, 2, 1, 18, 49, 55)); // azan waiting Start
  // rtc.adjust(DateTime(2022, 2, 1, 18, 50, 55)); // azan waiting
  // rtc.adjust(DateTime(2022, 2, 1, 18, 50, 55)); // azan waiting End
  // rtc.adjust(DateTime(2022, 2, 1, 18, 51, 55)); // azan Esha Start
  // rtc.adjust(DateTime(2022, 2, 1, 18, 53, 55)); // azan Esha End
  // rtc.adjust(DateTime(2022, 2, 1, 18, 59, 55)); // eqama Start
  // rtc.adjust(DateTime(2022, 2, 1, 19, 0, 55));  // eqama last minute Start
  // rtc.adjust(DateTime(2022, 2, 1, 19, 1, 45));  // eqama last minute End
  // rtc.adjust(DateTime(2022, 2, 1, 19, 1, 55));  // eqama
  // rtc.adjust(DateTime(2022, 2, 1, 19, 2, 55));  // Sleep Start
  // rtc.adjust(DateTime(2022, 2, 1, 19, 9, 55));  // Sleep End
  // rtc.adjust(DateTime(2022, 2, 1, 19, 9, 55));  // Azkar Start
  // rtc.adjust(DateTime(2022, 2, 1, 19,10, 55));  // Azkar End
  // full Time
  // rtc.adjust(DateTime(2022, 2, 1, 12, 57, 55)); // full Time
  // Never leave an active rtc.adjust() in diagnostic builds. Uncomment one
  // test case locally only for a controlled prayer-window test, then restore it.
  // rtc.adjust(DateTime(2022, 2, 1, 23, 59, 50)); // Midnight
}
#else
#define adjustTime()
#endif

void setup()
{
  if (DEBUG)
    Serial.begin(115200);

  DBGLN(F("\n\n=== Basit Masjed Clock Starting ==="));
  DBG(F("Reset: "));
  String bootResetReason = ESP.getResetReason();
  safeCopy(resetReason, bootResetReason.c_str(), sizeof(resetReason));
  DBGLN(resetReason);
  String bootResetInfo = ESP.getResetInfo();
  bootResetInfo.replace('\r', ' ');
  bootResetInfo.replace('\n', ' ');
  safeCopy(resetInfo, bootResetInfo.c_str(), sizeof(resetInfo));

  EEPROM.begin(512);
  if (!loadConfig())
  {
    DBGLN(F("No config, using defaults"));
    resetConfig();
  }
  loadPrayerPrefs();
  loadCardPrefs();
  dispState.cardNum = CardScheduler::firstIndex(cardOrder, cardEnabledMask) + 1;
  pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);

  initAP();
  initServer();

  Wire.begin();
  Wire.setClock(100000);

  dmd.start();
  displayReady = true;
  // Render into a back buffer and present only completed frames. This avoids
  // visible clear-then-draw tearing while Arabic text is being redrawn.
  dmd.setDoubleBuffer(true);
  dmd.clear();
  markDisplayDirty();

  splashScreen();
  presentDisplayFrame();

  if (startRTC(true))
  {
    DBGLN(F("RTC OK"));
    adjustTime();
    syncRTC();
  }
  else
  {
    DBGLN(F("RTC FAIL"));
    sysFlags.setRtc(false);
  }

  updatePrayerWindow(timeData.hour * 60 + timeData.min);
  lastSecondMs = millis();

  ESP.wdtEnable(WDTO_8S);
  DBGLN(F("Setup done"));
}

void loop()
{
  const uint32_t loopStartedUs = micros();
  dmd.loop();
  const unsigned long now = millis();
  serviceRTC(now);
  updateRtcBootIndicator(now);
  serviceWiFi(now);
  serviceFactoryReset(now);
  yield();
  server.handleClient();

  // Re-apply auto brightness when crossing into/out of night.
  static unsigned long lastBrightMs = 0;
  if (now - lastBrightMs >= 60000) {
    lastBrightMs = now;
    dmd.setBrightness(effectiveBrightness());
  }

  // static bool wifiOff = false;
  // if (!wifiOff && now >= MS_WIFI_TIMEOUT) {
  //   WiFi.mode(WIFI_OFF);
  //   WiFi.forceSleepBegin();
  //   wifiOff = true;
  //   DBGLN(F("WiFi OFF"));
  // }

  if (sysFlags.restartNow() &&
      static_cast<int32_t>(now - sysFlags.restartAt) >= 0)
  {
    ESP.restart();
  }

  if (sysFlags.updateNow())
  {
    presentDisplayFrame();
    recordLoopTelemetry(loopStartedUs, now);
    return;
  }

  if (otaSuccessPendingRestart)
  {
    renderOtaSuccess();
    presentDisplayFrame();
    recordLoopTelemetry(loopStartedUs, now);
    return;
  }

  if (sysFlags.splashScreen())
  {
    if (now >= MS_SPLASH)
    {
      sysFlags.setSplash(false);
      syncRTC();
      dmd.clear();
      markDisplayDirty();
    }
    presentDisplayFrame();
    recordLoopTelemetry(loopStartedUs, now);
    return;
  }

  if (sysFlags.testMode())
  {
    testMode();
    presentDisplayFrame();
    recordLoopTelemetry(loopStartedUs, now);
    return;
  }

  // Second tick - update time and prayer window
  static int16_t cachedNowMins = -1;
  const uint8_t secondTicks = consumeSecondTicks(now, lastSecondMs);
  if (secondTicks)
  {
    for (uint8_t tick = 0; tick < secondTicks; ++tick)
      tickSecond();
    cachedNowMins = timeData.hour * 60 + timeData.min;

    // Update prayer window once per minute
    if (timeData.sec == 0)
      updatePrayerWindow(cachedNowMins);

    if (!sysFlags.rtcOk() || timeData.year < MIN_VALID_YEAR)
    {
      dispState.setStopCards(true);
      smartText.setArea(0, DISPLAY_PROFILE.height);
      safeCopyP(dispState.runText,
                config.language == LANG_EN ? RTC_FAIL_TEXT_EN : RTC_FAIL_TEXT,
                sizeof(dispState.runText));
      smartText.drawFlash(dispState.runText, dispParams.yPos, now);
      if (smartText.consumeDirty())
        markDisplayDirty();
    }
    else
    {
      checkPrayers(now, cachedNowMins);
    }
  }

  if (!dispState.stopCards())
    handleCards();
  presentDisplayFrame();
  recordLoopTelemetry(loopStartedUs, now);
}
