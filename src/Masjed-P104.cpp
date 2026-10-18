#include "SmartText.h"
#include "TextSpacing.h"
#include "ArabicShaper.h"
#include "TimeMath.h"
#include "CardScheduler.h"
#include "constants.h"
#include "globals.h"
#include "PrayerMethodContract.h"
#include "PrayerPhaseContract.h"
#include "CivilTimeContract.h"
#include "CompactApiContract.h"
#include "texts.h"
#include <Arduino.h>
#include <DMDESP.h>
#include <LittleFS.h>
#include "ClockStorageContract.h"
#include <BasitWebServer.h>
#include <ESP8266WebServer.h> // Keep PlatformIO dependency discovery; header guard skips stock parsing.
#ifndef BASIT_WEBSERVER_HARDENED
#error "Build must use the project-local ESP8266WebServer safety overlay"
#endif
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PrayerTimes.h>
#include <RTClib.h>
#include <Wire.h>
#include <Updater.h>
#include <fonts/BasitFont16.h>
#include <fonts/BasitFont7.h>
#include <fonts/TahomaBold16.h>
#include <fonts/Tahoma7.h>
#include <fonts/BasitFont7Indic.h>
#include <fonts/BasitFont16Indic.h>
#include <fonts/Tahoma7Indic.h>
#include <fonts/TahomaBold16Indic.h>
#if defined(P10_SMALL8_TEST) && P10_SMALL8_TEST
#include <fonts/TahomaP10_8_Trial.h>
#endif

// PrayerTimes.cpp defines these setters but its legacy header omits them.
void set_maghrib_minutes(double minutes);
void set_isha_minutes(double minutes);

ESP8266WebServer server(80);
RTC_DS3231 rtc;
DMDESP dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);
SmartText smartText(dmd, DISPLAY_PROFILE.width);
static bool v2Request = false;
static char v2Body[CompactApi::BODY_SIZE];
static bool displayFrameDirty = true;
static bool displayReady = false;
static uint32_t displayRevision = 0;
static bool sleepingFrameShown = false;
static unsigned long splashStartedMs = 0;
static SmartText splashTopText(dmd, DISPLAY_PROFILE.width);
static SmartText splashBottomText(dmd, DISPLAY_PROFILE.width);
static char splashCompany[48] = {0};
static char splashPhone[16] = {0};
static bool narrowSplashReady = false;
static bool narrowErrorFontForced = false;
#if DEBUG
static bool debugWifiDisabled = false;
#endif

#if DMDESP_SCAN_DIAGNOSTICS
struct ScanDiagnostics {
  uint32_t lastLatchUs = 0, count = 0, gapCount = 0, gapSumUs = 0;
  uint32_t gapMinUs = UINT32_MAX, gapMaxUs = 0, over5ms = 0, scanMaxUs = 0;
  uint32_t stageMaxUs[4] = {};
  uint32_t apiMaxUs = 0, sendMaxUs = 0, storeMaxUs = 0;
  const char *apiPath = "none";
  bool haveLatch = false;
};
static ScanDiagnostics scanDiag;
static uint32_t scanStageStartedUs = 0;

void resetDmdScanGap() { scanDiag.haveLatch = false; }

void recordDmdScan(uint32_t latchUs, uint32_t durationUs)
{
  ++scanDiag.count;
  if (durationUs > scanDiag.scanMaxUs) scanDiag.scanMaxUs = durationUs;
  if (scanDiag.haveLatch) {
    const uint32_t gapUs = latchUs - scanDiag.lastLatchUs;
    ++scanDiag.gapCount;
    scanDiag.gapSumUs += gapUs;
    if (gapUs < scanDiag.gapMinUs) scanDiag.gapMinUs = gapUs;
    if (gapUs > scanDiag.gapMaxUs) scanDiag.gapMaxUs = gapUs;
    if (gapUs > 5000UL) ++scanDiag.over5ms;
  }
  scanDiag.lastLatchUs = latchUs;
  scanDiag.haveLatch = true;
}

static void finishScanStage(uint8_t stage)
{
  const uint32_t nowUs = micros();
  const uint32_t elapsedUs = nowUs - scanStageStartedUs;
  if (elapsedUs > scanDiag.stageMaxUs[stage]) scanDiag.stageMaxUs[stage] = elapsedUs;
  scanStageStartedUs = nowUs;
}

static void reportScanDiagnostics(unsigned long now)
{
  static unsigned long lastReport = 0;
  if (now - lastReport < 10000UL) return;
  lastReport = now;
  Serial.printf("SCAN wifi=%u peers=%u n=%lu gapMin=%lu gapAvg=%lu gapMax=%lu over5ms=%lu scanMax=%lu serviceMax=%lu yieldMax=%lu httpMax=%lu renderMax=%lu apiMax=%lu sendMax=%lu storeMax=%lu path=%s us\n",
                unsigned(WiFi.getMode()), unsigned(WiFi.softAPgetStationNum()),
                (unsigned long)scanDiag.count,
                (unsigned long)(scanDiag.gapCount ? scanDiag.gapMinUs : 0),
                (unsigned long)(scanDiag.gapCount ? scanDiag.gapSumUs / scanDiag.gapCount : 0),
                (unsigned long)scanDiag.gapMaxUs, (unsigned long)scanDiag.over5ms,
                (unsigned long)scanDiag.scanMaxUs, (unsigned long)scanDiag.stageMaxUs[0],
                (unsigned long)scanDiag.stageMaxUs[1], (unsigned long)scanDiag.stageMaxUs[2],
                (unsigned long)scanDiag.stageMaxUs[3], (unsigned long)scanDiag.apiMaxUs,
                (unsigned long)scanDiag.sendMaxUs, (unsigned long)scanDiag.storeMaxUs,
                scanDiag.apiPath);
  // Exclude the diagnostic Serial print's own pause from the next scan interval.
  scanDiag = ScanDiagnostics{};
}
#endif

static uint16_t cardEnabledMask = (uint16_t(1) << CARD_COUNT) - 1;
static uint8_t cardOrder[CARD_COUNT] = {0, 1, 2, 3, 4, 5,
                                        6, 7, 8, 9, 10, 11};
static uint8_t priorityResumeCard = 0;
static bool priorityDisplayActive = false;

inline void markDisplayDirty()
{
  displayFrameDirty = true;
}

void clearDisplay()
{
  dmd.clear();
  ++displayRevision;
  sleepingFrameShown = false;
  smartText.reset();
  dispState.setFirstShow(true);
  markDisplayDirty();
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
  if (sleepingFrameShown)
    return;
  clearDisplay();
  dmd.setPixel(0, 0, Bitmap::White);
  dmd.setPixel(1, 0, Bitmap::White);
  markDisplayDirty();
  sleepingFrameShown = true;
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

struct PersistedState
{
  uint32_t magic;
  uint16_t version;
  uint16_t reservedVersion;
  uint32_t sequence;
  Configuration config;
  uint8_t makkahIshaMode;
  uint8_t ultraPowerSave;
  uint8_t powerSaveBrightness;
  uint8_t reserved;
  uint16_t enabledMask;
  uint8_t order[CARD_COUNT];
  uint32_t crc;
};

static constexpr uint32_t CONFIG_STORE_MAGIC = 0x42534653UL; // "BSFS"
static constexpr uint32_t CONFIG_STORE_VERSION = 1;
static constexpr char CONFIG_STORE_A[] = "/config-a.bin";
static constexpr char CONFIG_STORE_B[] = "/config-b.bin";
static constexpr char CONFIG_STORE_TMP[] = "/config.tmp";
static PersistedState persistedState{};
static PersistedState stagedState{};
static uint8_t persistedSlot = 1; // First successful write targets slot A.
static uint32_t persistedSequence = 0;
static bool configStoreReady = false;
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
uint32_t persistedStateCrc(const PersistedState &state)
{
  return bytesCrc32(reinterpret_cast<const uint8_t *>(&state),
                    offsetof(PersistedState, crc));
}

bool validPersistedState(const PersistedState &state)
{
  const uint16_t validMask = (uint16_t(1) << CARD_COUNT) - 1;
  return state.magic == CONFIG_STORE_MAGIC &&
         state.version == CONFIG_STORE_VERSION &&
         state.sequence > 0 && state.config.isValid() &&
         validMakkahIshaMode(state.makkahIshaMode) &&
         state.ultraPowerSave <= 1 && state.powerSaveBrightness <= 125 &&
         state.enabledMask != 0 && !(state.enabledMask & ~validMask) &&
         CardScheduler::validOrder(state.order) &&
         state.crc == persistedStateCrc(state);
}

bool readPersistedState(const char *path, PersistedState &state)
{
  File file = LittleFS.open(path, "r");
  if (!file || file.size() != sizeof(PersistedState))
  {
    if (file) file.close();
    return false;
  }
  const bool ok = file.read(reinterpret_cast<uint8_t *>(&state), sizeof(state)) == sizeof(state);
  file.close();
  if (!ok || state.crc != persistedStateCrc(state))
    return false;
  const bool cityMigrated = state.config.migrateCityId();
  if (ClockSettings::migrateEqama(state.config) || cityMigrated)
    state.crc = persistedStateCrc(state);
  return validPersistedState(state);
}

bool writePersistedState(const char *target, const PersistedState &state)
{
  LittleFS.remove(CONFIG_STORE_TMP);
  File file = LittleFS.open(CONFIG_STORE_TMP, "w");
  if (!file || file.write(reinterpret_cast<const uint8_t *>(&state), sizeof(state)) != sizeof(state))
  {
    if (file) file.close();
    LittleFS.remove(CONFIG_STORE_TMP);
    return false;
  }
  file.flush();
  file.close();
  LittleFS.remove(target);
  if (!LittleFS.rename(CONFIG_STORE_TMP, target))
  {
    LittleFS.remove(CONFIG_STORE_TMP);
    return false;
  }
  return true;
}

template <typename Stage>
bool saveStorage(Stage stage)
{
  if (!configStoreReady) return false;
  stagedState = persistedState;
  stage();
  // Do not consume a slot or a flash write when the requested state is the
  // same byte-for-byte payload already committed.
  const size_t payloadOffset = offsetof(PersistedState, config);
  const size_t payloadSize = offsetof(PersistedState, crc) - payloadOffset;
  if (persistedSequence && memcmp(reinterpret_cast<const uint8_t *>(&stagedState) + payloadOffset,
             reinterpret_cast<const uint8_t *>(&persistedState) + payloadOffset,
             payloadSize) == 0)
    return true;
  stagedState.magic = CONFIG_STORE_MAGIC;
  stagedState.version = CONFIG_STORE_VERSION;
  stagedState.reservedVersion = 0;
  stagedState.sequence = persistedSequence + 1;
  stagedState.reserved = 0;
  stagedState.crc = persistedStateCrc(stagedState);
  if (!validPersistedState(stagedState)) return false;
  const uint8_t targetSlot = uint8_t(1 - persistedSlot);
  const char *target = targetSlot == 0 ? CONFIG_STORE_A : CONFIG_STORE_B;
#if DMDESP_SCAN_DIAGNOSTICS
  const uint32_t storeStartedUs = micros();
#endif
  const bool writeOk = writePersistedState(target, stagedState);
#if DMDESP_SCAN_DIAGNOSTICS
  const uint32_t storeUs = micros() - storeStartedUs;
  if (storeUs > scanDiag.storeMaxUs) scanDiag.storeMaxUs = storeUs;
#endif
  if (!writeOk) return false;
  persistedState = stagedState;
  persistedSequence = stagedState.sequence;
  persistedSlot = targetSlot;
  return true;
}

constexpr uint8_t MIN_POWER_SAVE_BRIGHTNESS = 1;

void stagePowerPrefs(uint8_t brightness)
{
  stagedState.powerSaveBrightness = brightness < MIN_POWER_SAVE_BRIGHTNESS
                                        ? MIN_POWER_SAVE_BRIGHTNESS : brightness;
}

bool loadPowerPrefs()
{
  powerSaveBrightness = persistedState.powerSaveBrightness < MIN_POWER_SAVE_BRIGHTNESS
                             ? MIN_POWER_SAVE_BRIGHTNESS : persistedState.powerSaveBrightness;
  return true;
}

void stagePrayerPrefs(uint8_t mode, bool ultra)
{
  stagedState.makkahIshaMode = mode;
  stagedState.ultraPowerSave = ultra ? 1 : 0;
}

bool loadPrayerPrefs()
{
  makkahIshaMode = persistedState.makkahIshaMode;
  ultraPowerSave = persistedState.ultraPowerSave != 0;
  return true;
}

void resetCardPrefsRuntime()
{
  cardEnabledMask = (uint16_t(1) << CARD_COUNT) - 1;
  for (uint8_t i = 0; i < CARD_COUNT; ++i)
    cardOrder[i] = i;
}

void stageCardPrefs(uint16_t mask, const uint8_t order[CARD_COUNT])
{
  stagedState.enabledMask = mask;
  memcpy(stagedState.order, order, sizeof(stagedState.order));
}

bool loadCardPrefs()
{
  cardEnabledMask = persistedState.enabledMask;
  memcpy(cardOrder, persistedState.order, sizeof(cardOrder));
  return true;
}

void stageConfig(const Configuration &source)
{
  stagedState.config = source;
}
bool saveConfig(const Configuration &source)
{
  return source.isValid() && saveStorage([&]() { stageConfig(source); });
}
bool loadConfig()
{
  if (!configStoreReady) return false;
  PersistedState a{}, b{};
  const bool validA = readPersistedState(CONFIG_STORE_A, a);
  const bool validB = readPersistedState(CONFIG_STORE_B, b);
  if (!validA && !validB) return false;
  const bool useA = validA && (!validB || int32_t(a.sequence - b.sequence) > 0);
  persistedState = useA ? a : b;
  persistedSlot = useA ? 0 : 1;
  persistedSequence = persistedState.sequence;
  config = persistedState.config;
  makkahIshaMode = persistedState.makkahIshaMode;
  ultraPowerSave = persistedState.ultraPowerSave != 0;
  powerSaveBrightness = persistedState.powerSaveBrightness < MIN_POWER_SAVE_BRIGHTNESS
                             ? MIN_POWER_SAVE_BRIGHTNESS : persistedState.powerSaveBrightness;
  cardEnabledMask = persistedState.enabledMask;
  memcpy(cardOrder, persistedState.order, sizeof(cardOrder));
  return true;
}
bool resetConfig()
{
  DBGLN(F("Reset defaults"));
  Configuration next{};
  next.setDefaults();
  uint8_t order[CARD_COUNT];
  for (uint8_t i = 0; i < CARD_COUNT; ++i)
    order[i] = i;
  if (!saveStorage([&]() {
        stageConfig(next);
        stagePrayerPrefs(0, false);
        stagePowerPrefs(1);
        stageCardPrefs((uint16_t(1) << CARD_COUNT) - 1, order);
      }))
    return false;
  config = next;
  makkahIshaMode = 0;
  ultraPowerSave = false;
  powerSaveBrightness = 1;
  resetCardPrefsRuntime();
  return true;
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

  CivilTime::gregorianToHijri(timeData.year, timeData.month, timeData.day,
                              config.hijriDayCorrection, timeData.hYear,
                              timeData.hMonth, timeData.hDay);
}

void calcPrayers()
{
  if (!CivilTime::validDate(timeData.year, timeData.month, timeData.day))
  {
    for (int16_t &time : prayerCache.times)
      time = -1;
    prayerCache.valid = false;
    prayerCache.activeStart = prayerCache.activeEnd = INT16_MIN;
    return;
  }
  const int timezoneOffset = config.timezoneOffset +
                  (config.daylightSavingTime == 1 && rtcSeasonalAdjustment ? 1 : 0);
  if (prayerCache.valid &&
      prayerCache.timeZone == timezoneOffset &&
      sameCalendarDate(timeData.year, timeData.month, timeData.day,
                       prayerCache.lastYear, prayerCache.lastMonth,
                       prayerCache.lastDay))
    return;

  set_high_lats_adjust_method(AngleBased);

  const PrayerMethodSpec method =
      prayerMethodSpec(config.prayerCalculationMethod, makkahIshaMode);
  // The library's setters intentionally select Custom. Configure all three
  // twilight rules explicitly so no stale or zero-initialized field survives.
  set_fajr_angle(method.fajrAngle);
  set_maghrib_minutes(method.maghribMinutes);
  if (method.ishaMinutes)
    set_isha_minutes(method.ishaValue);
  else
    set_isha_angle(method.ishaValue);
  set_asr_method(config.asrMethod == 0 ? Shafii : Hanafi);

  double times[8];
  get_prayer_times(timeData.year, timeData.month, timeData.day, config.latitude,
                   config.longitude, timezoneOffset, times);

  for (uint8_t i = 0; i <= 6; ++i)
  {
    if (!isfinite(times[i]))
    {
      prayerCache.times[i] = -1;
      continue;
    }
    uint8_t h, m;
    get_float_time_parts(times[i], h, m);
    prayerCache.times[i] = h * 60 + m;
  }
  prayerCache.lastYear = timeData.year;
  prayerCache.lastMonth = timeData.month;
  prayerCache.lastDay = timeData.day;
  prayerCache.timeZone = timezoneOffset;
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
    {
      const bool skipSunriseSleep =
          i == PrayerIndex::Sunrise;
      const int8_t sleepMinutes = eqamaCache.sleepTime;
      const int16_t postEqamaWindow =
          skipSunriseSleep ? 0 : (sleepMinutes > 0 ? sleepMinutes + 1 : 0);
      end = pt + eqamaCache.times[i] + postEqamaWindow + 1;
    }

    if (minuteInWindow(nowMins, start, end))
    {
      prayerCache.activeStart = start;
      prayerCache.activeEnd = end;
      return;
    }
  }
}

// RTClib does not report short I2C reads. Check both the calendar registers
// and oscillator-stop status before publishing an authoritative time sample.
bool readRtcTime(DateTime &value)
{
  sysFlags.setRtcTimeValid(false);
  Wire.beginTransmission(0x68);
  Wire.write(uint8_t(0));
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(uint8_t(0x68), uint8_t(7)) != 7)
  {
    sysFlags.setRtc(false);
    return false;
  }
  uint8_t registers[7];
  for (uint8_t &reg : registers)
    reg = Wire.read();
  Wire.beginTransmission(0x68);
  Wire.write(uint8_t(0x0F));
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(uint8_t(0x68), uint8_t(1)) != 1)
  {
    sysFlags.setRtc(false);
    return false;
  }
  sysFlags.setRtc(true);
  const bool valid = CivilTime::decodeDs3231(registers, Wire.read(), value);
  sysFlags.setRtcTimeValid(valid);
  return valid;
}

void syncRTC()
{
  if (rtcAdjustmentRequired)
  {
    sysFlags.setRtcTimeValid(false);
    return;
  }
  if (!sysFlags.rtcOk())
  {
    return;
  }

  DateTime now;
  lastRtcSyncMs = millis(); // Pace failed samples too; a transient read is retried.
  if (!readRtcTime(now))
    return;
  bool daylightSavingActive = config.daylightSavingTime == 1 &&
             CivilTime::isDstAtStandardTime(now.year(), now.month(), now.day(),
                                            now.hour(), now.minute());
  DateTime dt = daylightSavingActive ? now + TimeSpan(0, 1, 0, 0) : now;

  const bool dayChanged = !sameCalendarDate(dt.year(), dt.month(), dt.day(),
                                            timeData.year, timeData.month,
                                            timeData.day);
  const bool seasonalChanged = rtcSeasonalAdjustment != daylightSavingActive;
  rtcSeasonalAdjustment = daylightSavingActive;
  timeData.year = dt.year();
  timeData.month = dt.month();
  timeData.day = dt.day();
  timeData.hour = dt.hour();
  timeData.min = dt.minute();
  timeData.sec = dt.second();
  timeData.dow = dt.dayOfTheWeek();
  lastRtcSyncMs = millis();
  lastSecondMs = lastRtcSyncMs; // This RTC sample already includes elapsed ticks.

  if (dayChanged || seasonalChanged || !prayerCache.valid)
  {
    calcHijri();
    calcPrayers();
    updatePrayerWindow(timeData.hour * 60 + timeData.min);
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
  sysFlags.setRtcTimeValid(false);
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
  if (timeData.year < CivilTime::MIN_YEAR || timeData.month < 1 || timeData.month > 12)
    return;
  timeData.dow = (timeData.dow + 1) % 7;
  if (++timeData.day <= CivilTime::daysInMonth(timeData.year, timeData.month))
    return;
  timeData.day = 1;
  if (++timeData.month <= 12)
    return;
  timeData.month = 1;
  ++timeData.year;
}

void tickSecond()
{
  if (++timeData.sec <= 59)
    return;
  timeData.sec = 0;
  if (++timeData.min <= 59)
    return;
  timeData.min = 0;
  if (++timeData.hour > 23)
  {
    timeData.hour = 0;
    advanceSoftwareDate();
  }
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
  sleepingFrameShown = false;
  if (priorityResumeCard >= 1 && priorityResumeCard <= CARD_COUNT)
  {
    dispState.cardNum = priorityResumeCard;
    dispState.setFirstShow(true);
    smartText.reset();
  }
}

static int16_t displayTextWidth(const char *text)
{
  int16_t width = dmd.textWidth(text);
  for (size_t i = 1; text[i]; ++i)
  {
    const unsigned char previous = static_cast<unsigned char>(text[i - 1]);
    const unsigned char current = static_cast<unsigned char>(text[i]);
    if (needsTextGap(previous, current))
      ++width;
  }
  return width;
}

static void drawDisplayText(int16_t x, int8_t y, const char *text,
                            Bitmap &target = dmd)
{
  for (size_t i = 0; text[i]; ++i)
  {
    int charWidth = target.drawChar(x, y, text[i]);
    if (charWidth <= 0)
      charWidth = 1;
    x += charWidth;
    const unsigned char current = static_cast<unsigned char>(text[i]);
    const unsigned char next = static_cast<unsigned char>(text[i + 1]);
    if (needsTextGap(current, next))
    {
      target.drawFilledRect(x, y, x, y + target.textHeight() - 1, Bitmap::Black);
      ++x;
    }
  }
}

static int8_t topFontY()
{
  static const uint8_t *cachedFont = nullptr;
  static int8_t y = 0;
  const uint8_t *font = static_cast<const uint8_t *>(dmd.font());
  if (font != cachedFont)
  {
    cachedFont = font;
    y = 0;
    if (!font || pgm_read_byte(font + 3) > 8)
      return y;
    const uint8_t count = pgm_read_byte(font + 5);
    const bool fixed = !pgm_read_byte(font) && !pgm_read_byte(font + 1);
    uint16_t bytes = fixed ? count * pgm_read_byte(font + 2) : 0;
    if (!fixed)
      for (uint16_t i = 0; i < count; ++i)
        bytes += pgm_read_byte(font + 6 + i);
    const uint8_t *glyphs = font + 6 + (fixed ? 0 : count);
    uint8_t rows = 0;
    for (uint16_t i = 0; i < bytes; ++i)
      rows |= pgm_read_byte(glyphs + i);
    // Remove only the font's globally blank leading rows, preserving its artwork.
    while (rows && !(rows & 1))
    {
      --y;
      rows >>= 1;
    }
  }
  return y;
}

static void drawTopText(int16_t start, int16_t width, const char *text,
                        bool alignRight, bool alignCenter = false,
                        bool hideColon = false)
{
  static Bitmap line(DISPLAY_PROFILE.width, DISPLAY_PROFILE.topLineHeight);
  if (!line.isValid())
    return;
  line.setFont(dmd.font());
  line.setTextColor(dmd.textColor());
  line.clear();
  const int16_t textWidth = displayTextWidth(text);
  const int16_t drawX = alignCenter
                            ? (width > textWidth ? (width - textWidth) / 2 : 0)
                            : (alignRight ? width - 1 - textWidth : 0);
  int16_t colonSourceX = -1;
  if (hideColon)
  {
    const char *colon = strchr(text, ':');
    if (colon)
    {
      colonSourceX = drawX;
      for (const char *q = text; q < colon; ++q)
      {
        int charWidth = line.charWidth(*q);
        if (charWidth <= 0)
          charWidth = 1;
        colonSourceX += charWidth;
        if (needsTextGap(static_cast<unsigned char>(*q),
                         static_cast<unsigned char>(q[1])))
          ++colonSourceX;
      }
    }
  }
  drawDisplayText(drawX,
                  topFontY(), text, line);
  int16_t firstInk = width, lastInk = -1;
  for (int16_t x = 0; x < width; ++x)
  {
    bool lit = false;
    for (uint8_t row = 0; row < DISPLAY_PROFILE.topLineHeight; ++row)
      lit |= line.pixel(x, row) == Bitmap::White;
    if (!lit)
      continue;
    firstInk = min(firstInk, x);
    lastInk = x;
  }
  dmd.fill(start, 0, width, DISPLAY_PROFILE.topLineHeight, Bitmap::Black);
  if (lastInk >= firstInk)
  {
    const int16_t inkWidth = min(int16_t(width - 1),
                                 int16_t(lastInk - firstInk + 1));
    const int16_t sourceX = alignRight ? lastInk - inkWidth + 1 : firstInk;
    const int16_t targetX = alignCenter
                                ? (width > inkWidth ? (width - inkWidth) / 2 : 0)
                                : (alignRight ? width - 1 - inkWidth : 1);
    // One outer pixel of margin; copying cannot cross this zone's boundaries.
    line.copy(sourceX, 0, inkWidth, DISPLAY_PROFILE.topLineHeight,
              &dmd, start + targetX, 0);
    if (colonSourceX >= sourceX && colonSourceX < sourceX + inkWidth)
    {
      int16_t colonWidth = line.charWidth(':');
      if (colonWidth <= 0)
        colonWidth = 1;
      dmd.fill(start + targetX + colonSourceX - sourceX, 0, colonWidth,
               DISPLAY_PROFILE.topLineHeight, Bitmap::Black);
    }
  }
  markDisplayDirty();
}

void drawTopClock()
{
  static int8_t lastSec = -1;
  static uint32_t lastRevision = UINT32_MAX;
  if (lastSec == timeData.sec && lastRevision == displayRevision)
    return;
  lastRevision = displayRevision;
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
  int2str2(p, CivilTime::displayHour(timeData.hour, config.hour24));
  p += 2;
  const bool hideColon = clockArea <= 32 && (timeData.sec & 1);
  *p++ = ':';
  int2str2(p, timeData.min);
  p += 2;
  if (clockArea > 32)
  {
    *p++ = ':';
    int2str2(p, timeData.sec);
    p += 2;
  }
  *p = '\0';
  drawTopText(0, clockArea, fmtBuf, false, DISPLAY_PROFILE.narrow(), hideColon);
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
  static uint32_t lastRevision = UINT32_MAX;
  if (cachedLanguage != config.language || lastRevision != displayRevision)
  {
    lastRevision = displayRevision;
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
    const int16_t infoWidth = DISPLAY_PROFILE.width - DISPLAY_PROFILE.infoStart - 1;
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
  ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
  const int16_t infoWidth = DISPLAY_PROFILE.width - DISPLAY_PROFILE.infoStart - 1;
  if (displayTextWidth(shapedText) > infoWidth)
  {
    switch (sec5 & 3)
    {
    case 0:
      if (config.language == LANG_EN)
        fmtBuf[3] = '\0';
      else
        memmove(fmtBuf, fmtBuf + 4, strlen(fmtBuf + 4) + 1); // Omit the weekday's "ال".
      break;
    case 1:
      snprintf(fmtBuf, sizeof(fmtBuf), "%02d/%02d/%02d", timeData.day,
               timeData.month, timeData.year % 100);
      ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
      if (displayTextWidth(shapedText) > infoWidth)
        snprintf(fmtBuf, sizeof(fmtBuf), "%02d/%02d", timeData.day, timeData.month);
      break;
    case 2:
      snprintf(fmtBuf, sizeof(fmtBuf), "%d/%d", timeData.hDay, timeData.hMonth);
      break;
    case 3:
      safeCopy(fmtBuf, config.language == LANG_EN ? "Silent" : "صامت",
               sizeof(fmtBuf));
      break;
    }
  }
  if (strcmp(cachedText, fmtBuf) != 0)
  {
    safeCopy(cachedText, fmtBuf, sizeof(cachedText));
    ArabicShaper::shape(fmtBuf, shapedText, sizeof(shapedText));
    drawTopText(DISPLAY_PROFILE.infoStart, infoWidth + 1, shapedText, true);
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
                 unsigned long now = 0, bool blinkColon = false,
                 bool allowSmallAlternate = false)
{
  static bool wasAlternating = false;
  static bool showFirst = true;
  static bool advanceAlternate = false;
  static uint32_t alternateStartedMs = 0;
  static uint32_t lastRevision = UINT32_MAX;

  if (dispParams.isSmall)
    drawTopLine();

  const bool alternating = text2 && text2[0] &&
                           (!dispParams.isSmall || allowSmallAlternate);
  if (!alternating || !wasAlternating || lastRevision != displayRevision)
  {
    showFirst = true;
    advanceAlternate = false;
    alternateStartedMs = now;
  }
  else if (advanceAlternate)
  {
    showFirst = !showFirst;
    advanceAlternate = false;
    alternateStartedMs = now;
    smartText.reset();
  }
  wasAlternating = alternating;
  lastRevision = displayRevision;
  const char *text = alternating && !showFirst ? text2 : text1;

  smartText.setArea(dispParams.areaY, dispParams.areaH);
  smartText.setSpeed(dispState.scrollSpeed);
  smartText.setRightToLeft(config.language == LANG_EN);

  bool done = flash
                  ? smartText.drawFlash(text, dispParams.yPos, now)
                  : (blinkColon
                         ? smartText.drawClock(text, dispParams.yPos, now,
                                               (timeData.sec & 1) != 0)
                         : smartText.draw(text, dispParams.yPos, now));
  if (smartText.consumeDirty())
    markDisplayDirty();

  if (alternating)
  {
    // Moving text owns a complete pass; fitting text keeps a five-second hold.
    if (smartText.isStatic()
            ? uint32_t(now - alternateStartedMs) >= MS_CARD_DURATION
            : done)
      advanceAlternate = true;
  }
  else if (autoNext &&
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
    safeCopyP(prayerNameBuf, prayerName(i), sizeof(prayerNameBuf));
    if (i == 2 && friday)
      safeCopyP(prayerNameBuf, english ? MSG_JUMAA_EN : MSG_JUMAA,
                sizeof(prayerNameBuf));
    const bool skipEq = (i == 2 && friday);
    const bool skipSunriseSleep =
        i == PrayerIndex::Sunrise;
    const PrayerPhase phase =
        prayerPhase(since, skipEq ? 0 : eq,
                    skipSunriseSleep ? 0 : sleepTime);
    // 1. Pre-Prayer (3 mins before: delta 3, 2, 1)
    if (phase == PrayerPhase::PreAzan)
    {
      dispState.setStopCards(true);
      uint8_t h, m;
      minToHM(pt, h, m);
      snprintf(dispState.runText, sizeof(dispState.runText), "%s %d:%02d",
               prayerNameBuf, CivilTime::displayHour(h, config.hour24), m);
      snprintf(dispState.tempBuf, sizeof(dispState.tempBuf), "%d : %02d : %02d",
               CivilTime::displayHour(timeData.hour, config.hour24), timeData.min, timeData.sec);
      displayText(dispState.runText, dispState.tempBuf, false, false, now);
      return;
    }
    // 2. Azan (Now + 1 min: since 0, 1)
    if (phase == PrayerPhase::Azan)
    {
      dispState.setStopCards(true);
      if (i == 1)
      {
        displayText(prayerNameBuf, nullptr, true, false, now);
      }
      else
      {
        if (DISPLAY_PROFILE.width <= 32)
        {
          // The 1x1 width cannot show the extra "Adhan/أذان" label.
          displayText(prayerNameBuf, nullptr, true, false, now);
        }
        else
        {
          snprintf_P(dispState.runText, sizeof(dispState.runText),
                     english ? MSG_AZAN_EN : MSG_AZAN,
                     prayerNameBuf);
          displayText(dispState.runText, nullptr, true, false, now);
        }
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
      const bool compactEqamaText = DISPLAY_PROFILE.width == 64;
      snprintf_P(dispState.runText, sizeof(dispState.runText),
                 english ? (compactEqamaText ? MSG_EQAMA_IN_2X1_EN
                                             : (dispParams.isSmall ? MSG_EQAMA_IN_EN
                                                                    : MSG_EQAMA_IN_LARGE_EN))
                         : (compactEqamaText ? MSG_EQAMA_IN_2X1
                                             : (dispParams.isSmall ? MSG_EQAMA_IN
                                                                    : MSG_EQAMA_IN_LARGE)),
                 forwardMinuteDistance(nowMins, pt + eq));
      safeCopyP(dispState.tempBuf, english ? MSG_PHONE_SILENT_EN : MSG_PHONE_SILENT_AR,
                sizeof(dispState.tempBuf));
      displayText(dispState.runText, dispState.tempBuf, false, false, now,
                  false, DISPLAY_PROFILE.narrow());
      return;
    }
    if (phase == PrayerPhase::EqamaSeconds)
    {
      dispState.setStopCards(true);
      if (DISPLAY_PROFILE.width <= 32)
        snprintf(dispState.runText, sizeof(dispState.runText), "%02d",
                 60 - timeData.sec);
      else if (DISPLAY_PROFILE.width == 64)
        snprintf_P(dispState.runText, sizeof(dispState.runText),
                   english ? MSG_EQAMA_SEC_2X1_EN : MSG_EQAMA_SEC_2X1,
                   60 - timeData.sec);
      else
        snprintf_P(dispState.runText, sizeof(dispState.runText),
                   english ? MSG_EQAMA_SEC_EN : MSG_EQAMA_SEC,
                   60 - timeData.sec);
      displayText(dispState.runText, nullptr, false, false, now);
      return;
    }
    if (phase == PrayerPhase::Eqama)
    {
      dispState.setStopCards(true);
      safeCopyP(dispState.runText, english ? MSG_EQAMA_EN : MSG_EQAMA,
                sizeof(dispState.runText));
      displayText(dispState.runText, nullptr, true, false, now);
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
      if (dispParams.isSmall)
      {
        safeCopyP(dispState.runText,
                  english ? AZKAR_AFTER_SMALL_EN : AZKAR_AFTER_SMALL,
                  sizeof(dispState.runText));
        displayText(dispState.runText, nullptr, false, false, now);
        return;
      }
      safeCopyP(dispState.runText,
                english ? AZKAR_AFTER_EN : AZKAR_AFTER,
                sizeof(dispState.runText));
      safeCopyP(dispState.tempBuf,
                english ? AZKAR_AFTER2_EN : AZKAR_AFTER2,
                sizeof(dispState.tempBuf));
      displayText(dispState.runText, dispState.tempBuf, false, false, now);
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
  uint8_t h12 = CivilTime::displayHour(timeData.hour, config.hour24);
  if (h12 >= 10)
    *p++ = '0' + h12 / 10;
  *p++ = '0' + h12 % 10;
  *p++ = ' ';
  *p++ = ':';
  *p++ = ' ';
  int2str2(p, timeData.min);
  p += 2;
  // On 1x1/2x1 large-font layouts the clock scrolls. Seconds change every
  // second and would restart that scroll before it can reach the end.
  if (!DISPLAY_PROFILE.narrow())
  {
    *p++ = ' ';
    *p++ = ':';
    *p++ = ' ';
    int2str2(p, timeData.sec);
    p += 2;
  }
  *p = '\0';
  displayText(dispState.runText, nullptr, false, true, millis(),
              DISPLAY_PROFILE.width == 64);
}
void cardPrayers()
{
  if (dispState.firstShow())
  {
    char *p = dispState.runText;
    // Logical order is Fajr -> Sunrise -> Dhuhr -> Asr -> Maghrib -> Isha.
    // ArabicShaper performs the visual RTL reversal, so this puts Fajr at
    // the right edge and Isha at the left edge on the physical panel.
    uint8_t prayerItem = 0;
    for (uint8_t i : PrayerIndex::EventIndices)
    {
      // Name first in logical text makes the visual RTL order "name time".
      char nameBuf[16];
      safeCopyP(nameBuf, prayerName(i), sizeof(nameBuf));
      const char *name = nameBuf;
      while (*name)
        *p++ = *name++;
      *p++ = ' ';
      if (prayerCache.times[i] < 0 || prayerCache.times[i] >= 1440)
      {
        *p++ = '-';
        *p++ = '-';
      }
      else
      {
        uint8_t h, m;
        minToHM(prayerCache.times[i], h, m);
        p += sprintf(p, "%d:", CivilTime::displayHour(h, config.hour24));
        int2str2(p, m);
        p += 2;
      }
      // Add a space between each prayer item, except the last one.
      if (++prayerItem < sizeof(PrayerIndex::EventIndices))
      {
        *p++ = ' ';
        *p++ = ' ';
      }
    }
    *p++ = ' ';
    *p++ = ' ';
    *p++ = ' ';
    *p++ = ' ';
    *p = '\0';
    dispState.setFirstShow(false);
    dispState.cardTimer = millis() + MS_CARD_DURATION;
    smartText.reset();
  }
  displayText(dispState.runText, nullptr, false, true, millis());
}
void cardZekrAllah() { cardZekr(ZEKR_ALLAH, ZEKR_ALLAH_EN); }
void cardZekrMohamed() { cardZekr(ZEKR_MOHAMED, ZEKR_MOHAMED_EN); }
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
#if DEBUG
  if (debugWifiDisabled) return;
#endif
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
    if (!resetConfig())
    {
      DBGLN(F("Factory reset save failed"));
      return;
    }
    clearDisplay();
    dmd.setPixel(0, 0, Bitmap::White);
    markDisplayDirty();
    sysFlags.setRestart(true);
    sysFlags.restartAt = now + MS_RESTART_DELAY;
  }
}

bool validV2Body(const char *body)
{
  return CompactApi::validBody(body);
}

int findV2Value(const char *key, char *out, size_t outSize)
{
  return CompactApi::findValue(v2Body, key, out, outSize);
}

void sendV2(int status, const char *body)
{
#if DMDESP_SCAN_DIAGNOSTICS
  const uint32_t sendStartedUs = micros();
#endif
  // Waiting for a TCP ACK here stalls the foreground matrix refresh.
  server.send(status, "text/plain; charset=utf-8", body);
#if DMDESP_SCAN_DIAGNOSTICS
  const uint32_t sendUs = micros() - sendStartedUs;
  if (sendUs > scanDiag.sendMaxUs) scanDiag.sendMaxUs = sendUs;
#endif
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
    sendV2Error(413, "body_too_large");
    return false;
  }
  body.toCharArray(v2Body, sizeof(v2Body));
  if (body.length() != strlen(v2Body) || !validV2Body(v2Body))
  {
    sendV2Error(400, "bad_body");
    return false;
  }
  char version[4];
  if (findV2Value("v", version, sizeof(version)) != 1 || strcmp(version, "2") != 0)
  {
    sendV2Error(400, "version");
    return false;
  }
  // The AP password is the access-control boundary. Keep the compact v2
  // body format without a second application authentication layer.
  v2Request = true;
  return true;
}

void endV2() { v2Request = false; }

template <typename T>
bool readIntArg(const char *arg, T &value, long minValue, long maxValue,
                bool required = false)
{
  long parsed;
  char buf[24];
  const int found = findV2Value(arg, buf, sizeof(buf));
  if (found <= 0)
    return found == 0 && !required;
  if (!CompactApi::parseInteger(buf, minValue, maxValue, parsed))
    return false;
  value = static_cast<T>(parsed);
  return true;
}

bool readFloatArg(const char *arg, float &value, float minValue, float maxValue,
                  bool required = false)
{
  char buf[24];
  const int found = findV2Value(arg, buf, sizeof(buf));
  if (found <= 0)
    return found == 0 && !required;
  return CompactApi::parseDecimal(buf, minValue, maxValue, value);
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
  const int found = findV2Value(arg, value, valueSize);
  if (found <= 0)
    return found == 0;
  return isValidUtf8(value);
}
void respond(const char *msg)
{
  if (!strcmp(msg, "Error"))
  {
    sendV2Error(422, "arguments");
    return;
  }
  char body[96];
  snprintf(body, sizeof(body), "v=2\nok=1\nresult=%s", msg);
  sendV2(200, body);
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
    largeFont = TahomaBold16;
  }
  dmd.setFont(dispParams.isSmall ? smallFont : largeFont);
  if (config.digitStyle == 1)
  {
    smallFont = BasitFont7Indic;
    largeFont = config.fontFamily == 1 ? TahomaBold16Indic : BasitFont16Indic;
    dmd.setFont(dispParams.isSmall ? smallFont : largeFont);
  }
  dmd.setBrightness(effectiveBrightness());
  smartText.reset();
  static const uint16_t speeds[] = {70, 60, 50, 40, 30, 20};
  dispState.scrollSpeed =
      (config.scrollingSpeed <= 5) ? speeds[config.scrollingSpeed] : 40;
}

void invalidateChangedCalculations(const Configuration &next)
{
  if (next.latitude != config.latitude || next.longitude != config.longitude ||
      next.prayerCalculationMethod != config.prayerCalculationMethod || next.asrMethod != config.asrMethod ||
      next.timezoneOffset != config.timezoneOffset || next.daylightSavingTime != config.daylightSavingTime)
    prayerCache.valid = false;
  if (next.hijriDayCorrection != config.hijriDayCorrection)
    hijriLastYear = -1;
}

bool installConfig(const Configuration &next, bool refreshDisplay)
{
  if (!next.isValid() || !saveConfig(next))
    return false;
  invalidateChangedCalculations(next);
  config = next;
  if (refreshDisplay)
    applySettings();
  return true;
}

bool installSettings(const Configuration &next, uint8_t nextMakkahIshaMode,
                     bool nextUltraPowerSave, uint8_t nextPowerSaveBrightness)
{
  if (!next.isValid() || !validMakkahIshaMode(nextMakkahIshaMode) ||
      nextPowerSaveBrightness > 125 ||
      !saveStorage([&]() {
        stageConfig(next);
        stagePrayerPrefs(nextMakkahIshaMode, nextUltraPowerSave);
        stagePowerPrefs(nextPowerSaveBrightness);
      }))
    return false;
  invalidateChangedCalculations(next);
  if (makkahIshaMode != nextMakkahIshaMode)
    prayerCache.valid = false;
  config = next;
  makkahIshaMode = nextMakkahIshaMode;
  ultraPowerSave = nextUltraPowerSave;
  powerSaveBrightness = nextPowerSaveBrightness < MIN_POWER_SAVE_BRIGHTNESS
                             ? MIN_POWER_SAVE_BRIGHTNESS : nextPowerSaveBrightness;
  return true;
}

void refreshAfterConfigChange()
{
  calcHijri();
  calcPrayers();
  updatePrayerWindow(timeData.hour * 60 + timeData.min);
  dispState.setFirstShow(true);
  clearDisplay();
  markDisplayDirty();
}

void saveRespond(const Configuration &next, const char *ok,
                 bool refreshDisplay = false)
{
  if (!installConfig(next, refreshDisplay))
  {
    sendV2Error(500, "storage");
    return;
  }
  refreshAfterConfigChange();
  respond(ok);
}

bool powerSaveWindowActive()
{
  if (!sysFlags.rtcOk() || !sysFlags.rtcTimeValid() || !prayerCache.valid ||
      config.powerSaveAfterIsha <= 0 || config.powerSaveBeforeFajr <= 0)
    return false;
  const int nowMin = timeData.hour * 60 + timeData.min;
  return nightWindowActive(nowMin, prayerCache.times[6], prayerCache.times[0],
                           config.powerSaveAfterIsha, config.powerSaveBeforeFajr);
}

// The same night window used for dimming also gates the optional ultra mode.
// The panel shows a dim breathing dot/diamond; RTC/Wi-Fi continue running so
// the clock remains reachable and exits automatically at Fajr.
bool ultraPowerSaveActive()
{
  return ultraPowerSave && powerSaveWindowActive();
}

bool updateUltraPowerSaveDisplay()
{
  static bool active = false;
  static uint8_t lastPhase = 255;
  static uint32_t lastRevision = UINT32_MAX;
  const bool next = ultraPowerSaveActive();
  if (!next)
  {
    if (active)
    {
      active = false;
      lastPhase = 255;
      dispState.setStopCards(false);
      dispState.setFirstShow(true);
      clearDisplay();
      applySettings();
      markDisplayDirty();
    }
    return false;
  }

  if (!active || lastRevision != displayRevision)
  {
    active = true;
    lastPhase = 255;
    dispState.setStopCards(true);
    clearDisplay();
    dmd.setPixel(DISPLAY_PROFILE.width / 2, DISPLAY_PROFILE.height / 2,
                 Bitmap::White);
    lastRevision = displayRevision;
    markDisplayDirty();
  }

  // One centered pixel is the quietest reliable heartbeat. Brightness fades
  // up and down smoothly without changing the framebuffer (no flicker).
  static const uint8_t level[] = {2, 3, 4, 5, 6, 7, 8, 9,
                                  10, 9, 8, 7, 6, 5, 4, 3};
  const uint8_t phase = (millis() / 120UL) & 0x0F;
  if (phase == lastPhase)
    return true;
  lastPhase = phase;
  dmd.setBrightness(level[phase]);
  return true;
}

// Auto power-save: use configured night brightness after Isha + offset,
// exit before Fajr - powerSaveBeforeFajr min.
uint8_t effectiveBrightness()
{
  if (ultraPowerSaveActive())
    return 1;
  const bool powerSave = powerSaveWindowActive();
  return powerSave ? (powerSaveBrightness < MIN_POWER_SAVE_BRIGHTNESS
                          ? MIN_POWER_SAVE_BRIGHTNESS : powerSaveBrightness)
                   : config.brightness;
}

void handleNotFound() { server.send(404, "text/plain", "404"); }
void handleRoot() { server.send(200, "text/plain", "Basit"); }
void handleCheckUpdate()
{
  char reply[48];
  snprintf(reply, sizeof(reply), "update,P,%s,%u,good", DEVICE_MODEL,
           unsigned(DEVICE_VERSION));
  server.send(200, "text/plain", reply);
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
  if (!persistedSequence) { sendV2Error(500, "storage"); return; }
  static char buf[384], out[1152]; // Worst escaped response measured below 1.1KB.
  char nightBrightnessText[5];
  snprintf(nightBrightnessText, sizeof(nightBrightnessText), "%u",
           powerSaveBrightness);
  const int written = snprintf(
      buf, sizeof(buf),
      "v=2\nok=1\ndeviceId=%u\nprofile=%ux1\nmustChangePassword=%u\nfont=%u\nlang=%d\nhjc=%d\nf=%d\ns=%d\nz=%d\na=%d\nm=%d\ne=%d\ncountry=%d\ngovernment=%d\nlat=%.4f\nlon=%.4f\nprayerMethod=%d\nasrMethod=%d\ntimezoneOffset=%d\ndaylightSavingTime=%d\nspt=%d\nsgt=%d\nbri=%u\ntS=%u\nfS=%u\npsai=%d\npsbf=%d\ncity=%d",
      ESP.getChipId(), DISPLAYS_WIDE, config.mustChangePassword,
      config.fontFamily, config.language, config.hijriDayCorrection,
      config.eqamaFajr, config.eqamaShrouq, config.eqamaZohr, config.eqamaAsr,
      config.eqamaMaghrib, config.eqamaEsha, config.country, config.government,
      config.latitude, config.longitude, config.prayerCalculationMethod, config.asrMethod,
      config.timezoneOffset, config.daylightSavingTime, config.sleepPrayerTime,
      config.sleepGomaaTime, config.brightness, config.scrollingSpeed,
      config.isSmallFont, config.powerSaveAfterIsha,
      config.powerSaveBeforeFajr, config.city);
  if (written < 0 || size_t(written) >= sizeof(buf))
  {
    sendV2Error(500, "response_too_large");
    return;
  }
  safeCopy(out, buf, sizeof(out));
  if (!appendV2Field(out, sizeof(out), "hour24", config.hour24 ? "1" : "0") ||
      !appendV2Field(out, sizeof(out), "digits", config.digitStyle ? "1" : "0") ||
      !appendV2Field(out, sizeof(out), "makkahIsha", makkahIshaMode ? "1" : "0") ||
      !appendV2Field(out, sizeof(out), "eco", ultraPowerSave ? "1" : "0") ||
      !appendV2Field(out, sizeof(out), "nbri", nightBrightnessText) ||
      !appendV2Field(out, sizeof(out), "masjedName", config.masjedName) ||
      !appendV2Field(out, sizeof(out), "message", config.message))
    sendV2Error(500, "response_too_large");
  else
    sendV2(200, out);
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
  if (!saveStorage([&]() { stageCardPrefs(mask, order); }))
  {
    sendV2Error(500, "storage");
    return;
  }
  cardEnabledMask = mask;
  memcpy(cardOrder, order, sizeof(cardOrder));
  dispState.cardNum = CardScheduler::firstIndex(cardOrder, cardEnabledMask) + 1;
  dispState.setFirstShow(true);
  smartText.reset();
  clearDisplay();
  markDisplayDirty();
  respond("cardSettingsOk");
}

void handleSetDateTime()
{
  Configuration next = config;
  int year = CivilTime::MIN_YEAR, month = 1, day = 1, hour = 0, minute = 0, second = 0;
  if (!readIntArg("y", year, CivilTime::MIN_YEAR, CivilTime::MAX_YEAR, true) ||
      !readIntArg("m", month, 1, 12, true) || !readIntArg("d", day, 1, 31, true) ||
      !readIntArg("h", hour, 0, 23, true) || !readIntArg("mi", minute, 0, 59, true) ||
      !readIntArg("s", second, 0, 59, true) ||
      !readIntArg("hjc", next.hijriDayCorrection, -3, 3) ||
      !readIntArg("daylightSavingTime", next.daylightSavingTime, 0, 1) ||
      !readIntArg("timezoneOffset", next.timezoneOffset, -12, 14) ||
      !readIntArg("hour24", next.hour24, 0, 1) ||
      !CivilTime::validDate(year, month, day) ||
      !next.isValid())
  {
    respond("Error");
    return;
  }
  DateTime dt(year, month, day, hour, minute, second);
  const DateTime requested = dt;
  if (next.daylightSavingTime == 1 && isDST(dt.year(), dt.month(), dt.day()))
    dt = dt - TimeSpan(0, 1, 0, 0);
  // Retain the approved base-date seasonal rule, but never silently accept
  // a local time inside its spring gap that cannot round-trip.
  const bool daylightSavingAtRtc = next.daylightSavingTime == 1 && isDST(dt.year(), dt.month(), dt.day());
  if (dt.year() < CivilTime::MIN_YEAR ||
      (daylightSavingAtRtc ? dt + TimeSpan(0, 1, 0, 0) : dt) != requested)
  {
    respond("Error");
    return;
  }
  if (!sysFlags.rtcOk() && !beginRTCDevice(true))
  {
    sendV2Error(422, "rtc");
    return;
  }
  sysFlags.setRtc(true);
  DateTime previous;
  const bool hadPrevious = readRtcTime(previous);
  const bool wasAdjustmentRequired = rtcAdjustmentRequired;
  const auto restorePrevious = [&]() {
    rtcAdjustmentRequired = wasAdjustmentRequired || !hadPrevious;
    if (hadPrevious)
    {
      rtc.adjust(previous);
      DateTime restored;
      if (!readRtcTime(restored) || restored < previous ||
          (restored - previous).totalseconds() > 2)
        rtcAdjustmentRequired = true;
    }
    syncRTC();
  };
  rtc.adjust(dt);
  DateTime verified;
  if (!readRtcTime(verified) || verified < dt || (verified - dt).totalseconds() > 2)
  {
    restorePrevious();
    sendV2Error(422, "rtc");
    return;
  }
  if (!installConfig(next, false))
  {
    restorePrevious();
    sendV2Error(500, "storage");
    return;
  }
  rtcAdjustmentRequired = false;
  syncRTC();
  refreshAfterConfigChange();
  respond("dateTimeOk");
}
void handleSetEqama()
{
  Configuration next = config;
  if (!readIntArg("f", next.eqamaFajr, 0, ClockSettings::MAX_EQAMA) ||
      !readIntArg("s", next.eqamaShrouq, 0, ClockSettings::MAX_EQAMA) ||
      !readIntArg("z", next.eqamaZohr, 0, ClockSettings::MAX_EQAMA) ||
      !readIntArg("a", next.eqamaAsr, 0, ClockSettings::MAX_EQAMA) ||
      !readIntArg("m", next.eqamaMaghrib, 0, ClockSettings::MAX_EQAMA) ||
      !readIntArg("e", next.eqamaEsha, 0, ClockSettings::MAX_EQAMA))
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
      !readIntArg("ct", next.city, 0, INT32_MAX) ||
      !readFloatArg("lat", next.latitude, -90.0f, 90.0f, true) ||
      !readFloatArg("lon", next.longitude, -180.0f, 180.0f, true))
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
  int nextUltraPowerSave = ultraPowerSave ? 1 : 0;
  int nextPowerSaveBrightness = powerSaveBrightness;
  if (!readIntArg("prayerMethod", next.prayerCalculationMethod, 0, 4) ||
      !readIntArg("asrMethod", next.asrMethod, 0, 1) ||
      !readIntArg("spt", next.sleepPrayerTime, 0, 120) ||
      !readIntArg("sgt", next.sleepGomaaTime, 0, 120) ||
      !readIntArg("bri", next.brightness, 0, 255) ||
      !readIntArg("psai", next.powerSaveAfterIsha, 0, 720) ||
      !readIntArg("psbf", next.powerSaveBeforeFajr, 0, 720) ||
      !readIntArg("nbri", nextPowerSaveBrightness, 0, 125) ||
      !readIntArg("digits", next.digitStyle, 0, 1) ||
      !readIntArg("hour24", next.hour24, 0, 1) ||
      !readIntArg("lang", next.language, LANG_AR, LANG_EN) ||
      !readIntArg("makkahIsha", nextMakkahIshaMode, 0, 1) ||
      !readIntArg("eco", nextUltraPowerSave, 0, 1))
  {
    respond("Error");
    return;
  }
  if (!installSettings(next, nextMakkahIshaMode, nextUltraPowerSave != 0,
                       static_cast<uint8_t>(nextPowerSaveBrightness)))
  {
    sendV2Error(500, "storage");
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

void handleSetDisplay()
{
  Configuration next = config;
  if (!readIntArg("font", next.fontFamily, 0, 1) ||
      !readIntArg("digits", next.digitStyle, 0, 1) ||
      !readIntArg("lang", next.language, LANG_AR, LANG_EN) ||
      !readIntArg("tS", next.scrollingSpeed, 0, 5) ||
      !readIntArg("fS", next.isSmallFont, 0, 1) ||
      !readTextArg("sms", next.message, sizeof(next.message)) ||
      !readTextArg("mN", next.masjedName, sizeof(next.masjedName)))
  {
    respond("Error");
    return;
  }
  saveRespond(next, "displayOk", true);
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
  clearDisplay();
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
  if (!beginV2("/setNetwork", true))
    return;
  char password[64];
  if (findV2Value("password", password, sizeof(password)) != 1 ||
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

// Unified API name used by both display families.
void handleV2SetPassword() { handleV2SetNetwork(); }

void handleV2ResetDefault()
{
  if (!beginV2("/resetDefault", true))
    return;
  if (!resetConfig())
  {
    sendV2Error(500, "storage");
    endV2();
    return;
  }
  sendV2(200, "v=2\nok=1\nresult=resetDefaultOk\nrestart=1");
  server.client().flush();
  yield();
  endV2();
  sysFlags.setRestart(true);
  sysFlags.restartAt = millis() + MS_RESTART_DELAY;
}

void handleV2Health()
{
  if (!beginV2("/health", true))
    return;
  const unsigned long nowMs = millis();
  const unsigned long syncAge = lastRtcSyncMs ? (nowMs - lastRtcSyncMs) / 1000UL : 0xFFFFFFFFUL;
  char body[384];
  snprintf(body, sizeof(body),
           "v=2\nok=1\nfw=%s\nconfig=%u\nreset=%s\nresetInfo=%s\nuptime=%lu\nheap=%u\nmaxBlock=%u\nfragmentation=%u\nrtc=%u\ntimeValid=%u\nrssi=%d\napClients=%u\nsyncAge=%lu\nprofile=%ux1",
           FIRMWARE_VERSION, EEPROM_VERSION, resetReason, resetInfo,
           nowMs / 1000UL,
           ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
           ESP.getHeapFragmentation(), sysFlags.rtcOk(), sysFlags.rtcTimeValid(),
           WiFi.RSSI(), WiFi.softAPgetStationNum(), syncAge, DISPLAYS_WIDE);
  sendV2(200, body);
  endV2();
}

void handleSelfTest()
{
  if (!beginV2("/selfTest", true))
    return;
  const uint16_t heap = ESP.getFreeHeap();
  const bool configOk = persistedSequence && config.isValid();
  const bool rtcOk = sysFlags.rtcOk();
  const bool timeValid = sysFlags.rtcTimeValid();
  const bool wifiOk = WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA;
  const bool heapOk = heap >= 12000;
  const bool ok = configOk && rtcOk && timeValid && displayReady && wifiOk && heapOk;
  char body[256];
  snprintf(body, sizeof(body),
           "v=2\nok=%u\nconfig=%u\nrtc=%u\ntimeValid=%u\ndisplay=%u\nwifi=%u\nheap=%u\nprofile=%ux1\nreset=%s",
           ok, configOk, rtcOk, timeValid, displayReady, wifiOk, heap, DISPLAYS_WIDE,
           resetReason);
  // Keep the transport successful so the factory app can display every failed
  // check; the semantic result is carried by the compact `ok` field.
  sendV2(200, body);
  endV2();
}

void runV2(const char *path, void (*handler)(), bool allowBeforeChange = false)
{
#if DMDESP_SCAN_DIAGNOSTICS
  const uint32_t apiStartedUs = micros();
#endif
  if (!beginV2(path, allowBeforeChange))
    return;
  handler();
  endV2();
#if DMDESP_SCAN_DIAGNOSTICS
  const uint32_t apiUs = micros() - apiStartedUs;
  if (apiUs > scanDiag.apiMaxUs) {
    scanDiag.apiMaxUs = apiUs;
    scanDiag.apiPath = path;
  }
#endif
}
void handleV2GetData() { runV2("/getData", handleGetData, true); }
void handleV2SetDateTime() { runV2("/setDateTime", handleSetDateTime); }
void handleV2SetEqama() { runV2("/setEqama", handleSetEqama); }
void handleV2SetLocation() { runV2("/setLocation", handleSetLocation); }
void handleV2SetSettings() { runV2("/setSettings", handleSetSettings); }
void handleV2SetMessage() { runV2("/setMessage", handleSetMessage); }
void handleV2SetDisplay() { runV2("/setDisplay", handleSetDisplay); }
void handleV2GetCardSettings() { runV2("/getCardSettings", handleGetCardSettings, true); }
void handleV2SetCardSettings() { runV2("/setCardSettings", handleSetCardSettings); }
void handleV2SetFont() { runV2("/setFont", handleSetFont); }
void handleV2TestMode() { runV2("/testMode", handleTestMode); }
void handleV2SelfTest() { handleSelfTest(); }

static bool otaSuccessPendingRestart = false;
static bool otaSuccessFrameReady = false;
static char otaSuccessText[BUF_SMALL];
static WiFiUDP otaProgressUdp;
static IPAddress otaProgressPeer;
static uint16_t otaProgressPort = 0;
static uint8_t otaProgressSent = 255;
static uint32_t otaExpectedSize = 0;
static uint32_t otaReceivedSize = 0;
static bool otaSizeProvided = false;
static bool otaUploadComplete = false;
static bool otaFileSeen = false;
static uint8_t otaTracePercent = 0;

static const char *otaUpdateError(const char *fallback)
{
  switch (Update.getError())
  {
  case UPDATE_ERROR_SPACE: return "space";
  case UPDATE_ERROR_SIZE:
  case UPDATE_ERROR_NO_DATA: return "incomplete";
  case UPDATE_ERROR_ERASE:
  case UPDATE_ERROR_WRITE:
  case UPDATE_ERROR_READ: return "write";
  case UPDATE_ERROR_OOM: return "memory";
  case UPDATE_ERROR_MAGIC_BYTE:
  case UPDATE_ERROR_FLASH_CONFIG:
  case UPDATE_ERROR_NEW_FLASH_CONFIG: return "image";
  default: return fallback;
  }
}

static void cancelOtaUpdate()
{
  if (!Update.isRunning()) return;
  // This installed ESP8266 core has no abort(). Force its bounds error
  // before end(), even if all expected bytes arrived without a closing
  // multipart boundary. write() rejects this size before reading the byte.
  uint8_t unused = 0;
  Update.write(&unused, Update.remaining() + 1);
  Update.end();
}

void sendOtaProgress(uint8_t progress)
{
  if (!otaProgressPort || progress == otaProgressSent)
    return;
  char message[8];
  snprintf(message, sizeof(message), "p=%u", progress);
  if (otaProgressUdp.beginPacket(otaProgressPeer, otaProgressPort))
  {
    otaProgressUdp.write(reinterpret_cast<const uint8_t *>(message), strlen(message));
    otaProgressUdp.endPacket();
    otaProgressSent = progress;
  }
}

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

void otaFail()
{
  otaSuccessPendingRestart = false;
  otaSuccessFrameReady = false;
  otaUploadComplete = false;
  cancelOtaUpdate();
  dmd.resume();
  sysFlags.setUpdate(false);
  dispState.setStopCards(false);
  dispState.setFirstShow(true);
  applySettings();
  clearDisplay();
  markDisplayDirty();
}

bool otaSucceeded = false;
static const char *otaErrorCode = "unknown";

void renderOtaSuccess()
{
  if (!otaSuccessFrameReady)
  {
    dmd.setFont(BasitFont7);
    clearDisplay();
    drawOtaLine(config.language == LANG_EN ? OTA_SUCCESS_EN : OTA_SUCCESS_AR, 4);
    if (displayTextWidth(otaSuccessText) > DISPLAY_PROFILE.width)
    {
      clearDisplay();
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
  if (otaSuccessPendingRestart || sysFlags.restartNow())
  {
    server.send(503, "text/plain", "UpdateError:busy");
    return;
  }
  // Finalize only after the entire multipart request, not just its file part.
  if (sysFlags.updateNow() && otaUploadComplete &&
      (!otaSizeProvided || otaReceivedSize == otaExpectedSize))
  {
    otaSucceeded = Update.end(true); // legacy contract: commit actual bytes received
    if (otaSucceeded) sendOtaProgress(99);
    else otaErrorCode = otaUpdateError("end");
  }
  bool ok = otaSucceeded && !Update.hasError();
#if DEBUG
  Serial.printf("OTA complete: ok=%u error=%s\n", ok, ok ? "none" : otaErrorCode);
#endif
  if (ok)
    server.send(200, "text/plain", "UpdateOk");
  else
  {
    char reply[32];
    snprintf(reply, sizeof(reply), "UpdateError:%s", otaErrorCode);
    server.send(400, "text/plain", reply);
  }
  sysFlags.setUpdate(false);
  otaSucceeded = false;
  otaUploadComplete = false;
  otaFileSeen = false;
  otaSizeProvided = false;
  if (ok)
  {
    otaSuccessPendingRestart = true;
    otaSuccessFrameReady = false;
    dispState.setStopCards(true);
    dmd.resume();
    sysFlags.setRestart(true);
    sysFlags.restartAt = millis() + MS_OTA_RESTART_DELAY;
  }
  else
  {
    otaFail();
  }
}
void handleOTAUpload()
{
  // An accepted image must remain untouched until eboot installs it.
  if (otaSuccessPendingRestart || sysFlags.restartNow()) return;
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    if (otaFileSeen || Update.isRunning())
    {
      otaErrorCode = "image"; // multiple file parts are not an OTA image
      otaFail();
      return;
    }
    otaFileSeen = true;
    otaSucceeded = false;
    otaUploadComplete = false;
    otaReceivedSize = otaExpectedSize = 0;
    otaSizeProvided = false;
    otaErrorCode = "incomplete";
    const String sizeText = server.hasArg("size") ? server.arg("size") : String();
    if (!sizeText.isEmpty())
    {
      bool sizeOk = sizeText.length() <= 10;
      for (unsigned int i = 0; sizeOk && i < sizeText.length(); ++i)
      {
        if (sizeText[i] < '0' || sizeText[i] > '9')
        {
          sizeOk = false;
          break;
        }
        const uint8_t digit = sizeText[i] - '0';
        if (otaExpectedSize > (UINT32_MAX - digit) / 10)
        {
          sizeOk = false;
          break;
        }
        otaExpectedSize = otaExpectedSize * 10 + digit;
      }
      if (!sizeOk || !otaExpectedSize)
      {
        otaErrorCode = "incomplete";
        otaFileSeen = false;
        otaFail();
        return;
      }
      otaSizeProvided = true;
    }
    const uint32_t available = ESP.getFreeSketchSpace();
    const uint32_t capacity = available > 0x1000 ? ((available - 0x1000) & 0xFFFFF000) : 0;
    if (!otaSizeProvided) otaExpectedSize = capacity;
    if (otaExpectedSize > capacity)
    {
      otaErrorCode = "space";
      return;
    }
    otaProgressPort = 0;
    if (server.hasArg("up"))
    {
      const long port = server.arg("up").toInt();
      if (port >= 1024 && port <= 65535)
      {
        otaProgressPort = static_cast<uint16_t>(port);
        otaProgressPeer = server.client().remoteIP();
      }
    }
    otaProgressSent = 255;
    otaTracePercent = 0;
    sendOtaProgress(0);
    otaSuccessPendingRestart = false;
    otaSuccessFrameReady = false;
    DBGLN(F("OTA Start"));
    dispState.setStopCards(true);
    dmd.pause();
    Update.installSignature(nullptr, nullptr);
    otaErrorCode = "incomplete";
    bool started = Update.begin(otaExpectedSize);
#if DEBUG
    Serial.printf("OTA begin: ok=%u expected=%u free=%u heap=%u\n", started,
                  otaExpectedSize, ESP.getFreeSketchSpace(), ESP.getFreeHeap());
#endif
    sysFlags.setUpdate(started);
    if (!started)
    {
      otaErrorCode = otaUpdateError("begin");
      Update.printError(Serial);
      dispState.setStopCards(false);
      otaFail();
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (!sysFlags.updateNow())
      return;
    if (otaUploadComplete || upload.currentSize > otaExpectedSize - otaReceivedSize)
    {
      otaErrorCode = "oversize";
      otaFail();
      return;
    }
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
    {
      otaErrorCode = otaUpdateError("write");
      Update.printError(Serial);
      otaFail();
      return;
    }
    otaReceivedSize += upload.currentSize;
    uint32_t progress = (static_cast<uint64_t>(otaReceivedSize) * 98) / otaExpectedSize;
    if (progress > 98) progress = 98;
#if DEBUG
    if (progress >= otaTracePercent + 25U)
    {
      otaTracePercent = static_cast<uint8_t>(progress);
      Serial.printf("OTA flash: bytes=%u progress=%u heap=%u\n",
                    Update.progress(), progress, ESP.getFreeHeap());
    }
#endif
    sendOtaProgress(static_cast<uint8_t>(progress));
    // Service SDK/Wi-Fi every chunk even when no progress peer is registered.
    yield();
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
#if DEBUG
    Serial.printf("OTA end: received=%u expected=%u\n", upload.totalSize, otaExpectedSize);
#endif
    if (!sysFlags.updateNow()) return;
    if (otaReceivedSize == 0 || upload.totalSize != otaReceivedSize ||
        (otaSizeProvided && otaReceivedSize != otaExpectedSize))
    {
      otaErrorCode = "incomplete";
      otaFail();
      return;
    }
    otaUploadComplete = true;
    sendOtaProgress(98); // validation/acceptance follows in handleOTAComplete
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    otaErrorCode = "aborted";
    DBGLN(F("OTA Aborted"));
    otaSucceeded = false;
    otaFileSeen = false;
    otaFail();
  }
}

void initServer()
{
  server.onNotFound(handleNotFound);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/checkUpdate", HTTP_GET, handleCheckUpdate);
  server.on("/getData", HTTP_POST, handleV2GetData);
  server.on("/setDateTime", HTTP_POST, handleV2SetDateTime);
  server.on("/setEqama", HTTP_POST, handleV2SetEqama);
  server.on("/setLocation", HTTP_POST, handleV2SetLocation);
  server.on("/setSettings", HTTP_POST, handleV2SetSettings);
  server.on("/setMessage", HTTP_POST, handleV2SetMessage);
  server.on("/setDisplay", HTTP_POST, handleV2SetDisplay);
  server.on("/getCardSettings", HTTP_POST, handleV2GetCardSettings);
  server.on("/setCardSettings", HTTP_POST, handleV2SetCardSettings);
  server.on("/setNetwork", HTTP_POST, handleV2SetNetwork);
  server.on("/setPassword", HTTP_POST, handleV2SetPassword);
  server.on("/resetDefault", HTTP_POST, handleV2ResetDefault);
  server.on("/setFont", HTTP_POST, handleV2SetFont);
  server.on("/testMode", HTTP_POST, handleV2TestMode);
  server.on("/health", HTTP_POST, handleV2Health);
  server.on("/selfTest", HTTP_POST, handleV2SelfTest);
  server.on("/updateMain", HTTP_POST, handleOTAComplete, handleOTAUpload);
  server.begin();
  DBGLN(F("Web server started"));
}

#if DEBUG
// Debug-only Wi-Fi switch for comparing display timing with and without AP traffic.
void serviceDebugSerial()
{
  while (Serial.available() > 0)
  {
    const int command = Serial.read();
    if (command == 'w' || command == 'W')
    {
      debugWifiDisabled = true;
      server.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      DBGLN(F("WiFi disabled by serial command"));
    }
    else if (command == 'a' || command == 'A')
    {
      debugWifiDisabled = false;
      initAP();
      server.begin();
      DBGLN(F("WiFi AP restored by serial command"));
    }
  }
}
#else
inline void serviceDebugSerial() {}
#endif

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
    clearDisplay();
    markDisplayDirty();
    return;
  }
  dmd.setBrightness(TEST_BRIGHTNESS);
  dmd.drawFilledRect(0, 0, DISPLAY_PROFILE.width - 1,
                     DISPLAY_PROFILE.height - 1,
                     Bitmap::White);
  markDisplayDirty();
}

#if defined(P10_FONT_VISUAL_TEST) && P10_FONT_VISUAL_TEST
static_assert(DISPLAYS_WIDE == 4 && DISPLAYS_HIGH == 1,
              "Font bench pages are sized for P104 4x1 only");
static const unsigned long FONT_TEST_HOLD_MS =
#if defined(P10_SMALL8_TEST) && P10_SMALL8_TEST
    10000UL;
static const char FONT_TEST_PAGES[][64] PROGMEM = {
    "الصلاة", "الإقامة", "المغرب"};
#else
    6000UL;
static const char FONT_TEST_PAGES[][64] PROGMEM = {
    "ب  ت  ث", "ج  ح  خ", "د  ذ  ر  ز", "س  ش  ص  ض",
    "ط  ظ  ع  غ", "ف  ق  ك", "ل  م  ن", "ه  ة  و  ي",
    "أ  إ  آ  ؤ  ئ", "لا  لأ  لإ  لآ",
    "تثبيت", "النبي", "سبحان", "الصلاة", "الإقامة",
    "الظهر", "المغرب", "العشاء"};
#endif

static bool renderFontVisualTest(unsigned long now)
{
  static uint8_t page = 0;
  static unsigned long pageStartedMs = 0;
  static bool started = false;
  const uint8_t count = sizeof(FONT_TEST_PAGES) / sizeof(FONT_TEST_PAGES[0]);
  if (page >= count)
    return false;
  if (started && now - pageStartedMs >= FONT_TEST_HOLD_MS)
  {
    ++page;
    started = false;
    if (page >= count)
    {
      applySettings();
      clearDisplay();
      markDisplayDirty();
      dispState.setFirstShow(true);
      dispState.cardTimer = now + MS_CARD_DURATION;
      return false;
    }
  }
#if defined(P10_SMALL8_TEST) && P10_SMALL8_TEST
  dmd.setFont(TahomaP10_8_Trial);
#else
  dmd.setFont(config.digitStyle == 1 ? TahomaBold16Indic : TahomaBold16);
#endif
  if (!started)
  {
    pageStartedMs = now;
    started = true;
    safeCopyP(dispState.runText, FONT_TEST_PAGES[page], sizeof(dispState.runText));
    smartText.setShaping(true);
    smartText.setRightToLeft(false); // Same Arabic direction as normal cards.
#if defined(P10_SMALL8_TEST) && P10_SMALL8_TEST
    clearDisplay();
    char shaped[64];
    ArabicShaper::shape(dispState.runText, shaped, sizeof(shaped));
    dmd.setFont(config.digitStyle == 1 ? Tahoma7Indic : Tahoma7);
    drawDisplayText((DISPLAY_PROFILE.width - displayTextWidth(shaped)) / 2, 0, shaped);
    dmd.setFont(TahomaP10_8_Trial);
    smartText.setArea(8, 8);
#else
    smartText.setArea(0, DISPLAY_PROFILE.height);
#endif
    smartText.reset();
  }
  smartText.draw(dispState.runText,
#if defined(P10_SMALL8_TEST) && P10_SMALL8_TEST
                 8,
#else
                 0,
#endif
                 now);
  if (smartText.consumeDirty())
    markDisplayDirty();
  return true;
}
#endif

void recordLoopTelemetry(uint32_t startedUs, unsigned long now)
{
#if DMDESP_SCAN_DIAGNOSTICS
  finishScanStage(3);
#endif
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
#if DMDESP_SCAN_DIAGNOSTICS
  reportScanDiagnostics(now);
#endif
}

static void prepareNarrowSplash()
{
  safeCopyP(splashCompany, config.language == LANG_EN ? SPLASH_SHORT_EN : SPLASH_SHORT_AR,
            sizeof(splashCompany));
  safeCopy(splashPhone, "01030007330", sizeof(splashPhone));
  dmd.setFont(BasitFont7);
  splashTopText.reset();
  splashBottomText.reset();
  narrowSplashReady = true;
}

static void renderNarrowSplash(unsigned long now)
{
  if (!narrowSplashReady)
    prepareNarrowSplash();
  dmd.setFont(BasitFont7);
  splashTopText.setArea(0, 8);
  splashTopText.setSpeed(80);
  splashTopText.setRightToLeft(false);
  splashBottomText.setArea(8, 8);
  splashBottomText.setSpeed(80);
  splashBottomText.setRightToLeft(true);
  splashTopText.draw(splashCompany, 0, now);
  splashBottomText.draw(splashPhone, 8, now);
  if (splashTopText.consumeDirty() || splashBottomText.consumeDirty())
    markDisplayDirty();
}

void splashScreen()
{
  clearDisplay();
  dmd.setFont(BasitFont7);
  if (DISPLAY_PROFILE.narrow())
  {
    prepareNarrowSplash();
    renderNarrowSplash(millis());
  }
  else
  {
    char title[32];
    char shapedTitle[32];
    safeCopyP(title, config.language == LANG_EN ? SPLASH_TITLE_EN : SPLASH_TITLE,
              sizeof(title));
    ArabicShaper::shape(title, shapedTitle, sizeof(shapedTitle));
    drawDisplayText((DISPLAY_PROFILE.width - displayTextWidth(shapedTitle)) >> 1,
                    0, shapedTitle);
    drawDisplayText((DISPLAY_PROFILE.width - displayTextWidth("01030007330")) >> 1,
                    DISPLAY_PROFILE.smallTextY, "01030007330");
  }
  markDisplayDirty();
  applySettings();
}

#if DEBUG == 1
void adjustTime()
{
  DBGLN(F("adjustTime"));
}
#else
#define adjustTime()
#endif

void setup()
{
  if (DEBUG)
    Serial.begin(74880);

  DBGLN(F("\n\n=== Basit Masjed Clock Starting ==="));
  DBG(F("Reset: "));
  String bootResetReason = ESP.getResetReason();
  safeCopy(resetReason, bootResetReason.c_str(), sizeof(resetReason));
  DBGLN(resetReason);
  String bootResetInfo = ESP.getResetInfo();
  bootResetInfo.replace('\r', ' ');
  bootResetInfo.replace('\n', ' ');
  safeCopy(resetInfo, bootResetInfo.c_str(), sizeof(resetInfo));

  configStoreReady = ClockStorage::begin();
  if (!loadConfig())
  {
    config.setDefaults();
    const bool newStore = configStoreReady && !LittleFS.exists(CONFIG_STORE_A) &&
                          !LittleFS.exists(CONFIG_STORE_B) && !LittleFS.exists(CONFIG_STORE_TMP);
    if (!newStore || !resetConfig())
    {
      DBGLN(F("Storage unavailable or invalid; original files retained"));
    }
  }
  if (persistedSequence)
  {
    loadPrayerPrefs();
    loadPowerPrefs();
    loadCardPrefs();
  }
  else
    resetCardPrefsRuntime();
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
  clearDisplay();
  markDisplayDirty();

  splashScreen();
  presentDisplayFrame();
  splashStartedMs = millis();

  if (startRTC(true))
  {
    DBGLN(sysFlags.rtcTimeValid() ? F("RTC OK") : F("RTC connected; time invalid"));
    adjustTime();
    // startRTC() already took the first validated RTC sample.
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
#if DMDESP_SCAN_DIAGNOSTICS
  scanStageStartedUs = micros();
#endif
  unsigned long now = millis();
  serviceRTC(now);
  updateRtcBootIndicator(now);
  serviceWiFi(now);
  serviceFactoryReset(now);
  serviceDebugSerial();
#if DMDESP_SCAN_DIAGNOSTICS
  finishScanStage(0);
#endif
  yield();
#if DMDESP_SCAN_DIAGNOSTICS
  finishScanStage(1);
#endif
  server.handleClient();
#if DMDESP_SCAN_DIAGNOSTICS
  finishScanStage(2);
#endif
  now = millis(); // A request may have reset the RTC/software tick deadline.

  // Re-apply auto brightness when crossing into/out of night.
  static unsigned long lastBrightMs = 0;
  if (now - lastBrightMs >= 60000) {
    lastBrightMs = now;
    dmd.setBrightness(effectiveBrightness());
  }

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
    if (DISPLAY_PROFILE.narrow())
      renderNarrowSplash(now);
    if (splashStartedMs && now - splashStartedMs >= MS_SPLASH)
    {
      sysFlags.setSplash(false);
      syncRTC();
      clearDisplay();
      narrowSplashReady = false;
      applySettings(); // narrow splash uses BasitFont7; restore user font for cards
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
  const int16_t oldYear = timeData.year;
  const int8_t oldMonth = timeData.month, oldDay = timeData.day, oldHour = timeData.hour;
  const bool longStall = now - lastSecondMs >= 60000UL;
  const uint8_t secondTicks = consumeSecondTicks(now, lastSecondMs);
  if (secondTicks)
  {
    for (uint8_t tick = 0; tick < secondTicks; ++tick)
      tickSecond();
    // A fresh RTC sample is taken once, after all due software ticks.
    if (sysFlags.rtcOk() &&
        (now - lastRtcSyncMs >= MS_RTC_SYNC ||
                             oldHour != timeData.hour || longStall))
      syncRTC();
    const bool dateChanged = !sameCalendarDate(timeData.year, timeData.month,
                                               timeData.day, oldYear, oldMonth, oldDay);
    if (dateChanged)
    {
      calcHijri();
      calcPrayers();
    }
    const int16_t nowMins = timeData.hour * 60 + timeData.min;
    if (cachedNowMins != nowMins || dateChanged)
      updatePrayerWindow(nowMins);
    cachedNowMins = nowMins;
  }

#if defined(P10_FONT_VISUAL_TEST) && P10_FONT_VISUAL_TEST
  // Bench-only display; timed once, without suspending RTC/Wi-Fi service.
  if (renderFontVisualTest(now))
  {
    presentDisplayFrame();
    recordLoopTelemetry(loopStartedUs, now);
    return;
  }
#endif

  const bool timeInvalid = !sysFlags.rtcTimeValid();
  const bool displayError = !persistedSequence || !sysFlags.rtcOk() || timeInvalid;
  if (!displayError && narrowErrorFontForced)
  {
    applySettings();
    narrowErrorFontForced = false;
  }
  if (displayError)
  {
    dispState.setStopCards(true);
    smartText.setArea(0, DISPLAY_PROFILE.height);
    if (DISPLAY_PROFILE.narrow() && !narrowErrorFontForced)
    {
      // A 16px font cannot fit a useful status on a 1x1 panel.
      dmd.setFont(BasitFont7);
      smartText.reset();
      splashTopText.reset();
      splashBottomText.reset();
      narrowErrorFontForced = true;
    }
    const char *rtcError = nullptr;
    if (DISPLAY_PROFILE.narrow())
    {
      if (!persistedSequence)
        rtcError = STORAGE_ERROR_SHORT;
      else if (!sysFlags.rtcOk())
        rtcError = RTC_DISCONNECTED_SHORT;
      else if (config.language == LANG_EN)
        rtcError = ((now / 1800UL) & 1U) ? RTC_TIME_INVALID_SHORT_EN_2
                                        : RTC_TIME_INVALID_SHORT_EN_1;
      else
        rtcError = ((now / 1800UL) & 1U) ? RTC_TIME_INVALID_SHORT_AR_2
                                        : RTC_TIME_INVALID_SHORT_AR_1;
    }
    else
    {
      rtcError = !persistedSequence
                     ? (config.language == LANG_EN ? STORAGE_ERROR_TEXT_EN : STORAGE_ERROR_TEXT_AR)
                     : !sysFlags.rtcOk()
                     ? RTC_DISCONNECTED_TEXT
                     : (config.language == LANG_EN
                            ? RTC_TIME_INVALID_TEXT_EN
                            : RTC_TIME_INVALID_TEXT_AR);
    }
    const bool narrowTimeError = DISPLAY_PROFILE.narrow() &&
                                 persistedSequence && sysFlags.rtcOk() &&
                                 !sysFlags.rtcTimeValid();
    if (narrowTimeError)
    {
      safeCopyP(splashCompany,
                config.language == LANG_EN ? RTC_TIME_INVALID_SHORT_EN_1
                                            : RTC_TIME_INVALID_SHORT_AR_1,
                sizeof(splashCompany));
      safeCopyP(splashPhone,
                config.language == LANG_EN ? RTC_TIME_INVALID_SHORT_EN_2
                                            : RTC_TIME_INVALID_SHORT_AR_2,
                sizeof(splashPhone));
      splashTopText.setArea(0, 8);
      splashTopText.setRightToLeft(false);
      splashBottomText.setArea(8, 8);
      splashBottomText.setRightToLeft(false);
      splashTopText.draw(splashCompany, 0, now);
      splashBottomText.draw(splashPhone, 8, now);
      if (splashTopText.consumeDirty() || splashBottomText.consumeDirty())
        markDisplayDirty();
    }
    else
    {
      safeCopyP(dispState.runText, rtcError, sizeof(dispState.runText));
      if (DISPLAY_PROFILE.narrow())
        smartText.draw(dispState.runText, 0, now);
      else
        smartText.drawFlash(dispState.runText, dispParams.yPos, now);
      if (smartText.consumeDirty())
        markDisplayDirty();
    }
  }
  else if (!ultraPowerSaveActive())
    checkPrayers(now, timeData.hour * 60 + timeData.min);

  if (!updateUltraPowerSaveDisplay() && !dispState.stopCards())
    handleCards();
  presentDisplayFrame();
  recordLoopTelemetry(loopStartedUs, now);
}
