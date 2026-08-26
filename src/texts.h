#ifndef TEXTS_H
#define TEXTS_H

#include <Arduino.h>

static const char SYAM_AYAM_WHITE[] PROGMEM = "iJezWA JzmWA ^X 15 h 14 h 13 +kbWA ?aj}A ?aknB ?VJVIY";
static const char NEW_HIJRI_YEAR[] PROGMEM = "HjHeWA iJezWA ?arWA _bLayxB JkgB ?cYAh ?aR >V";
static const char SYAM_ASHORA[] PROGMEM = "?JfX ^X 10 h 9 iXhj 'AJhMaR ?aknB ?VJVIY";
static const char RAMADAN_HADITH[] PROGMEM = "_bYI ^X ?HuC aX _W JtS aBalcFAh aYaxjA JHuWA _wkW ?aU ^Xh - _bYI ^X ?HuC aX _W JtS aBalcFAh aYaxjA ^aoXJ ?aN ^X";
static const char EID_EL_FITR[] PROGMEM = "JkgB ?cYAh ?aR >V  -   =JabxWA JptWA HkR";
static const char SYAM_YOUM_ARAFA[] PROGMEM = "JkgB ?cYAh ?aR >V  -  _TJR ?hj ?aknB ?VJVIY";
static const char EID_AL_ADHAA[] PROGMEM = "JkgB ?cYAh ?aR >V  -  =JabxWA ifO}A HkR";
// لا تنس ذكر الله - سبحان الله وبحمده سبحان الله العظيم
static const char ZEKR_ALLAH[] PROGMEM = "?kqrWA _wWA ^afbL _HxfBh _wWA ^afbL - _wWA JVI (yC }";
// لا تنس الصلاة على النبى - اللهم صل على سيدنا محمد وعلى اله وصحبه وسلم
static const char ZEKR_MOHAMED[] PROGMEM = "?wLh _bfNh _WA iwRh HxfX aYHkL iwR >N ?zwWA - ibyWA iwR _}nWA (yC }";
// قال رسول الله من حسن اسلام المرء تركه مالا يعنيه
// static const char MN_HOSN_ISLAM[] PROGMEM = "_kyrj } aX _VJC 'JxWA ?}LA ^lF ^X  _wWA >hLJ >aU";

static const char PROGMEM dayNameGlobal[][10] = {"HF}A", "^kyD}A", "'aD}dWA", "'arBJ}A", "(kxgWA", "_rxeWA", "\"blWA"};
static const char PROGMEM arabicMonthName[][12] = {
    "?JfX", "JtN", ">hA /kBJ", "^aD /kBJ", ">hA HaxE", "^aD HaxE",
    "!EJ", "^abrM", "^aoXJ", ">AhM", "_HruWA hI", "_efWA hI"};

#endif // TEXTS_H