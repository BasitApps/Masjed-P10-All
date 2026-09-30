# Basit Mosque Clock — Commercial V2 Handoff Plan

> وثيقة التسليم الأساسية لأي Agent يكمل المشروع. اقرأ أيضاً `ROADMAP.md`،
> `DECISIONS.md`، `FONT_SPEC.md`، و`RELIABILITY_PROGRESS.md` قبل التعديل.

> **قرار النقل الحالي (2026-09-05):** واجهة العميل تعتمد كلمة مرور AP كحد
> الوصول الوحيد. تصميم HMAC/nonce أدناه تاريخي؛ تحقق توقيع OTA ما زال فعالاً.

## 1. القرارات المعتمدة

- لا JSON ولا ArduinoJson؛ البروتوكول نصي صغير ومحدد الإصدار.
- API v2 يستخدم POST نصياً بسيطاً، وكلمة مرور AP هي حد الوصول الوحيد؛ HMAC
  التاريخي لم يعد جزءاً من Firmware أو Android الحالي.
- `blur()` يبقى فقط أثناء الترحيل ثم يُحذف بعد إصدار Android المتوافق.
- مقاسات المنتج: P10 ‏`1x1`، `2x1`، `3x1`، `4x1` فقط؛ لا `4x2`.
- مصدر واحد مع Build Profile مستقل لكل مقاس، وليس اكتشافاً وقت التشغيل.
- Wi-Fi AP له كلمة مرور افتراضية قابلة للتغيير من التطبيق.
- كلمة واحدة فقط للعميل: كلمة مرور Wi-Fi/AP.
- Diagnostics الكاملة للاختبار/الصيانة؛ واجهة العميل تعرض حالة مبسطة.
- Codex ينفذ الاختبارات القصيرة، والمالك ينفذ 24 ساعة ثم 7 أيام.
- ESP32 خارج النطاق الحالي.

## 2. صيغة البيانات بدون JSON

ArduinoJson يمكن ضبطه بذاكرة ثابتة، لكنه غير مطلوب لأن حقول الساعة معروفة.
الصيغة المعتمدة أسطر UTF-8 مرتبة لكل Endpoint:

```text
y=2026
m=9
d=2
h=1
mi=38
message=السلام عليكم
```

- Body من نوع `text/plain; charset=utf-8`.
- نقسم السطر عند أول `=`، ونُهرب `\\` وnewline فقط (`\\\\` و`\\n`).
- الرد يستخدم الصيغة نفسها ويبدأ بـ`v=2`.
- لا Base64 للنص العربي؛ يبقى UTF-8 بحجمه الطبيعي.

## 3. نموذج كلمة السر الأبسط

لا QR ولا Pairing Code ولا كلمة إدارة ثانية:

1. الجهاز يخرج من المصنع بكلمة Wi-Fi افتراضية وحالة
   `mustChangePassword=true`.
2. التطبيق يتصل بالكلمة الافتراضية، ولا يسمح بباقي الإعدادات قبل اختيار كلمة
   جديدة (8–63 حرفاً، ويوصى بعشرة فأكثر).
3. الجهاز والتطبيق يشتقان مفتاح الطلبات:

```text
Kdevice = PBKDF2-HMAC-SHA256(wifiPassword,
                             deviceId || "Basit-v2",
                             calibratedIterations, 32 bytes)
```

4. الجهاز يحسب `Kdevice` فقط عند الإقلاع/تغيير الكلمة ويستخدم HMAC السريع لكل
   Request. التطبيق يحفظ المفتاح المشتق في Android Keystore.
5. عند تغيير الكلمة لاحقاً، يتغير Wi-Fi وHMAC معاً وتُلغى الهواتف التي ما زالت
   تحمل الكلمة القديمة.

الكلمة الافتراضية معروفة ولذلك الحماية قبل التغيير محدودة عمداً؛ إجبار التغيير
أول مرة هو شرط الأمان. هذا التبسيط يعني أيضاً أن من يعرف كلمة Wi-Fi يستطيع
إدارة الساعة، لذلك لا تُشارك كشبكة عامة.

ESP8266 لا يملك Secure Element؛ مهاجم يملك اللوحة وقدرة قراءة Flash قد يستخرج
السر. الحماية المستهدفة هي مستخدم الشبكة أو تطبيق غير مصرح، لا هجوم مختبر
فيزيائي. تغيير هذا الحد مستقبلاً يحتاج عتاداً مختلفاً.

## 4. تعامل Android مع المفتاح

- أول اتصال: التطبيق يستخدم الكلمة الافتراضية ثم يفرض إدخال كلمة جديدة.
- يشتق `Kdevice` ويستورده إلى Android Keystore باسم
  `basit_hmac_<deviceId>`؛ لا يسجل كلمة السر أو المفتاح في Logs.
- هاتف جديد: المستخدم يدخل كلمة Wi-Fi الحالية التي يحتاجها أصلاً للاتصال؛
  التطبيق يشتق المفتاح نفسه ويتحقق بطلب HMAC.
- تغيير الكلمة: التطبيق يرسل أمراً مصرحاً، يحفظ المفتاح الجديد، ينتظر رد
  النجاح، ثم يعيد الاتصال بالـAP بعد إعادة تشغيله.
- نسيان الكلمة: ضغط زر Hardware Reset الغاطس 10 ثوانٍ يعيد إعدادات المصنع
  والكلمة الافتراضية، ثم يفرض التطبيق تغييرها من جديد.

زر Reset يقرأ بـ`INPUT_PULLUP` مع debounce ومدة ضغط واضحة، ويُختار GPIO بعد
مراجعة تعارضات P10/boot pins. الضغط القصير لا يمسح شيئاً.

## 5. API v2 وHMAC

### Challenge

```text
GET /v2/challenge?id=<deviceId>
```

يرد الجهاز بـnonce عشوائي 16 بايت Base64url صالح 30 ثانية. يحتفظ بعدد صغير
ثابت من nonces في RAM؛ كل nonce يستخدم مرة ثم يحذف. لا EEPROM، والمصدر
`os_random()`.

### الطلب المحمي

```text
POST /v2/setDateTime
Content-Type: text/plain; charset=utf-8
X-Basit-Id: <deviceId>
X-Basit-Nonce: <base64url nonce>
X-Basit-Mac: <base64url 16-byte tag>
```

النص الموقع بالضبط:

```text
BASIT2\nPOST\n<path>\n<deviceId>\n<nonce>\n<body-bytes>
```

- HMAC-SHA256 من ESP8266/BearSSL الموجود.
- نرسل أول 16 بايت من HMAC: أمان 128-bit و22 حرف Base64url.
- مقارنة constant-time.
- MAC يشمل المسار والـbody والـnonce لمنع التعديل وreplay.
- الرد يوقع: `BASIT2\nRESP\n<nonce>\n<status>\n<body>`.

أكواد v2: ‏`200` نجاح، `400` تنسيق، `401` HMAC، `409` nonce، `422` نطاق،
و`500/503` عطل. لا نعيد `200 + Error` في v2.

Endpoints:

- `GET /v2/challenge`
- `POST /v2/getData`
- `POST /v2/setDateTime`
- `POST /v2/setEqama`
- `POST /v2/setLocation`
- `POST /v2/setSettings`
- `POST /v2/setMessage`
- `POST /v2/setNetwork`
- `POST /v2/setFont`
- `POST /v2/update`
- `POST /v2/health` (محمي للصيانة)

## 6. الترحيل من v1 وblur

1. تجميد v1 وعدم إضافة خصائص جديدة إليه.
2. إضافة v2 بجوار v1 بلا تغيير EEPROM في أول بناء تجريبي.
3. Android يحاول v2 أولاً ويرجع إلى v1 للأجهزة القديمة فقط.
4. اختبار الأوامر وUnicode وHMAC وreplay وانقطاع الشبكة.
5. إصدار Android بالتزامن مع Firmware.
6. بعد إثبات الترقية: تعطيل ثم حذف v1 و`blur()/unBlur()` لاستعادة Flash.
7. رفع Major version وEEPROM version فقط عند تنسيق الإصدار النهائي.

## 7. Build Profiles لمقاسات P10

```text
p10_1x1: DISPLAYS_WIDE=1, DISPLAYS_HIGH=1
p10_2x1: DISPLAYS_WIDE=2, DISPLAYS_HIGH=1
p10_3x1: DISPLAYS_WIDE=3, DISPLAYS_HIGH=1
p10_4x1: DISPLAYS_WIDE=4, DISPLAYS_HIGH=1
```

- اسم Firmware يتضمن المقاس والموديل والإصدار.
- لا نخزن المقاس في EEPROM ولا نعيد إنشاء DMDESP وقت التشغيل.
- كل Layout يستخدم `DISPLAY_WIDTH` دون `128` ثابتة.
- الارتفاع ثابت 16، فلا نحتاج Layout رأسياً جديداً.
- CI يبني Profiles الأربعة ويمنع الإصدار إذا فشل أحدها.

## 8. OTA التجاري

المرحلة الأولى:

- Manifest: model/profile/version/size/SHA-256/signature.
- توقيع Vendor غير متماثل ECDSA P-256 عبر BearSSL؛ المفتاح الخاص خارج المشروع
  والمفتاح العام فقط داخل Firmware.
- رفض مقاس مختلف أو Hash/توقيع خاطئ أو downgrade إلا بوضع صيانة فعلي.
- OTA غير حاجب ويغذي WDT ولا يكتب Config.

الـESP8266 Arduino OTA لا يقدم rollback تلقائياً آمناً مثل ESP32. لذلك ننفذ
أولاً Signed OTA + تحقق كامل + Serial recovery. بعد ذلك فقط نقيم Dual-image أو
custom bootloader في Flash ‏4MB مع اختبارات انقطاع الطاقة، ولا نَعِد بالميزة
قبل إثباتها.

## 9. Wi-Fi والإعدادات

- كلمة افتراضية موحدة وبسيطة للتجهيز، مع تغيير إجباري أول مرة.
- تغييرها من التطبيق مع validation ‏8–63 حرفاً (الموصى به 10+).
- حفظ فقط عند تغير القيمة لحماية Flash.
- الرد قبل إعادة تشغيل AP ثم يعيد Android الاتصال.
- زر Factory Reset لمدة 10 ثوانٍ يمسح Config ويعيد الكلمة الافتراضية وراية
  `mustChangePassword`؛ اختلاف deviceId يجعل المفتاح المشتق مختلفاً بين
  الأجهزة، لكن الكلمة الافتراضية تظل معروفة حتى يغيرها العميل.

## 10. Diagnostics في المنتج

بدون كتابة دورية على Flash:

- firmware version/profile، reset reason، uptime.
- free heap/fragmentation الحاليان، RTC/AP status.
- عداد abnormal boots يكتب فقط عند reset غير طبيعي.
- `/v2/health` محمي ومتاح لتطبيق الصيانة؛ العميل يرى «سليم/يحتاج صيانة».
- `DEBUG=0` في Release ولا أسرار أو Requests في Serial logs.

هذه البيانات مفيدة للدعم بعد البيع حتى إن لم تظهر للعميل.

## 11. الاختبارات والمسؤوليات

Codex — قصيرة وآلية:

- HMAC test vectors متطابقة بين C++ وAndroid.
- parser وUTF-8 وescaping وrange validation.
- MAC خاطئ/ناقص، nonce مكرر/منتهي، body معدل.
- Build Profiles الأربعة وقياس RAM/Flash.
- ترحيل v1/v2 وتغيير Wi-Fi وإعادة الاتصال.
- زر Reset: ضغط قصير، 10 ثوانٍ، debounce، ومسح/إقلاع سليم.
- OTA صحيح وخاطئ وإلغاء upload.
- Reset/boot/RTC checks قصيرة عند توصيل العتاد.

المالك — طويلة:

- Soak ‏24 ساعة ثم 7 أيام على Release Candidate.
- مراقبة الشاشة وRTC والصلاة وWi-Fi والحرارة ومصدر الطاقة الحقيقي.
- أي Restart أو freeze أو فساد عرض يلغي النتيجة ويعيد الاختبار من الصفر.

## 12. ترتيب التنفيذ

1. اعتماد الخطوط الثلاثة على P10.
2. Build Profiles الأربعة وإزالة أرقام العرض الثابتة.
3. HMAC/nonce/parser مع Native test vectors.
4. v2 بجوار v1 وتجربته بأداة كمبيوتر صغيرة.
5. Android first-password/Keystore/v2/fallback v1.
6. اختبار Firmware + Android ميدانياً.
7. تغيير Wi-Fi الآمن وService health.
8. Signed OTA والتحقق من profile/version.
9. زر Factory Reset وتدفق تغيير كلمة Wi-Fi الأولى في Android.
10. Release Candidate، اختبارات قصيرة، ثم 24h و7d.
11. حذف v1 وblur بعد نجاح الترحيل.

## 13. الحالة الحالية

- ESP8266 على `COM5`، والشاشة التجريبية `4x1`.
- Firmware مرفوع بخطين فقط: Basit/Janat وTahoma؛ Traditional وArial Bold ملغيان.
- `/setFont` يحفظ الخط واللغة والأرقام (`font=0..1&lang=0..1`) في EEPROM بدون إعادة تشغيل.
- Android v1 يرسل أرقاماً خاماً؛ Firmware الحالي يقبل raw ثم blur fallback.
- آخر Build: RAM ‏35,452 B (43.3%)، Flash ‏373,383 B (35.7%).
- Native/Soak مؤجلان أثناء الخط؛ الطويل يبدأ من الصفر بعد Android وRC.

## 13.1 سجل التنفيذ

- [x] إضافة Build Profiles للمقاسات `1x1` و`2x1` و`3x1` و`4x1`؛ الافتراضي `4x1`.
- [x] اشتقاق عرض الشاشة من `DISPLAYS_WIDE` وإضافة اسم المقاس لملف Firmware.
- [x] بناء Profiles الأربعة (نجاح 4/4، RAM ‏45.3%، Flash ‏36.6%).
- [x] كلمة Wi-Fi الأولى + حفظها + زر Factory Reset على GPIO2 (ضغط 10 ثوانٍ بعد الإقلاع).
- [x] API v2 الأساسي: parser + nonce + PBKDF2/HMAC + الإعدادات وhealth.
- [ ] Signed OTA v2 (مرحلة مستقلة؛ OTA v1 ما زال للمعمل فقط).
- [x] الاختبارات القصيرة ورفع نسخة `4x1`؛ HMAC حي وhealth/getData ناجحان.

## 14. شروط الإطلاق التجاري

- الخطوط والمقاسات الأربعة معتمدة بصرياً.
- تغيير الكلمة أول مرة وKeystore يعملان بعد إعادة تشغيل الهاتف.
- بعد تعطيل v1 لا يقبل الجهاز أمراً بلا HMAC أو مع replay.
- Signed OTA ناجح وفشل التوقيع آمن.
- لا secrets في logs أو repository أو artifacts.
- Build/Test لكل Profile ناجح مع هامش RAM/Flash.
- 24h ثم 7d بلا crash/WDT/freeze/heap trend/فساد EEPROM.
