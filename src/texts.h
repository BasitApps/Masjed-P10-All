#ifndef TEXTS_H
#define TEXTS_H

#include <Arduino.h>

// All strings are plain Arabic UTF-8 in LOGICAL order (shaping happens at
// render time via ArabicShaper). English variants used when
// config.language == 1.
//
// NOTE (owner review): wording of event/azkar strings below is a best-effort
// migration of the legacy encoded strings - verify wording before release.

// ---- Events (Hijri calendar) ----
static const char SYAM_AYAM_WHITE[] PROGMEM =
    "تذكير: أيام البيض 13 و14 و15 - الصيام فيه مثل صيام الشهر كله";
static const char NEW_HIJRI_YEAR[] PROGMEM =
    "ذكرى هجرة الرسول صلى الله عليه وسلم - كل عام وأنتم بخير";
static const char SYAM_ASHORA[] PROGMEM =
    "تذكير بالصيام يوم 9 و10 من محرم - يوم عاشوراء";
static const char RAMADAN_HADITH[] PROGMEM =
    "من لم يدع قول الزور والعمل به فليس لله حاجة في أن يدع طعامه وشرابه";
static const char EID_EL_FITR[] PROGMEM =
    "عيد فطر سعيد - كل عام وأنتم بخير";
static const char SYAM_YOUM_ARAFA[] PROGMEM =
    "تذكير بصيام يوم عرفة - عيد سعيد";
static const char EID_AL_ADHAA[] PROGMEM =
    "عيد أضحى مبارك - كل عام وأنتم بخير";

// ---- Azkar ----
static const char ZEKR_ALLAH[] PROGMEM =
    "لا تنس ذكر الله - سبحان الله وبحمده سبحان الله العظيم";
static const char ZEKR_MOHAMED[] PROGMEM =
    "لا تنس الصلاة على النبي - اللهم صل على سيدنا محمد وعلى آله وصحبه وسلم";
static const char AZKAR_AFTER[] PROGMEM = "قوموا صلوا رحمكم الله";
static const char AZKAR_AFTER2[] PROGMEM = "أذكار الصلاة";

// ---- Splash / system ----
static const char SPLASH_TITLE[] PROGMEM = "برمجيات بسيط";
static const char RTC_FAIL_TEXT[] PROGMEM = "اضبط الساعة";

// ---- Prayer names (index: 0 Fajr, 1 Sunrise, 2 Dhuhr, 3 Asr, 4 Maghrib,
//      5 unused, 6 Isha) ----
static const char PROGMEM PRAYER_NAME_AR[][16] = {
    "الفجر", "الشروق", "الظهر", "العصر", "المغرب", "", "العشاء"};
static const char PROGMEM PRAYER_NAME_EN[][10] = {
    "Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "", "Isha"};

// ---- Day names ----
static const char PROGMEM DAY_NAME_AR[][20] = {
    "الأحد", "الاثنين", "الثلاثاء", "الأربعاء", "الخميس", "الجمعة",
    "السبت"};
static const char PROGMEM DAY_NAME_EN[][10] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
    "Saturday"};

// ---- Hijri month names ----
static const char PROGMEM HIJRI_MONTH_AR[][28] = {
    "محرم", "صفر", "ربيع الأول", "ربيع الثاني", "جمادى الأولى",
    "جمادى الثانية", "رجب", "شعبان", "رمضان", "شوال", "ذو القعدة",
    "ذو الحجة"};
static const char PROGMEM HIJRI_MONTH_EN[][10] = {
    "Muharram", "Safar", "Rabi1", "Rabi2", "Jumada1", "Jumada2", "Rajab",
    "Shaban", "Ramadan", "Shawwal", "Qudah", "Hajjah"};

// ---- Inline message fragments ----
static const char MSG_AZAN[] PROGMEM = "أذان %s";
static const char MSG_EQAMA_IN[] PROGMEM = "الإقامة بعد %d دقيقة";
static const char MSG_EQAMA_SEC[] PROGMEM = "الإقامة بعد %d ثانية";
static const char MSG_EQAMA[] PROGMEM = "الإقامة";
static const char MSG_JUMAA[] PROGMEM = "الجمعة";

#endif // TEXTS_H
