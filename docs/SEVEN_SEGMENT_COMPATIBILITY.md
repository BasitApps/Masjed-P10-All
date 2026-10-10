# P10 / 7-segment compatibility matrix

## Current P10/M73 contract — 2026-10-09

Both use synchronous `ESP8266WebServer`, LittleFS persistence, compact root
POST, `/health` and `/selfTest`, and identical civil-time, prayer method/phase,
POST parsing and common-settings helpers. Storage bytes and display/audio
implementations remain device-specific, not interchangeable firmware images.

City IDs are nonnegative signed-32-bit values. P10 v6 settings migrate to record
version 7 after CRC validation without resetting them; M73's city layout remains
32-bit. Shared factory iqama is `{15,10,10,10,8,10}` and restart grace five seconds.
See `ROADMAP.md` for tests and pending device/publication gates. The 2026-10-07
sections below describe earlier batches, not current server/storage implementations.

## M71 compatibility gate — 2026-10-08

- Source found at `C:/Users/m_nag/Desktop/Arduino/Projects/Masjed-71/src/masjed-71.cpp`.
  It advertises `update,M,71,9,good` and implements legacy GET settings routes.
- Android still maps `M-71` to `updateM71.bin` version 9, but `MainViewModel`
  now sends configuration commands exclusively through compact POST. Thus this
  existing M71 image is **not compatible** with the current Android control API.
- M71 uses serial `shiftOut`/latch display hardware, not M73's TA6932 driver.
  Do not upload M73 firmware to M71. M71 needs its own compact-API firmware
  and a matching Android asset/version before claiming support.
- No M71 source, image, model mapping or connected hardware was modified here.
- **Deferred independently by owner 2026-10-09:**
  `superpowers/plans/2026-10-09-m71-api-upgrade.md`. Do not implement/build/upload
  M71 while finishing P10/M73; do not upload `updateM73.bin` to M71.

آخر مراجعة: 2026-10-07 (دفعة RTC/الوقت؛ نتائج2026-10-02 أدناه تاريخية)

المصدر المقارن: `C:\Users\m_nag\Desktop\Arduino\Projects\Masjed_TA_All_v2`.
هذه الوثيقة تسجل عقد التوافق المشترك؛ التعديلات على 7-segment تبقى في مستودعه
وبـ commits مستقلة.

| المجال | P10 الحالي | 7-segment الحالي | قرار التوافق |
|---|---|---|---|
| خادم HTTP | `ESP8266WebServer`، POST compact + `checkUpdate`/`updateMain` | `ESPAsyncWebServer`، POST compact + `checkUpdate`/`updateMain` | اختلاف المكتبة داخلي فقط؛ عقد التطبيق موحد. |
| ترميز النقل | جسم POST UTF-8 بسيط | جسم POST UTF-8 بسيط | لا توجد مسارات blur/CSV في المسار التجاري. |
| فهارس PrayerTimes | 0 Fajr، 1 Sunrise، 2 Dhuhr، 3 Asr، 4 Sunset، 5 Maghrib، 6 Isha؛ كلا firmware يستعمل قائمة الأحداث `{0,1,2,3,5,6}` | نفس العقد بعد تصحيح الحلقة | **R8-001 مغلق برمجيًا** في P10 وcommit 7‑segment `8996bab`; اختبار fixed-date/العتاد ما زال مطلوبًا. |
| طريقة مكة | نفس جدول الزوايا، و`makkahIsha=0` قاعدة الطريقة أو `1` بعد 90 دقيقة | نفس الخيار محفوظ في config | طريقة الحساب والإقامة موحدة؛ العرض وواجهة الإعداد فقط يختلفان. |
| الفجر/العشاء/المغرب | محددات صريحة في `PrayerMethodConfig` | setters مباشرة حسب method | لا نفترض تطابق القيم العشرية؛ المقارنة الرقمية المحلية ما زالت مطلوبة. |
| الإقامة | 0..120، والشروق محفوظ كحقل مستقل ويمكن تعطيله حسب العقد | حقول `eqamaFajr..eqamaEsha`، مع fallback قديم للشروق إلى 10 | لا نغيّر default أو معنى الصفر في 7-segment من مشروع P10. |
| نافذة الحدث | `prayerPhase()` موحد لقبل الأذان/الأذان/العد التنازلي/الإقامة/النوم/الأذكار، مع استثناء الجمعة الموثق؛ الإقامة 0 تعني تعطيل المرحلة | `Checking()` يستخدم نفس المصنف، وحساب الدقائق يعالج عبور منتصف الليل | نفس حدود الزمن؛ اختلاف العرض والصوت فقط. اختبار RTC والحدود على العتاد ما زال مطلوبًا. |
| DST/RTC | `daylightSavingTime` وRTC recovery داخل P10؛ الحساب لا يعدل RTC | `timezoneOffset` + `daylightSavingTime` مع نفس قاعدة التاريخ الموسمي | قاعدة الموسم موحدة: آخر جمعة أبريل إلى آخر خميس أكتوبر شاملًا؛ أسماء الحقول موحدة. اختبار RTC الفعلي ما زال مطلوبًا. |
| التاريخ الهجري | تحويل Gregorian→Hijri صحيح ثابت مع تصحيح ±3 وكاش بالتاريخ الكامل | نفس التحويل الثابت بدل التقريب القديم بالحلقات | نفس اليوم/الشهر/السنة؛ الاختلاف فقط في مواضع الخانات على الشاشة. |
| API البيانات | POST compact key/value يعيد `profile`, `lang`, `font`, `digitStyle`, `hour24` وغيرها | نفس POST compact key/value يعيد `profile=7seg` وحقول 7-segment | `v=2` داخل الجسم لا بادئة URL؛ التطبيق يرسل POST إلى `/getData` ونظائره. |
| تغيير كلمة مرور AP | POST `/setPassword` (alias داخلي لـ`setNetwork`) | POST `/setPassword` | اسم واحد في التطبيق. |
| استعادة ضبط المصنع | POST `/resetDefault` للفني؛ يحفظ ثم يرسل `resetDefaultOk` ويعيد التشغيل بعد `MS_RESTART_DELAY` | POST `/resetDefault` للفني؛ يتحقق من format/save/readback ثم يعيد التشغيل بعد 5 ثوانٍ | نفس اسم المسار والنتيجة؛ زر التطبيق لا يظهر إلا في وضع الفني. |
| حالات خطأ العرض | صحة RTC تُعرض من health/API | `E1`: `tm1[13]=E` و`tm1[12]=1` عند فشل RTC/I2C؛ `E2`: `tm1[13]=E` و`tm1[12]=2` مع أصفار وقت وامضة عند وقت غير صالح؛ `E4`: `tm3[7]=E` و`tm3[8]=4` عند غياب DS18B20 | أكواد تشخيصية محلية لا تغيّر أوقات الصلاة أو الإقامة. |
| التخزين | EEPROM versioned + sidecar محفوظ داخل gap بدون تغيير legacy layout | بنية `Configration` مختلفة | لا مشاركة bytes أو defaults بين المستودعين. |

## تحديث الاعتمادية — 2026-10-07

- كلاهما يميز اتصال RTC من صلاحية وقته، ويرفض OSF/القراءات الناقصة وBCD/التواريخ
  غير الصالحة، والنطاق2025..2099. في السفن تُعرض الشرطات وتبقى AP متاحة عند العطل.
- حفظ الوقت والتحكم12/24 في نفس الطلب، مع readback قبل النجاح والتحقق من استعادة
  العينة السابقة عند فشل التخزين. القفل عند حالة غير مؤكدة RAM فقط في هذه الدفعة.
- إزاحة الصلاة الموسمية مأخوذة من نفس تاريخ RTC الأساسي للساعة؛ حد آخر خميس أكتوبر
  الذي ينتج يومًا محليًا لاحقًا لم يعد يجعل الصلاة تختلف ساعة عن الساعة.
- الجمعة تحتفظ بالأذان وتلغي مراحل الإقامة في كلاهما. NaN لا يتحول لأرقام/أحداث؛
  المعادلة وجدول الطريقة وخيار مكة كما هي. في السفن تبديل التاريخ مرة لكل ثانية عينة.
- السفن:24/24 فحص مصدر و3/3 فحوص موجودة، ta73/ta73s SUCCESS؛ لا نشر صور أو زيادة
  إصدار أو رفع في هذه الدفعة. بقية API/الحفظ/OTA والمعاينة الميدانية ليست مغلقة.
- راجع `FIRMWARE_REVIEW_2026-10-07.md` للبصمات والحدود والخطوة التالية؛ لا تعتمد
  أرقام الإصدار القديمة في نتيجة2026-10-02 التالية على أنها حالة النشر الحالية.

دفعة API اللاحقة: معاملات Eqama/location/settings/password تُحفظ من مرشح مؤقت،
وقفل واحد يحمي مخازن Async المشتركة؛ المتزامن يرد `503 busy`. ta73 اختُبر OTA على
لوحة M73 المتصلة وعاد `UpdateOk` ثم `getData`; هذا ليس اعتمادًا نهائيًا ولا نشرًا
لـM73s أو Android. جسم التطبيق raw `application/octet-stream`، وليس `text/plain`.

## نتيجة المراجعة

- تم توحيد فهرس المغرب في P10 و7-segment، وتم توحيد جدول طريقة الحساب وخيار مكة؛
  كل الطلبات التجارية تستخدم الجسم المضغوط نفسه.
- تطابق الحساب العددي لتاريخ/موقع ثابت مثبت في `docs/PRAYER_REFERENCE_CASES.csv`؛ المتبقي هو جدول المسجد المحلي واختبار نافذة أذان/إقامة على العتاد.
- مصنف المرحلة أصبح مشتركًا في السلوك بين المسارين؛ لا يُعتبر نجاح build وحده دليلًا على صحة RTC أو حدود منتصف الليل.
- تمت إضافة العقد الموحد إلى 7-segment ورفع هدف الترقية للنسختين M73/M73s إلى 6؛ ملفات OTA
  الجديدة نُشرت إلى Android assets. التطبيق يفرض الترقية قبل الإعدادات، ولا يسجل
  أي firmware جديد مسارات GET القديمة.
