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
static const char SYAM_AYAM_WHITE_EN[] PROGMEM =
    "Reminder: White Days 13, 14 and 15 - fasting them is like fasting the whole month";
static const char NEW_HIJRI_YEAR_EN[] PROGMEM =
    "Prophet migration anniversary - Happy New Year";
static const char SYAM_ASHORA_EN[] PROGMEM =
    "Reminder: Fast Muharram 9 and 10 - Ashura Day";
static const char RAMADAN_HADITH_EN[] PROGMEM =
    "Whoever does not give up false speech and acting upon it, Allah has no need for his food and drink";
static const char EID_EL_FITR_EN[] PROGMEM =
    "Happy Eid al-Fitr - Best wishes";
static const char SYAM_YOUM_ARAFA_EN[] PROGMEM =
    "Reminder: Fast the Day of Arafah - Happy Eid";
static const char EID_AL_ADHAA_EN[] PROGMEM =
    "Blessed Eid al-Adha - Best wishes";

// ---- Azkar ----
static const char ZEKR_ALLAH[] PROGMEM =
    "لا تنس ذكر الله - سبحان الله وبحمده سبحان الله العظيم";
static const char ZEKR_MOHAMED[] PROGMEM =
    "لا تنس الصلاة على النبي - اللهم صل على سيدنا محمد وعلى آله وصحبه وسلم";
static const char AZKAR_AFTER[] PROGMEM = "لاتنس السنن";
static const char AZKAR_AFTER2[] PROGMEM = "أذكار الصلاة";
static const char ZEKR_ALLAH_EN[] PROGMEM =
    "Remember Allah - Glory and praise be to Allah, the Almighty";
static const char ZEKR_MOHAMED_EN[] PROGMEM =
    "Send blessings upon the Prophet - O Allah, bless our master Muhammad and his family and companions";
static const char AZKAR_AFTER_EN[] PROGMEM = "Do not forget the Sunnah prayers";
static const char AZKAR_AFTER2_EN[] PROGMEM = "Prayer supplications";

// ---- Splash / system ----
static const char SPLASH_TITLE[] PROGMEM = "بسيط للبرمجيات";
static const char RTC_DISCONNECTED_TEXT[] PROGMEM = "RTC E1";
static const char RTC_TIME_INVALID_TEXT_AR[] PROGMEM = "اضبط الوقت";
static const char RTC_TIME_INVALID_TEXT_EN[] PROGMEM = "TIME E2";
static const char SPLASH_TITLE_EN[] PROGMEM = "Basit Apps";
static const char CARD_RAMADAN_EN[] PROGMEM = "%d days until Ramadan";
static const char CARD_TEMP_EN[] PROGMEM = "Temperature %d degrees";
static const char OTA_SUCCESS_AR[] PROGMEM = "تم التحديث";
static const char OTA_SUCCESS_EN[] PROGMEM = "Update complete";
static const char OTA_PROGRESS_AR[] PROGMEM = "تحديث";
static const char OTA_PROGRESS_EN[] PROGMEM = "Update";
static const char OTA_SUCCESS_SHORT_AR[] PROGMEM = "تم";
static const char OTA_SUCCESS_SECOND_AR[] PROGMEM = "التحديث";

// ---- PrayerTimes indices: 4 Sunset (not displayed), 5 Maghrib ----
static const char PROGMEM PRAYER_NAME_AR[][16] = {
    "الفجر", "الشروق", "الظهر", "العصر", "", "المغرب", "العشاء"};
static const char PROGMEM PRAYER_NAME_EN[][10] = {
    "Fajr", "Sunrise", "Dhuhr", "Asr", "", "Maghrib", "Isha"};

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
// Compact forms used only by the fixed 64px top-info zone on 4x1 panels.
static const char PROGMEM HIJRI_MONTH_SHORT_AR[][24] = {
    "محرم", "صفر", "ربيع 1", "ربيع 2", "جمادى 1", "جمادى 2",
    "رجب", "شعبان", "رمضان", "شوال", "ذو القعدة", "ذو الحجة"};
static const char PROGMEM HIJRI_MONTH_EN[][10] = {
    "Muharram", "Safar", "RabiI", "RabiII", "JumadaI", "JumadaII", "Rajab",
    "Shaban", "Ramadan", "Shawwal", "DhuQadah", "DhuHijjah"};
static const char PROGMEM HIJRI_MONTH_SHORT_EN[][10] = {
    "Muh", "Saf", "Rab1", "Rab2", "Jum1", "Jum2", "Raj", "Sha", "Ram",
    "Shaw", "Qud", "Haj"};

// ---- Inline message fragments ----
static const char MSG_AZAN[] PROGMEM = "أذان %s";
static const char MSG_EQAMA_IN[] PROGMEM = "الإقامة بعد %d دقيقة";
static const char MSG_EQAMA_SEC[] PROGMEM = "الإقامة %02d";
static const char MSG_EQAMA[] PROGMEM = "الإقامة";
static const char MSG_JUMAA[] PROGMEM = "الجمعة";
static const char MSG_PHONE_SILENT_AR[] PROGMEM = "الهاتف صامت";
static const char MSG_PHONE_SILENT_EN[] PROGMEM = "Phone Silent";
static const char MSG_AZAN_EN[] PROGMEM = "Adhan %s";
static const char MSG_EQAMA_IN_EN[] PROGMEM = "Iqama in %d minutes";
static const char MSG_EQAMA_SEC_EN[] PROGMEM = "Iqama %02d";
static const char MSG_EQAMA_EN[] PROGMEM = "Iqama";
static const char MSG_JUMAA_EN[] PROGMEM = "Jumuah";

#endif // TEXTS_H
