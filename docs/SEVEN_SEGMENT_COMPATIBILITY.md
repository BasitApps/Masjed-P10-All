# P10 / 7-segment compatibility matrix

آخر مراجعة: 2026-10-01

المصدر المقارن: `C:\Users\m_nag\Desktop\Arduino\Projects\Masjed_TA_All_v2`.
هذه الوثيقة قراءة توافقية فقط؛ لم يتم تعديل مستودع 7-segment.

| المجال | P10 الحالي | 7-segment الحالي | قرار التوافق |
|---|---|---|---|
| خادم HTTP | `ESP8266WebServer`، مسارات legacy GET وv2 POST | `ESPAsyncWebServer`، مسارات GET مع token | لا ننقل async إلى P10 استباقيًا؛ اختلاف معماري لا يثبت مشكلة في P10. |
| ترميز النقل | legacy CSV يمر عبر `blur/unBlur`، وv2 نص UTF-8 بسيط | نفس blur في الكود المقارن مع ردود CSV قديمة | يبقى CSV/blur كما هو للأجهزة المباعة؛ v2 هو مسار التطوير. |
| فهارس PrayerTimes | 0 Fajr، 1 Sunrise، 2 Dhuhr، 3 Asr، 4 Sunset، 5 Maghrib، 6 Isha | تم تصحيح الحلقة لتستبعد Sunset (`4`) وتستخدم Maghrib (`5`) للإقامة والعرض، والعشاء يبقى `6` | **R8-001 مغلق برمجيًا** في commit 7‑segment `8996bab`; ما زال اختبار fixed-date/العتاد مطلوبًا. |
| طريقة مكة | method rule متوافق + خيار firmware-only: fixed 90 minutes | `Makkah, 18.5, 17` فقط | لا نغير سلوك 7-segment تلقائيًا؛ خيار 90 دقيقة يبقى deferred للبرنامج/المستودع المقابل. |
| الفجر/العشاء/المغرب | محددات صريحة في `PrayerMethodConfig` | setters مباشرة حسب method | لا نفترض تطابق القيم العشرية؛ المقارنة الرقمية المحلية ما زالت مطلوبة. |
| الإقامة | 0..120، والشروق محفوظ كحقل مستقل ويمكن تعطيله حسب العقد | حقول `eqamaFajr..eqamaEsha`، مع fallback قديم للشروق إلى 10 | لا نغيّر default أو معنى الصفر في 7-segment من مشروع P10. |
| DST/RTC | `summerTime` وRTC recovery داخل P10؛ الحساب لا يعدل RTC | `dst` + `winterTime` مع تصحيح موسمي داخل `getPrayerTimes()` | نفس الاسم لا يعني نفس الإشارة أو التوقيت؛ الاختبار يجب أن يضبط RTC ثم يقارن الناتج. |
| API البيانات | v2 يعيد `profile`, `lang`, `font`, `digitStyle`, `hour24` وغيرها | `/getData` يعيد CSV أقدم ولا يصف خصائص P10 | التطبيق يختار المسار حسب `profile`؛ لا يرسل حقول P10 إلى 7-segment. |
| التخزين | EEPROM versioned + sidecar محفوظ داخل gap بدون تغيير legacy layout | بنية `Configration` مختلفة | لا مشاركة bytes أو defaults بين المستودعين. |

## نتيجة المراجعة

- لا يوجد تغيير كود مطلوب في P10 لهذه المقارنة.
- الخطر P1 الخاص بفهرس المغرب عولج في commit مستقل داخل مستودع 7-segment، وليس بتغيير صامت في P10.
- لا يُعتبر نجاح build أو تشابه أسماء الحقول دليلًا على تطابق الحساب؛ يلزم لاحقًا إدخال تاريخ/موقع ثابت ومقارنة الفهارس السبعة، ثم اختبار نافذة أذان/إقامة على العتاد.
