#include "SmartText.h"
#include "constants.h"
#include "globals.h"
#include "texts.h"
#include <Arduino.h>
#include <DMDESP.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PrayerTimes.h>
#include <RTClib.h>
#include <Wire.h>
#include <fonts/Janat16.h>
#include <fonts/Janat7.h>

ESP8266WebServer server(80);
RTC_DS3231 rtc;
DMDESP dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);
SmartText smartText(dmd, DISPLAY_WIDTH);

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
void unBlur(const char *text, char *out, size_t outSize)
{
  size_t len = 0;
  const char *p = text;
  while (*p && len + 1 < outSize)
  {
    int val = 0;
    while (*p >= '0' && *p <= '9')
      val = val * 10 + (*p++ - '0');
    out[len++] = (char)((val - 37 + 256) & 0xFF);
    if (*p == '-')
      ++p;
  }
  out[len] = '\0';
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

bool saveConfig()
{
  Configuration tmp;
  memcpy(&tmp, &config, sizeof(Configuration));
  xorConfig(tmp);
  EEPROM.put(0, tmp);
  return EEPROM.commit();
}
bool loadConfig()
{
  Configuration tmp;
  EEPROM.get(0, tmp);
  if (tmp.eepromVersion == 0xFFFF)
  {
    DBGLN(F("EEPROM empty"));
    return false;
  }
  xorConfig(tmp);
  if (tmp.eepromVersion != EEPROM_VERSION || !tmp.isValid())
  {
    DBGLN(F("EEPROM invalid"));
    return false;
  }
  memcpy(&config, &tmp, sizeof(Configuration));
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
}

int dayOfWeek(int y, int m, int d)
{
  static const int8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3)
    y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}
bool isDST(int y, int m, int d)
{
  if (config.summerTime == 0)
    return false;
  if (m > 4 && m < 10)
    return true;
  if (m == 4)
    return d >= 30 - ((dayOfWeek(y, 4, 30) - 5 + 7) % 7);
  if (m == 10)
    return d <= 31 - ((dayOfWeek(y, 10, 31) - 4 + 7) % 7);
  return false;
}
void calcHijri()
{
  if (hijriLastDay == timeData.day)
    return;
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
  if (prayerCache.valid && prayerCache.lastDay == timeData.day)
    return;

  set_high_lats_adjust_method(AngleBased);

  struct MCfg
  {
    int m;
    double f, i;
  };
  static const MCfg methods[] = {{Egypt, 19.5, 17.5},
                                 {Makkah, 18.5, 17.0},
                                 {Custom, 18.0, 17.5},
                                 {MWL, 18.0, 17.0},
                                 {Karachi, 18.0, 18.0}};

  if (config.calcMethod <= 4)
  {
    set_calc_method((CalculationMethod)methods[config.calcMethod].m);
    set_fajr_angle(methods[config.calcMethod].f);
    set_isha_angle(methods[config.calcMethod].i);
  }
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
  prayerCache.lastDay = timeData.day;
  prayerCache.valid = true;
  prayerCache.activeStart = prayerCache.activeEnd = -1; // Reset active window
}
void updatePrayerWindow(int16_t nowMins)
{
  refreshEqamaCache();

  prayerCache.activeStart = -1;
  prayerCache.activeEnd = -1;

  const int16_t sleepOffset = eqamaCache.sleepTime + 1;

  for (uint8_t j = 0; j < 6; ++j)
  {
    uint8_t i = PRAYER_INDICES[j];
    int16_t pt = prayerCache.times[i];

    if (pt < 0 || pt >= 1440)
      continue;

    int16_t eq = eqamaCache.times[i];
    int16_t start = pt - 3;
    int16_t end = pt + eq + sleepOffset;

    if (nowMins >= start && nowMins <= end)
    {
      prayerCache.activeStart = start;
      prayerCache.activeEnd = end;
      return;
    }
    if (nowMins < start && start - nowMins <= 5)
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
    timeData = {0, 1400, 0, 0, 0, 0, 0, 0, 1, 1};
    return;
  }

  DateTime now = rtc.now();
  bool dst = isDST(now.year(), now.month(), now.day());
  DateTime dt = dst ? now + TimeSpan(0, 1, 0, 0) : now;

  bool dayChanged = (dt.day() != timeData.day);
  timeData.year = dt.year();
  timeData.month = dt.month();
  timeData.day = dt.day();
  timeData.hour = dt.hour();
  timeData.min = dt.minute();
  timeData.sec = dt.second();
  timeData.dow = dt.dayOfTheWeek();

  if (dayChanged)
  {
    calcHijri();
    calcPrayers();
  }
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
  if (++timeData.hour > 23)
    timeData.hour = 0;
  syncRTC();
  lastSync = millis();
}

void nextCard()
{
  dispState.cardNum =
      (dispState.cardNum >= CARD_COUNT) ? 1 : (dispState.cardNum + 1);
  dispState.setFirstShow(true);
  dispState.cardTimer = millis() + MS_CARD_DURATION;
  smartText.reset();
}

void drawTopClock()
{
  static int8_t lastSec = -1;
  if (lastSec == timeData.sec)
    return;
  lastSec = timeData.sec;
  char *p = fmtBuf;
  *p++ = (timeData.hour >= 12) ? '?' : '*';
  *p++ = ' ';
  p += sprintf(p, "%d:", hour12(timeData.hour));
  int2str2(p, timeData.min);
  p += 2;
  *p++ = ':';
  int2str2(p, timeData.sec);
  p += 2;
  *p = '\0';
  if (timeData.sec == 0)
    dmd.drawFilledRect(0, 0, 63, 6, Bitmap::Black);
  dmd.drawText(4, -1, fmtBuf);
}
void drawTopInfo()
{
  static int8_t last5 = -1;
  static int16_t cachedWidth = -1;
  static char cachedText[32] = {0};
  int8_t sec5 = timeData.sec / 5;
  if (sec5 == last5)
    return;
  last5 = sec5;
  switch (sec5 & 3)
  {
  case 0:
    if (cachedDayIndex != timeData.dow)
    {
      safeCopyP(cachedDayName, dayNameGlobal[timeData.dow],
                sizeof(cachedDayName));
      cachedDayIndex = timeData.dow;
    }
    safeCopy(fmtBuf, cachedDayName, sizeof(fmtBuf));
    break;
  case 1:
    snprintf(fmtBuf, sizeof(fmtBuf), "%d-%d-%d", timeData.year, timeData.month,
             timeData.day);
    break;
  case 2:
  {
    int8_t monthIdx = timeData.hMonth - 1;
    if (cachedMonthIndex != monthIdx)
    {
      safeCopyP(cachedMonthName, arabicMonthName[monthIdx],
                sizeof(cachedMonthName));
      cachedMonthIndex = monthIdx;
    }
    snprintf(fmtBuf, sizeof(fmtBuf), "%s %d", cachedMonthName, timeData.hDay);
    break;
  }
  case 3:
    safeCopy(fmtBuf, "\"XaN ;CazWA", sizeof(fmtBuf));
    break;
  }
  if (strcmp(cachedText, fmtBuf) != 0)
  {
    safeCopy(cachedText, fmtBuf, sizeof(cachedText));
    cachedWidth = dmd.textWidth(fmtBuf);
    dmd.drawFilledRect(64, 0, DISPLAY_WIDTH - 1, 6, Bitmap::Black);
    dmd.drawText(DISPLAY_WIDTH - cachedWidth - 4, -1, fmtBuf);
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

  bool done = flash ? smartText.drawFlash(text, dispParams.yPos, now)
                    : smartText.draw(text, dispParams.yPos, now);

  if (autoNext && (done || (smartText.isStatic() && now > dispState.cardTimer)))
    nextCard();
}

void checkPrayers(unsigned long now, int16_t nowMins)
{
  if (prayerCache.activeStart < 0 || nowMins < prayerCache.activeStart ||
      nowMins > prayerCache.activeEnd)
  {
    dispState.setStopCards(false);
    return;
  }
  dispState.setStopCards(false);
  refreshEqamaCache();
  const bool friday = (timeData.dow == 5);
  const int8_t sleepTime = eqamaCache.sleepTime;
  for (uint8_t j = 0; j < 6; ++j)
  {
    uint8_t i = PRAYER_INDICES[j];
    const int16_t pt = prayerCache.times[i];
    if (pt < 0 || pt >= 1440)
      continue;
    const int8_t eq = eqamaCache.times[i];
    const int16_t delta = pt - nowMins, since = nowMins - pt;
    if ((!friday && i != 2) && (delta > 5 || since > 30)) 
      continue;
    const bool skipEq = (i == 1) || (i == 2 && friday);
    // 1. Pre-Prayer (3 mins before: delta 3, 2, 1)
    if (delta > 0 && delta <= 3)
    {
      dispState.setStopCards(true);
      uint8_t h, m;
      minToHM(pt, h, m);
      snprintf(dispState.runText, sizeof(dispState.runText), "%d:%02d  %s",
               hour12(h), m, TimeName[i]);
      snprintf(dispState.tempBuf, sizeof(dispState.tempBuf), "%d : %02d : %02d",
               hour12(timeData.hour), timeData.min, timeData.sec);
      displayText(dispState.runText, dispState.tempBuf, false, true, now);
      return;
    }
    // 2. Azan (Now + 1 min: since 0, 1)
    if (since >= 0 && since <= 1)
    {
      dispState.setStopCards(true);
      if (i == 1)
      {
        displayText(TimeName[i], nullptr, true, true, now);
      }
      else
      {
        snprintf(dispState.runText, sizeof(dispState.runText), "%s ^AIA",
                 (i == 2 && friday) ? "_rxeWA" : TimeName[i]);
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
    if (skipEq)
      continue;
    if (since >= 2 && since < eq - 1)
    {
      dispState.setStopCards(true);
      snprintf(dispState.runText, sizeof(dispState.runText),
               dispParams.isSmall ? "_ukUH %d HrB _XaU}A" : "H %d HrB _XaU}A",
               (pt + eq) - nowMins);
      displayText(dispState.runText, "\"XaN ;CazWA @", false, true, now);
      return;
    }
    if (since == eq - 1)
    {
      dispState.setStopCards(true);
      snprintf(dispState.runText, sizeof(dispState.runText),
               dispParams.isSmall ? "_kYaD %d HrB _XaU}A" : "%d HrB _XaU}A",
               60 - timeData.sec);
      displayText(dispState.runText, nullptr, false, true, now);
      return;
    }
    if (since == eq)
    {
      dispState.setStopCards(true);
      displayText("_XaU}A", nullptr, true, true, now);
      return;
    }
    if (since > eq && since < eq + sleepTime)
    {
      dispState.setStopCards(true);
      sleeping();
      return;
    }
    if (since >= eq + sleepTime && since < eq + 1 + sleepTime)
    {
      dispState.setStopCards(true);
      snprintf(dispState.runText, sizeof(dispState.runText),
               dispParams.isSmall ? "_}nWA JaVIAh ^ylWA (yC}" : "_}nWA JaVIAh");
      displayText(dispState.runText, "^ylWA (yC}", false, true, now);
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

void cardZekr(PGM_P content)
{
  char buf[150];
  safeCopyP(buf, content, sizeof(buf));
  drawCard(buf);
}
void cardMasjedName() { drawCard(config.masjedName, true); }
void cardMessage() { drawCard(config.message, true); }
void cardBigFontOnly(uint8_t type)
{
  if (dispParams.isSmall)
  {
    nextCard();
    return;
  }
  switch (type)
  {
  case 0:
    if (cachedDayIndex != timeData.dow)
    {
      safeCopyP(cachedDayName, dayNameGlobal[timeData.dow],
                sizeof(cachedDayName));
      cachedDayIndex = timeData.dow;
    }
    safeCopy(fmtBuf, cachedDayName, sizeof(fmtBuf));
    break;
  case 1:
    snprintf(fmtBuf, sizeof(fmtBuf), "%d-%d-%d", timeData.year, timeData.month,
             timeData.day);
    break;
  case 2:
    int8_t monthIdx = timeData.hMonth - 1;
    if (cachedMonthIndex != monthIdx)
    {
      safeCopyP(cachedMonthName, arabicMonthName[monthIdx],
                sizeof(cachedMonthName));
      cachedMonthIndex = monthIdx;
    }
    snprintf(fmtBuf, sizeof(fmtBuf), "%d %s %d", timeData.hYear,
             cachedMonthName, timeData.hDay);
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
  snprintf(fmtBuf, sizeof(fmtBuf), "?hj %d HrB ^aoXJ", days);
  drawCard(fmtBuf);
}
void cardEvent()
{
  PGM_P event = nullptr;
  const int8_t d = timeData.hDay, m = timeData.hMonth;
  if (d >= 13 && d <= 15 && m != 9)
    event = SYAM_AYAM_WHITE;
  else if ((m == 12 && d >= 29) || (m == 1 && d == 1))
    event = NEW_HIJRI_YEAR;
  else if (m == 1 && d >= 8 && d <= 10)
    event = SYAM_ASHORA;
  else if (m == 9)
    event = RAMADAN_HADITH;
  else if (m == 10 && d == 1)
    event = EID_EL_FITR;
  else if (m == 12 && d >= 7 && d <= 9)
    event = SYAM_YOUM_ARAFA;
  else if (m == 12 && d >= 10 && d <= 13)
    event = EID_AL_ADHAA;
  if (!event)
  {
    nextCard();
    return;
  }
  char buf[150];
  safeCopyP(buf, event, sizeof(buf));
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
    snprintf(fmtBuf, sizeof(fmtBuf), "%d  _JAJfWA _EJH", cachedTemp);
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
  *p++ = (timeData.hour >= 12) ? '?' : '*';
  *p++ = ' ';
  uint8_t h12 = hour12(timeData.hour);
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
    for (int8_t i = 6; i >= 0; --i)
    {
      if (i == 5)
        continue;
      uint8_t h, m;
      minToHM(prayerCache.times[i], h, m);
      p += sprintf(p, "%d:", hour12(h));
      int2str2(p, m);
      p += 2;
      *p++ = ' ';
      // Copy prayer name
      const char *name = TimeName[i];
      while (*name)
        *p++ = *name++;
      *p++ = ' ';
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
void cardZekrAllah() { cardZekr(ZEKR_ALLAH); }
void cardZekrMohamed() { cardZekr(ZEKR_MOHAMED); }
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
    dispState.cardNum = 1;
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
  WiFi.softAP(ssid, AP_PASSWORD);
}

bool getArg(const char *arg, char *buf, size_t len)
{
  if (!server.hasArg(arg))
    return false;
  unBlur(server.arg(arg).c_str(), buf, len);
  return buf[0] != '\0';
}
int getArgInt(const char *arg, int def = 0)
{
  char buf[16];
  return getArg(arg, buf, sizeof(buf)) ? atoi(buf) : def;
}
float getArgFloat(const char *arg, float def = 0)
{
  char buf[16];
  return getArg(arg, buf, sizeof(buf)) ? atof(buf) : def;
}
void respond(const char *msg)
{
  char buf[128];
  blur(msg, buf, sizeof(buf));
  server.send(200, "text/plain", buf);
}
void saveRespond(const char *ok)
{
  respond(saveConfig() ? ok : "Error");
  invalidateCaches(prayerCache, hijriLastDay);
  calcPrayers();
  int16_t cachedNowMins = timeData.hour * 60 + timeData.min;
  updatePrayerWindow(cachedNowMins);
  dispState.setFirstShow(true);
  dmd.clear();
}
uint8_t effectiveBrightness();
void applySettings()
{
  dispParams.isSmall = (config.isSmallFont == 0);
  dispParams.yPos = dispParams.isSmall ? SMALL_FONT_Y_POS : 0;
  dispParams.areaY = dispParams.isSmall ? 8 : 0;
  dispParams.areaH = dispParams.isSmall ? 8 : DISPLAY_HEIGHT;

  dmd.setFont(dispParams.isSmall ? Janat7 : Janat16);
  dmd.setBrightness(effectiveBrightness());
  static const uint16_t speeds[] = {70, 60, 50, 40, 30, 20};
  dispState.scrollSpeed =
      (config.scrollingSpeed <= 5) ? speeds[config.scrollingSpeed] : 40;
}

// Auto power-save: dim to half brightness after Isha + powerSaveAfterIsha min,
// exit before Fajr - powerSaveBeforeFajr min.
uint8_t effectiveBrightness()
{
  DateTime n = rtc.now();
  int nowMin = n.hour() * 60 + n.minute();
  int isha = prayerCache.times[6]; // Isha (minutes)
  int fajr = prayerCache.times[0]; // Fajr (minutes)
  bool powerSave = (nowMin >= isha + config.powerSaveAfterIsha) ||
                   (nowMin < fajr - config.powerSaveBeforeFajr);
  return powerSave ? (config.brightness / 2) : config.brightness;
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
void handleGetData()
{
  char buf[BUF_LARGE], out[768];
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
void handleSetDateTime()
{
  DateTime dt(getArgInt("y", 2025), getArgInt("m", 1), getArgInt("d", 1),
              getArgInt("h"), getArgInt("mi"), getArgInt("s"));
  config.hijriDayCorrection = getArgInt("hjc", config.hijriDayCorrection);
  config.summerTime = getArgInt("st", config.summerTime);
  config.dst = getArgInt("dst", config.dst);
  if (config.summerTime == 1 && isDST(dt.year(), dt.month(), dt.day()))
    dt = dt - TimeSpan(0, 1, 0, 0);
  rtc.adjust(dt);
  saveRespond("dateTimeOk");
  syncRTC();
  calcHijri();
}
void handleSetEqama()
{
  config.eqamaFajr = getArgInt("f", config.eqamaFajr);
  config.eqamaShrouq = getArgInt("s", config.eqamaShrouq);
  config.eqamaZohr = getArgInt("z", config.eqamaZohr);
  config.eqamaAsr = getArgInt("a", config.eqamaAsr);
  config.eqamaMagrb = getArgInt("m", config.eqamaMagrb);
  config.eqamaEsha = getArgInt("e", config.eqamaEsha);
  saveRespond("eqamaOk");
}
void handleSetLocation()
{
  config.country = getArgInt("c", config.country);
  config.government = getArgInt("g", config.government);
  config.city = getArgInt("ct", config.city);
  config.latitude = getArgFloat("lat", config.latitude);
  config.longitude = getArgFloat("lon", config.longitude);
  saveRespond("locationOk");
}
void handleSetSettings()
{
  config.calcMethod = getArgInt("cm", config.calcMethod);
  config.wayMethod = getArgInt("wm", config.wayMethod);
  config.sleepPrayerTime = getArgInt("spt", config.sleepPrayerTime);
  config.sleepGomaaTime = getArgInt("sgt", config.sleepGomaaTime);
  config.brightness = getArgInt("bri", config.brightness);
  config.powerSaveAfterIsha = getArgInt("psai", config.powerSaveAfterIsha);
  config.powerSaveBeforeFajr = getArgInt("psbf", config.powerSaveBeforeFajr);
  applySettings();
  saveRespond("settingsOk");
}
void handleSetMessage()
{
  config.scrollingSpeed = getArgInt("tS", config.scrollingSpeed);
  config.isSmallFont = getArgInt("fS", config.isSmallFont);
  getArg("sms", config.message, sizeof(config.message));
  getArg("mN", config.masjedName, sizeof(config.masjedName));
  applySettings();
  saveRespond("messageOk");
}
void handleOTAComplete()
{
  server.send(200, "text/plain",
              Update.hasError() ? "UpdateError" : "UpdateOk");
  if (!Update.hasError())
  {
    if (!sysFlags.restartNow())
    {
      sysFlags.setRestart(true);
      sysFlags.restartAt = millis() + MS_RESTART_DELAY;
    }
  }
}
void handleOTAUpload()
{
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    DBGLN(F("OTA Start"));
    // dispState.setStopCards(true);
    sysFlags.setUpdate(true);
    Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      Update.printError(Serial);
    ESP.wdtFeed();
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (Update.end(true))
    {
      DBGLN(F("OTA Success"));
      // dispState.setStopCards(true);
      // dmd.clear();
      // dmd.setFont(Janat7);
      // dmd.drawText((DISPLAY_WIDTH - dmd.textWidth("#jHfcWA ?C")) >> 1, 4,
      //              "#jHfcWA ?C");
    }
    else
    {
      DBGLN(F("OTA Failed"));
      Update.printError(Serial);
    }
    // sysFlags.setUpdate(false);
  }
}

void initServer()
{
  server.onNotFound(handleNotFound);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/checkUpdate", HTTP_GET, handleCheckUpdate);
  server.on("/testMode", HTTP_GET, handleTestMode);
  server.on("/getData", HTTP_GET, handleGetData);
  server.on("/setDateTime", HTTP_GET, handleSetDateTime);
  server.on("/setEqama", HTTP_GET, handleSetEqama);
  server.on("/setLocation", HTTP_GET, handleSetLocation);
  server.on("/setSettings", HTTP_GET, handleSetSettings);
  server.on("/setMessage", HTTP_GET, handleSetMessage);
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
    return;
  }
  dmd.setBrightness(TEST_BRIGHTNESS);
  dmd.drawFilledRect(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1,
                     Bitmap::White);
}
void splashScreen()
{
  dmd.clear();
  dmd.setFont(Janat7);
  dmd.drawText((DISPLAY_WIDTH - dmd.textWidth("\"akeXJbwW PklB")) >> 1, -1,
               "\"akeXJbwW PklB");
  dmd.drawText((DISPLAY_WIDTH - dmd.textWidth("01030007330")) >> 1, 8,
               "01030007330");
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
  rtc.adjust(DateTime(2022, 2, 1, 23, 59, 50)); // Midnight
}
#else
#define adjustTime()
#endif

void setup()
{
  if (DEBUG)
    Serial.begin(115200);

  DBGLN(F("\n\n=== Basit Remote Starting ==="));
  DBG(F("Reset: "));
  DBGLN(ESP.getResetReason());

  EEPROM.begin(512);
  if (!loadConfig())
  {
    DBGLN(F("No config, using defaults"));
    resetConfig();
  }

  initAP();
  initServer();

  Wire.begin();
  Wire.setClock(400000); // Fast I2C 400kHz

  dmd.start();
  dmd.clear();

  // sysFlags.setSplash(true);
  // sysFlags.setSplash(false);
  splashScreen();

  if (rtc.begin())
  {
    DBGLN(F("RTC OK"));
    sysFlags.setRtc(true);
    rtc.disable32K();
    rtc.writeSqwPinMode(DS3231_OFF);
    adjustTime();
    syncRTC();
  }
  else
  {
    DBGLN(F("RTC FAIL"));
    sysFlags.setRtc(false);
  }

  updatePrayerWindow(timeData.hour * 60 + timeData.min);

  ESP.wdtEnable(WDTO_8S);
  DBGLN(F("Setup done"));
}

void loop()
{
  dmd.loop();
  const unsigned long now = millis();

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

  if (sysFlags.restartNow() && now >= sysFlags.restartAt)
  {
    ESP.restart();
  }

  if (sysFlags.splashScreen())
  {
    if (now >= MS_SPLASH)
    {
      sysFlags.setSplash(false);
      syncRTC();
      dmd.clear();
    }
    return;
  }

  if (sysFlags.testMode())
  {
    testMode();
    return;
  }

  // Second tick - update time and prayer window
  static int16_t cachedNowMins = -1;
  if (now - lastSecondMs >= MS_SECOND)
  {
    lastSecondMs = now;
    tickSecond();
    cachedNowMins = timeData.hour * 60 + timeData.min;

    // Update prayer window once per minute
    if (timeData.sec == 0)
      updatePrayerWindow(cachedNowMins);

    if (timeData.year < MIN_VALID_YEAR)
    {
      dispState.setStopCards(true);
      smartText.setArea(0, DISPLAY_HEIGHT);
      smartText.drawFlash("\"UhWA PbOA", dispParams.yPos, now);
    }
    else
    {
      checkPrayers(now, cachedNowMins);
    }
  }

  if (!dispState.stopCards())
    handleCards();

  // if (!wifiOff) {
  yield(); // Prevent blocking, allow other tasks to run
  server.handleClient();
  // }
}
