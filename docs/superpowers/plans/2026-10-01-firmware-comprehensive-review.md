# خطة المراجعة الشاملة للفيرموير: الصلاة والإقامة والبساطة والأداء

> **لأي مساعد يستكمل:** اقرأ هذا الملف كاملًا ثم `AGENTS.md` و`docs/DECISIONS.md`. نفّذ المهام بالتتابع وحدث سجل التقدم هنا. يمكن استخدام `superpowers:executing-plans` إن كانت متاحة؛ لا تحتاج الخطة إلى مساعدين متوازيين أو أدوات خاصة كي تُستكمل.

**الهدف:** مراجعة صحة حساب الصلاة والإقامة وكل مسارات الفيرموير، ثم إصلاح الأسباب المثبتة بأقل تغيير، مع أداء مستقر 24/7 وتوافق الأجهزة القديمة.

**البنية المعتمدة:** نستبقي Arduino وDMDESP وPrayerTimes وRTClib والبنية الحالية. الحسابات الثقيلة عند تغير المدخلات، والوقت الداخلي بنظام 24 ساعة، والعرض بواسطة الكاش والإطارات المكتملة. لا إعادة كتابة شاملة ولا تحويل async تلقائيًا؛ يُبسّط المسار الذي يثبت أنه يحتاج ذلك.

**التقنيات:** ESP8266/esp12e، PlatformIO، Arduino C++، DS3231/I2C، P10، HTTP/AP، EEPROM/CRC، اختبارات Unity native الموجودة.

**المواصفة:** طلب المالك بتاريخ 2026-10-01: استمرار تطوير الفيرموير، مراجعة طرق الصلاة والإقامة وكل الأكواد، والتركيز على البساطة والأداء. هذا الملف هو الخطة وسجل التقدم الحاليان؛ `RELIABILITY_PLAN.md` و`RELIABILITY_PROGRESS.md` مراجع تاريخية، وليسا دليلًا أن هذه المراجعة الجديدة اكتملت.

## 1. حالة التسليم الحالية

- نقطة الرجوع: tag `firmware-rc1`، commit `2d83464cf08eb08ed4d4f9777fa08dc468faaf8a`، إصدار `P104-rc1`.
- المالك قال إنه رفع النسخة وجربها و«تمام». هذا تأكيد تشغيل أولي، وليس دليل اختبار تحمل أو صحة كل طرق الحساب.
- المقاسات المدعومة: 1×1، 2×1، 3×1، 4×1؛ ارتفاع الشاشة 16 بكسل. 4×2 خارج النطاق.
- موضوع الخطوط انتهى حسب المالك: Basit/Tahoma، ارتفاع 7 و16، وأرقام غربية/هندية. لا إعادة توليد أو تغيير الرسم ضمن المراجعة إلا لعطل مثبت ومطلوب.
- يوجد وقت إعداد الخطة تعديل محلي للمالك في `GLCDFontCreator2/BasitFont16.ftsd` و`lib/DMDESP/fonts/BasitFont16.h` وملفات تصدير/Java غير متتبعة. احفظها كما هي، وافحص `git status` من جديد عند الاستئناف.
- R0/R1 بدأا، ونُفذت إصلاحات R2/R4 الأولى مع اختبارات native وبناء المقاسات الأربعة؛
  خيار مكة الجديد firmware-only ومسار Android الخاص به مؤجل في
  `docs/ANDROID_DEFERRED_PLAN.md`.
- **المهمة التالية:** إكمال R1/R2/R4 بالأرقام المرجعية وحدود النوافذ، ثم الانتقال
  إلى تدقيق R5–R8 دون تغيير عقد الأجهزة القديمة.

خط أساس RC1 من سجل البناء، وليس قياس heap أثناء التشغيل:

| Profile | RAM ساكنة (بايت) | Flash البرنامج (بايت) |
|---|---:|---:|
| p10_1x1_release | 37412 | 426131 |
| p10_2x1_release | 37436 | 426235 |
| p10_3x1_release | 37528 | 427439 |
| p10_4x1_release | 37536 | 427591 |

حجم `.bin` مختلف عن Flash البرنامج؛ الأحجام والبصمات في `release/firmware-rc1/manifest.txt`. كانت آخر نتيجة native مسجلة 19/19؛ أعد التحقق عندما يتغير المنطق ولا تنسب النتيجة القديمة للكود الجديد.

## 2. القيود والقرارات المستمرة

1. البساطة أولًا: أعد استخدام المساعدات والمكتبات الحالية، وحذف التكرار المثبت قبل إضافة طبقات أو مكتبات أو تخصيصات «للمستقبل».
2. إصلاح السبب المشترك بعد تتبع كل المستدعين. أي بند مراجعة ليس عطلًا مؤكدًا حتى توجد حالة إعادة إنتاج أو نتيجة قياس.
3. دعم الأجهزة القديمة أساسي: حافظ على `blur/unBlur` وعقد `/getData` والأوامر القديمة. المسار التجاري OTA هو raw `/updateMain`؛ النظام الموقّع مؤجل وهندسي فقط.
4. حماية العميل هي كلمة مرور AP القابلة للتغيير، واستعادتها بالزر الفعلي؛ لا إعادة فرض كلمة مرور إضافية أو HMAC أو تغيير إجباري أول استخدام.
5. لا JSON أو تبعية جديدة لمجرد التنظيم. لا `String` في المسارات الساخنة؛ `std::string` يخصص heap أيضًا وليس علاجًا تلقائيًا للتجزئة.
6. لا تغيير EEPROM_VERSION أو layout دون ضرورة مثبتة، وخطة ترحيل تحافظ على إعدادات العميل، وتوثيق القرار. الإصدار الحالي EEPROM_VERSION=6.
7. `lib/*` للقراءة، عدا الخطوط المسموح بها. أي تعديل لا غنى عنه بمكتبة أخرى يحتاج تفسير الضرورة والتوافق في DECISIONS؛ حاول إصلاح واجهة الاستدعاء أولًا.
8. حافظ على double buffering وdirty frames وسلوك timer المثبت على العتاد. تغيير DMD سبق أن سبب رسمًا من أعلى لأسفل؛ لا تعاود التجربة دون قياس وحالة محددة.
9. أثناء OTA الحالي يتوقف DMD ثم يعود لرسالة النتيجة وإعادة التشغيل. لا تعيد شريط التقدم دون طلب؛ سبق أن سبب رعشة ومشكلات فعلية.
10. العمل الأساسي هنا P10. 7-segment مراجعة مقارنة؛ أي تعديل فيه ينفذ كدفعة مستقلة واضحة. Android يعدل فقط لعقد مشترك يحتاجه الإصلاح؛ لا APK، والمالك يجرب من Android Studio.
11. لا رفع/مسح flash أو ضبط RTC على الجهاز في مهمة التوثيق أو المراجعة المصدرية. عند طلب رفع لاحقًا تحقق من المقاس والمنفذ الفعلي؛ لا تفترض COM5 لمجرد أنه استُخدم سابقًا.
12. المالك طلب توفير التوكنز: قراءة موجهة، ملخصات قصيرة، اختبارات قصيرة مناسبة للتغيير. الاختبار الطويل عند المالك؛ لا مؤقتات مراقبة أو تشغيل مستمر دون طلب.
13. خطط التحسين لا تمنح إذن نشر إصدار تجاري أو غلق التطوير. يبقى RC1 نقطة رجوع، وكل دفعة لها commit صغير عند تنفيذها.

## 3. خريطة الملفات والمسارات

| المسار/الرمز | المسؤولية ومجال المراجعة |
|---|---|
| `src/Masjed-P104.cpp` | setup/loop، جميع الخدمات والـhandlers والعرض وOTA |
| `calcPrayers()` | اختيار الطريقة والمذهب، المنطقة الزمنية، التحويل لدقائق والكاش |
| `isDST()` / `syncRTC()` / `handleSetDateTime()` | علاقة الوقت الأساسي في RTC بالتوقيت الصيفي والوقت المحلي |
| `updatePrayerWindow()` / `checkPrayers()` / `sleeping()` | اختيار الحدث وأولويته والعد التنازلي والنوم |
| `src/globals.h` | Configuration، PrayerCache، EqamaCache، timeData، الفهارس |
| `src/TimeMath.h` | signedMinuteDelta، minuteInWindow، التقويم، displayHour |
| `lib/PrayerTimes/PrayerTimes.{h,cpp}` | معادلات الشمس، معاملات الطرق، الحالات القطبية، تقريب الدقائق |
| `src/SmartText.{h,cpp}` / `ArabicShaper.*` / `texts.h` | النصوص والكاش والذاكرة وسلامة اتجاه الوقت |
| `src/DisplayProfile.h` / `CardScheduler.h` | مناطق المقاسات وترتيب البطاقات وأولوية العرض |
| `saveConfig()` / `loadConfig()` / card preferences | CRC والترحيل ونتيجة الحفظ والكتابة عند التغيير |
| `test/test_time_math` / `test/test_card_scheduler` / `test/test_shaper` | اختبارات native الحالية، توسع فقط بما يثبت الإصلاح |
| `platformio.ini` / `renameFirmware.py` | profiles وأسماء الملفات؛ لا تكسر تسميات التحديث |
| `tools/release_ota.ps1` / `tools/write_release_manifest.py` | تسليم raw firmware وأحجامه وبصماته؛ راجع النسخة والوجهة قبل استخدامهما |

المشروع المرتبط للقراءة: `C:/Users/m_nag/Desktop/Arduino/Projects/Masjed_TA_All_v2`.
Android الصحيح: `C:/Users/m_nag/Desktop/Android_Projects/MasjedClockWiFi`؛ توجد مسارات قديمة متشابهة في المحادثة، فلا تعدلها بالخطأ.

## 4. نقاط تحقق أولية من قراءة المصدر

هذه ملاحظات قابلة للتحقق وليست تقرير أعطال مكتمل:

| البند | ما قرئ فعليًا | ما يجب إثباته |
|---|---|---|
| فهارس الصلاة | مكتبة PrayerTimes: 0 فجر، 1 شروق، 2 ظهر، 3 عصر، 4 غروب Sunset، 5 مغرب، 6 عشاء. PRAYER_INDICES الحالية `{0,1,2,3,4,6}` | هل اختيار index 4 للمغرب مقصود ويطابق كل الطرق؟ index 5 ليس شروقًا كما يقول AGENTS القديم |
| طريقة مكة | `calcPrayers` يضبط Fajr/Maghrib/Isha على Custom صراحةً، مع `makkahIsha=0/1` | تحقق رقمي محلي للزاوية المتوافقة مقابل 90 دقيقة، دون تغيير المكتبة |
| مفاتيح الكاش | PrayerCache and Hijri cache now store year/month/day | config changes already invalidate both caches |
| الإقامة الطويلة | checkPrayers يحتوي شرط استبعاد since>30 لبعض الأيام/الصلوات؛ الإعداد يسمح 0..120 | اختبار إقامة 31 و60 و120 دقيقة ونوافذ النوم؛ هل الاستبعاد يخالف الإعداد؟ |
| عبور منتصف الليل | since يستخدم signedMinuteDelta لكن نص الدقائق المتبقية يستخدم `(pt+eq)-nowMins` | هل النص يعرض 1440 دقيقة إضافية بعد منتصف الليل؟ اختبر حالة محددة |
| إقامة 0 و1 | فرع الأذان since=0..1 يسبق فروع الإقامة | ما المعنى المنتجى للقيم 0/1؟ تعطيل أم إقامة فورية؟ لا تغيّر المعنى دون تثبيته |
| التعافي RTC | توجد إعادة محاولة وباص recovery؛ loop يعرض خطأ إذا rtcOk=false | تحقق مما يستمر فعلًا من الوقت والعرض والصلاة أثناء الفقد، وهل السلوك هو المطلوب |
| tick بعد عمل حاجب | loop يزيد ثانية واحدة ويضبط lastSecondMs=now | هل الوقت البرمجي يفقد الزمن المنقضي بين مزامنات RTC بعد طلب بطيء أو عدة ثوانٍ؟ |
| التوقيت الصيفي | isDST يقبل تاريخًا فقط وconfig.dst عدد صحيح يمثل offset المنطقة | حدود ساعة الانتقال، الإضافة مرة واحدة، وعدم تطبيق قاعدة مصر تلقائيًا على بلد آخر |

## 5. مراحل التنفيذ ومعايير القبول

### R0 — خط الأساس وجرد الأكواد

**الملفات:** هذا الملف، `docs/ROADMAP.md`، وجرد `src/` و`tools/` و`platformio.ini` والمكتبات المستدعاة.

- [ ] سجل commit/branch/status الحاليين، وافصل تعديلات المالك عن الدفعة؛ لا commit شامل ولا reset لإخفائها.
- [ ] تتبع المسار setup → loop → RTC → الحساب → النوافذ → العرض، وHTTP → validation → save → cache refresh → reply.
- [ ] أنشئ جدول جرد هنا بأسماء الملفات/الوحدات وحالة قراءتها؛ يشمل كل كود المشروع وأدوات البناء، ثم المكتبات بقدر المسارات المستخدمة. ميز read-only dependencies عن كودنا.
- [ ] سجل فرق المصدر الحالي عن RC1، وقياسات البناء الحالية حين يبدأ تنفيذ الكود. لا تبنِ نسخًا متكررة لمجرد التوثيق.

**القبول:** كل وحدة لها مسؤولية واضحة ومراجعة مخططة، والتعديلات غير التابعة للخطة محفوظة، وخط أساس قابل للمقارنة.

### R1 — مواصفات نتائج الصلاة والإقامة

**المخرجات عند التنفيذ:** `docs/PRAYER_TIME_SPEC.md` و`docs/PRAYER_REFERENCE_CASES.csv`؛ CSV تطوير فقط، ليس بروتوكولًا على الجهاز.

- [ ] اكتب mapping لقيم cm=0..4 وwm=0..1 بين Android والفيرموير ومعاملات المكتبة الفعلية، بدون افتراض أن الاسم يكفي.
- [ ] وثق وحدة كل قيمة: offset الأساسي بالساعات، DST إضافة مستقلة، إحداثيات بدرجات، الصلاة دقائق 0..1439، الإقامة دقائق 0..120، displayHour للعرض فقط.
- [ ] ثبت مع المالك المعاني غير المحسومة: إقامة 0، الشروق كحدث منفصل بلا أذان، الجمعة ونهاية نومها، وسياسة الطريقة المخصصة؛ استمر في مراجعة البنود المستقلة أثناء انتظار الإجابة.
- [ ] اختَر حالات مرجعية: القاهرة والإسكندرية ومكة ومدينة مرتفعة العرض، أيام شتاء/صيف/اعتدال، يوم كبيس وحدود DST ورمضان. سجل الإحداثيات والـoffset والطريقة والمذهب وتاريخ المرجع.
- [ ] استخدم مصدرًا رسميًا عند اعتماد المواقيت/قواعد DST، ومرجع معادلات مستقلًا للتحقق العددي؛ سجل الرابط وتاريخ الوصول والافتراضات، ولا تكتب نتائج متوقعة من نفس الكود الذي تختبره.
- [ ] صفوف CSV المقترحة: id,date,lat,lon,utc_offset,dst_hours,method,asr_method,event,expected_minute,tolerance_minutes,source,notes. تحقق أن المتوقع دقائق يومية صالحة؛ لا تعمم ترتيب ساعات اليوم على صلاة عبر منتصف الليل.
- [ ] ميز فرق التقريب (حد عددي أولي ≤1 دقيقة عند تطابق المعادلة والمعاملات) عن فرق التقويم الرسمي/الاحتياط/الارتفاع؛ لكل مرجع سماحية معللة معتمدة، ولا تعدل الزوايا لتطابق رقمًا منفردًا.

**القبول:** لكل طريقة مدعومة حالة مستقلة على الأقل، وعقد الإقامة موثق، وكل فرق له سبب أو بند تحقيق؛ لا ادعاء أن الحساب يغني عن تقويم الجهة المحلية.

### R2 — صحة محرك حساب الصلاة والكاش

**الملفات:** `calcPrayers`، `globals.h`، `TimeMath.h` عند الحاجة، قراءة PrayerTimes؛ `test/test_prayer_times/test_main.cpp` وnative configuration فقط إذا احتاج الاختبار الحقيقي ذلك.

- [ ] شغّل الحالات المرجعية على واجهة الحساب الفعلية وحدد الفروق قبل أي تعديل؛ لا تكتفِ باختبار دالة مكررة تعيد نفس الحساب.
- [ ] راجع معاملات كل طريقة واستدعاءات setters وترتيبها وindex المغرب، والتقريب ومجال double إلى integer، وحالات NaN/Inf والأوقات غير المتاحة.
- [ ] أضف حالة فاشلة لكل سبب مثبت، ثم أصلحه في نقطة الاستدعاء المشتركة؛ حافظ على المكتبة ما أمكن، وأي تغيير ضروري فيها موثق في DECISIONS.
- [ ] اختبر تغيير شهر/سنة مع نفس رقم اليوم، تغيير الموقع/الطريقة/المذهب/offset، وتبديل DST؛ اجعل صلاحية الكاش تغطي المدخلات المؤثرة كاملة بأبسط مفتاح أو invalidation موثوق.
- [ ] اختبر الإحداثيات الحدية والتواريخ غير الصالحة والطرق غير المدعومة. نتيجة حساب غير صالحة لا تتحول إلى 00:00 ولا تشغل نافذة صلاة وهمية؛ سياسة العرض للوقت غير المتاح تثبت في R1.
- [ ] احسب فقط عند تغير المدخلات الفعلية؛ تحديث الخط/السطوع/البطاقات لا يعيد المعادلات الفلكية بلا داعٍ.

**القبول:** تطابق الحالات المرجعية ضمن السماحية، صحة mapping، لا cast لنتيجة غير finite، ولا stale cache أو حساب ثقيل دوري غير لازم.

### R3 — مصدر الوقت وDST والتقويم

**الملفات:** `isDST`، `syncRTC`، `handleSetDateTime`، `tickSecond`، `serviceRTC`، `calcHijri`، `TimeMath.h`، `test/test_time_math/test_main.cpp`.

- [ ] وثق هل RTC يخزن الوقت المحلي الأساسي دون الصيفي، وكيف يعود الوقت المرسل من الموبايل إلى هذا الأساس؛ config.dst اسمه تاريخي فلا تعامله كـboolean.
- [ ] اختبر الشتاء/الصيف قبل الانتقال وعنده وبعده، والساعة المتكررة/المحذوفة، ضبط الوقت يوم الانتقال، تبديل st، ونهاية الشهر والسنة والكبيسة 2028/2100 ضمن حدود دعم كل دالة.
- [ ] تحقق من مصدر قاعدة DST ونطاق البلدان المقصود. لا تضف قاعدة عالمية أو offset نصف ساعة للمنتج دون احتياج؛ وثق حدود offset الصحيح الحالية بدل تقريب قيم غير مدعومة بصمت.
- [ ] اختبر توقف loop 2–5 ثوانٍ، الرجوع/القفز بالوقت من التطبيق أو RTC، وعدم تكرار الحدث بسبب تصحيح الوقت؛ أصلح انجراف tick إن ثبت.
- [ ] اختبر I2C فاشل/RTC بتوقيت غير صالح/lostPower/تعافي، وحدد سياسة الوقت التقريبي مقابل الوقت الموثوق وتوقيت العودة للحساب؛ لا restart loop للتعافي.
- [ ] راجع الهجري وتصحيح ±3 وأطوال الشهور وتغير التاريخ؛ هذا حساب تقويمي وليس وعدًا بتطابق الرؤية الرسمية.
- [ ] ابحث عن NTP/STA فعلًا. لم يظهر مسار NTP في القراءة الأولية؛ إذا كان غائبًا سجل ذلك ولا تضف اتصال إنترنت ضمن المراجعة.

**القبول:** الصيفي يضاف مرة واحدة، لا تواريخ صفرية تعرض كبيانات صحيحة، تعافٍ محدود دون تعليق، وحساب الصلاة والعرض لا يتوقفان انتظارًا للشبكة.

### R4 — نوافذ الأذان والإقامة والجمعة

**الملفات:** `updatePrayerWindow` و`checkPrayers` و`refreshEqamaCache` و`TimeMath.h`؛ `test/test_prayer_windows/test_main.cpp` عند الحاجة لاختبار القرار الفعلي.

- [ ] ارسم جدول حالات موحد: قبل الحدث، الأذان/الشروق، دقائق الإقامة، آخر 60 ثانية، الإقامة، النوم، الأذكار، الرجوع للكروت؛ سجل شروط البداية والنهاية وشمول الحدود.
- [ ] اختبر لكل صلاة t-181/t-180/t-1/t/t+59/t+60 ثانية، ونقاط eq-1/eq/eq+sleep؛ قيم eq=0,1,2,10,30,31,60,120، ونوم 0 و1 و120.
- [ ] اختبر عبور منتصف الليل: صلاة 23:55 مع إقامة 10، يجب أن تكون الدقائق المتبقية عند 00:02 ثلاث دقائق وفق عقد R1؛ لا 1443 ولا قيمة سالبة.
- [ ] اختبر الجمعة والظهر في غير الجمعة والشروق، وتعطيل الإقامة إن كان 0 يعني التعطيل، وتداخل نافذتين وتغيير الإعداد داخل نافذة وضبط الوقت للخلف/الأمام.
- [ ] وحد قاعدة اختيار الحدث بين updatePrayerWindow وcheckPrayers لمنع window صحيحة لا تجد renderer أو العكس. أعد استخدام helpers؛ استخرج قرارًا صغيرًا قابلًا للاختبار فقط إن احتاجه الاختبار، دون إطار State Machine عام.
- [ ] تحقق من سياسة الأولوية وترتيب الفهارس، ونهاية النوم، واستعادة البطاقة دون شاشة سوداء أو قفزة متكررة. لا تخلط الشروق مع Sunset.

**القبول:** انتقال واحد واضح لكل حالة، العد التنازلي صحيح، والإعداد 0..120 يعمل حسب العقد، ونتائج صغيرة/كبيرة و12/24 لا تغير توقيت الحدث.

### R5 — مراجعة شاملة للذاكرة والبساطة

**الملفات:** كل `src/*` وأدوات البناء والملفات المستخدمة في `lib/*` للقراءة؛ سجل findings في القسم 8 قبل التنفيذ.

- [ ] جرد المكتبات والرموز والكود الميت والتكرار والمخازن الكبيرة. افحص كل String ومكان تخصيصها ومعدل تكراره؛ String في الإقلاع أو واجهة مكتبة لا يُستبدل تلقائيًا.
- [ ] راجع PROGMEM ونسخ `_P`، snprintf وترجيعاته، UTF-8 وحدود الأطوال وNUL، الفهارس والتحويلات الموقعة/غير الموقعة، والـstack لكل مسار عميق.
- [ ] افحص صلاحية pointers ومشاركة المخازن عبر callbacks، واستخدام volatile/atomic في مسار timer، وتسرب الذاكرة والفشل في تخصيص double buffer.
- [ ] أبق الحساب الفلكي والنصوص الثابتة خارج hot paths؛ قلل clear/redraw وإعادة shape عند ثبات المدخلات، دون تغيير رسم الخطوط أو الفصل بين الأرقام والحروف.
- [ ] احذف helper/field/dependency فقط بعد البحث عن الاستخدام في كل الملفات والمشاريع المرتبطة المؤثرة، وتحقق من أن حذف الحقل لا يغير EEPROM layout.
- [ ] سجل RAM/Flash قبل وبعد، والـheap/max block/fragmentation من تشخيص الفني أثناء HTTP/scroll؛ لا تخفض نوع الحساب من double إلى float إلا بدليل دقة وأداء.

**القبول:** كل حذف/تحسين له دليل، لا زيادة غير معللة في الذاكرة، لا trend هبوط heap تحت حمل متكرر، ولا abstraction أو تبعية جديدة بلا ضرورة.

### R6 — حلقة التشغيل والعرض والشبكة وWDT

**الملفات:** setup/loop، SmartText، DisplayProfile، CardScheduler، خدمات WiFi/RTC، handlers وOTA.

- [ ] افحص delay/wait/while وكل أعمال callbacks وحدود عمل I2C وHTTP. فرق بين delayMicroseconds قصير للباص وبين انتظار ثوانٍ، وبين yield وإزالة الحجب فعلًا.
- [ ] استخدم قياسات loop الموجودة لتحديد المسار المسبب للرعشة أو التأخير. لا تعمم حد زمن من RAM build؛ سجل أطول زمن وعلاقة التأخير بالمسح على اللوحة الحقيقية.
- [ ] اختبر rollover لـmillis عند نحو 49.7 يومًا باستخدام وقت محاكى، deadlines restart/cards/retries، وكذلك micros إن استُخدم لزمن loop.
- [ ] افحص AP توقف/عودة، تغيير كلمة المرور والرد قبل restart، العملاء المتعددين وHTTP البطيء والطلبات المقطوعة، ومنع retry/restart غير محدود.
- [ ] افحص الأولوية: OTA والرسالة النهائية، restart، splash/test، RTC fault، الصلاة، البطاقات، وتوفير الطاقة؛ تحقق أن تعطيل البطاقات لا يخفي حدث الصلاة.
- [ ] قارن العرض عبر الأربعة مقاسات والصغير/الكبير والعربي/الإنجليزي و12/24 والأرقام؛ فحص smoke قصير، وليس فتح مشروع خط جديد.
- [ ] لا تنقل إلى ESPAsyncWebServer أو تغير مؤقت DMD استباقيًا؛ لا async إلا إذا أثبت القياس ضرورة وكانت الفائدة تتجاوز مخاطر التوافق.

**القبول:** خدمة العرض والصلاة مستمرة خارج الاستثناء المقصود للـOTA، لا WDT/boot loop ولا starvation مثبت، retries محدودة ومواعيد millis سليمة.

### R7 — الحفظ وAPI وOTA والتوافق

**الملفات:** Configuration/load/save/cardPrefs، جميع HTTP handlers، callbacks OTA، release tools، قراءة العقد المقابل في Android و7-segment.

- [ ] اختبر أعدادًا خارج النطاق، ناقصة/فارغة/غير رقمية، NaN/Inf، UTF-8 غير صالح، رسائل طويلة وحجم body مبالغًا فيه؛ الفشل لا يعدل config جزئيًا.
- [ ] تحقق أن رد نجاح الحفظ يأتي بعد نجاح التخزين، وأن readback يعيد القيم نفسها بما فيها الشروق 0 أو غيره حسب R1، والموقع والطريقة والوقت.
- [ ] تحقق من skip commit عند عدم التغير، CRC وترحيل v4/v5 إلى v6، defaults عند التلف، حدود sidecar البطاقات داخل EEPROM.begin(512)، وفشل commit بدون فقد آخر إعداد مقبول.
- [ ] لا تضف deferred commit إن كان سيعطي نجاحًا قبل الحفظ؛ لا تخزن telemetry أو tick في flash، ولا A/B storage دون متطلب مثبت.
- [ ] راجع legacy/v2 ومسارات capabilities في Android: نفس معنى المعاملات والفهارس، لا تعديل CSV قديم يربك الأجهزة المباعة أو 7-segment.
- [ ] اختبر OTA صالح/ملف غير صالح/قطع اتصال/حجم غير مطابق/خطأ كتابة، ثم رسالة النجاح وrestart مرة واحدة؛ راجع مدى كشف ملف profile خاطئ بالآلية الموجودة، ولا تدع وجود CRC/SHA يثبت توافق المقاس.
- [ ] افحص تطابق version/profile/اسم الملف ووجهة asset والـmanifest؛ سكربت manifest الحالي يثبت RC1 صراحةً، فلا تستخدمه لإصدار آخر دون تحديث مقصود.

**القبول:** لا حفظ جزئي أو نجاح وهمي، لا كتابة flash متكررة لنفس الإعداد، raw OTA القديم متوافق، والإعدادات محفوظة بعد restart/OTA.

### R8 — المقارنة مع 7-segment والتحقق والتسليم

- [ ] اقرأ مشروع 7-segment وحدد فروق المكتبة والطرق والفهارس والـAPI وDST/RTC؛ سجل مصفوفة فرق. لا تفترض أن P10 و7-segment يشتركان في enum أو defaults.
- [ ] إذا طُلب إصلاح 7-segment، نفذه في مستودعه كسجل/commit مستقل، ولا تغير قدرة حقول الساعة ذات الخانة الواحدة تلقائيًا إلى 24 ساعة.
- [ ] نفذ native المناسبة مرة بعد التغيير؛ بناء release للمقاسات الأربعة قبل تسليم الدفعة الجامعة. اختبارات الشبكة/العرض القصيرة فقط مع عتاد متصل وطلب واضح.
- [ ] أعط المالك قائمة تجربة عملية قصيرة: حد أذان/إقامة، الجمعة والشروق، تغيير موقع/وقت، RTC فصل/عودة، حفظ/قراءة/restart، OTA. سجل جهاز/profile/version لكل نتيجة.
- [ ] المالك ينفذ اختبار تحمل 24 ساعة ثم المدة الأطول التي يختارها؛ لا تضع [x] بمجرد نجاح build أو قوله «تمام» عن خطوة غير محددة.
- [ ] تحديث findings/DECISIONS/ROADMAP، ثم RC تالٍ إذا تغير الكود، مع SHA-256 وcommit محدد؛ لا commercial tag أو غلق التطوير دون طلب المالك.

**القبول:** كل P0/P1 مغلق بالدليل، وكل فرق حساب مفسر، وكل خطر متبقٍ له وصف وقرار، والتوافق والقياسات والاختبار الحقيقي متميزون عن نجاح الترجمة.

## 6. أوامر عملية للمساعد التالي

نفذ من جذر المشروع. هذه أوامر لاحقة للمراجعة والتنفيذ، لم تُشغّل ضمن إنشاء الخطة.

```powershell
git status --short
git log -1 --oneline
git diff --stat firmware-rc1
rg -n 'calcPrayers|updatePrayerWindow|checkPrayers|syncRTC|tickSecond|isDST|saveConfig|EEPROM.commit|delay\(|String|while\(' src

# بناء دفعة جامعة، -j 1 يقلل ذاكرة جهاز التطوير (سبق فشل مترجم بنفادها).
& 'C:\Users\m_nag\.platformio\penv\Scripts\pio.exe' run -j 1 -e p10_1x1_release -e p10_2x1_release -e p10_3x1_release -e p10_4x1_release

# Native: يستخدم toolchain الموجود؛ لا تثبيت إضافي إذا كان كافيًا.
$env:Path = 'C:\Users\m_nag\.platformio\packages\toolchain-gccmingw32\bin;' + $env:Path
& 'C:\Users\m_nag\.platformio\penv\Scripts\pio.exe' test -e native

git diff --check
```

البيئة native الحالية تبني ArabicShaper فقط، وتختبر شaper/time_math/card_scheduler. لا تدّع اختبار PrayerTimes نفسه إلا بعد إضافة المسار الحقيقي للاختبار؛ يمكن adapter host صغير إن احتاج Arduino، دون تعديل مكتبة العتاد لمجرد تشغيل host.

## 7. سجل التقدم — حدّثه بعد كل دفعة

العلامات: `[ ]` لم يبدأ، `[~]` جارٍ، `[x]` مكتمل بالدليل. ضع «مراجعة فقط/إصلاح/تحقق على العتاد» صراحةً.

| المهمة | الحالة | الدليل / الخطوة التالية |
|---|---|---|
| إنشاء خطة وتسليم سياق | [x] | 2026-10-01: هذا الملف وروابط الاستئناف؛ تغييرات توثيق فقط |
| R0 خط الأساس والجرد | [x] | 2026-10-01: baseline في ledger و`r0-baseline.txt`؛ لا كود تغير |
| R1 مواصفة الصلاة والإقامة | [~] | `docs/PRAYER_TIME_SPEC.md` و`docs/PRAYER_REFERENCE_CASES.csv`؛ الأرقام والقرارات المحلية معلقة |
| R2 الحساب والكاش | [~] | 2026-10-01: explicit table + selectable Makkah rule + full-date prayer cache key; numeric reference comparison remains |
| R3 الوقت وDST/RTC/التقويم | [~] | 2026-10-01: full-date caches and bounded second-tick catch-up; DST/RTC hardware cases remain |
| R4 نوافذ الصلاة والإقامة | [~] | 2026-10-01: cross-midnight countdown and centralized phase classifier; 0..120/Friday/Sunrise hardware boundaries remain |
| R5 تدقيق الذاكرة والبساطة | [~] | 2026-10-01: bounded malformed UTF-8 lookahead; dynamic String uses remain cold HTTP/boot paths; heap soak remains |
| R6 loop والعرض والشبكة | [~] | 2026-10-01: loop catches up up to 60 elapsed seconds; hardware max-loop/OTA cases remain |
| R7 الحفظ وAPI/OTA | [~] | 2026-10-01: strict OTA version/size parsing added; save/OTA fault matrix remains |
| R8 المقارنة والتسليم | [~] | 2026-10-01: compatibility matrix added; 7-segment Maghrib index mismatch is isolated as R8-001; independent numeric/hardware comparison remains |

قالب كل جلسة، أضفه أسفل الملف:

```text
التاريخ / المهمة:
commit البداية / branch / تغييرات المالك المتروكة:
الملفات/المسارات التي روجعت:
السبب المثبت وحالة إعادة الإنتاج:
التعديل ومعناه للمستخدم:
التحقق: الأمر + النتيجة + RAM/Flash قبل/بعد عند تغير الكود:
نتيجة المالك على الجهاز/profile/version (إن وجدت):
commit الدفعة (إن وُجد):
المتبقي/العائق المحدد:
المهمة التالية والملف/الرمز الذي يبدأ منه المساعد:
```

## 8. سجل النتائج والقرارات

لم تسجل أعطال جديدة مثبتة بهذه الخطة بعد. نقاط القسم 4 تتحول إلى نتائج فقط بعد التحقق.

لكل finding: `ID | P0/P1/P2 | file:function | trigger | expected/actual | evidence | minimal fix | verification | status/commit`.

- P0: crash/WDT/فقد إعدادات/كتابة خارج حدود.
- P1: وقت صلاة/إقامة خاطئ، stale cache، فقد حدث، تعليق أو تعافٍ خاطئ.
- P2: تكلفة أو تكرار أو عرض يمكن تحسينه مع بقاء الوظيفة صحيحة.
- لا refactor شامل لمجرد حجم الملف؛ التغييرات الصغيرة القابلة للإثبات هي الافتراضية.

`R2-001 | P1 | src/Masjed-P104.cpp:calcPrayers | كل اختيار طريقة كان يستدعي Fajr/Isha setters التي تختار Custom، وMaghrib تركته الحالة الافتراضية غير المهيأة | إعدادات الطرق لم تكن صريحة | الدليل: PrayerTimes.h/cpp ووقت البناء | أضيفت src/PrayerMethodConfig.h، وضُبطت قواعد Fajr/Maghrib/Isha صراحةً، وأضيفت forward declarations للـminute setters دون تعديل المكتبة | test_prayer_method_config 3/3، native 23/23، release profile 1/1 بعد الدفعة؛ المقاسات الأربعة السابقة 4/4 | resolved in working tree, awaiting independent/local numeric reference and owner hardware check`.
`R2-002 | P1 | src/Masjed-P104.cpp:calcPrayers + EEPROM sidecar | طريقة مكة تحتاج قرار 90 دقيقة ثابتة أو قاعدة الطريقة دون كسر الأجهزة القديمة | غياب خيار محفوظ في العقد السابق | sidecar offset 460 يحفظ mode 0/1، وv2 يقرأ/يكتب `makkahIsha` اختيارياً؛ CSV القديم لم يتغير | native method-mode tests 3/3 + release build; Android deferred in docs/ANDROID_DEFERRED_PLAN.md | resolved in working tree; Android UI and local numeric reference remain deferred`.
`R2-003 | P1 | src/Masjed-P104.cpp:calcPrayers/calcHijri | صلاحية الكاش كانت تعتمد على رقم اليوم فقط | نفس اليوم الرقمي في شهر/سنة جديدة كان يعيد مواقيت الصلاة/الهجري السابقة | أضيف مفتاح year/month/day runtime لكلا الكاشين مع اختبار sameCalendarDate | test_time_math 6/6، native 24/24، release profiles 4/4 (RAM 37408/37428/37524/37544; Flash 426559/426699/427899/428055) | resolved in working tree; hardware/reference checks remain`.
`R7-001 | P1 | src/Masjed-P104.cpp:authorizeOtaV2 | `strtoul` accepted trailing characters in version/size | malformed manifest metadata could be partially accepted | reuse strict `parseLongValue` with positive bounds | native 24/24; release profiles 4/4 (RAM 37404/37424/37520/37540; Flash 426475/426615/427815/427971) | resolved in working tree; hardware/network OTA checks remain`.
`R3-001 | P1 | src/Masjed-P104.cpp:loop/tickSecond | deadline was reset to `now` after a stall | 2–5 seconds of blocking work silently lost software-clock seconds | consume elapsed whole-second ticks with a 60-tick bound; RTC remains authority | test_time_math 7/7, native 25/25, release profiles 4/4 (RAM 37408/37428/37524/37544; Flash 426623/426763/427963/428119) | resolved in working tree; real blocking/RTC correction cases remain`.
`R4-002 | P1 | src/Masjed-P104.cpp:checkPrayers | phase boundaries were duplicated across independent predicates | future edits could make renderer and window semantics diverge | add tested `prayerPhase()` and consume it in the existing renderer; preserve Friday special path | test_time_math 8/8, native 26/26, release profiles 4/4 (RAM 37408/37428/37524/37544; Flash 426607/426747/427947/428087) | resolved in working tree; hardware boundary checks remain`.
`R4-001 | P1 | src/Masjed-P104.cpp:checkPrayers | بعد منتصف الليل كان عداد الإقامة يستخدم طرحًا خامًا رغم أن نافذة الحدث تعبر 1440 | قد تظهر قيمة أكبر من 1440 بدل الدقائق المتبقية | اختبار forward distance عبر منتصف الليل فشل قبل helper | أضيفت `forwardMinuteDistance()` واستخدمت في النص فقط مع إبقاء النافذة كما هي | `test_time_math` 5/5، native 25/25، release profiles 4/4 | resolved in working tree; event semantics still need hardware/reference cases`.
`R8-001 | P1 | Masjed_TA_All_v2/src/Masjed_TA.cpp:Checking | الحلقة كانت تستبعد `times[5]` وتربط `times[4]` بالإقامة/الأذان المسماة Maghrib | 7-segment كان يعرض Sunset كالمغرب ويتجاهل وقت المغرب الحقيقي | قراءة مباشرة للكود + مواصفة الفهارس الحالية؛ لا تخمين من أسماء الحقول | تم توحيد جدول الزوايا والفهارس في commit 7-segment `8996bab`، مع إبقاء P10 دون تغيير | ta73/ta73s build 2/2؛ fixed-date numeric and hardware reference pending | resolved in separate repository; verification pending`.
`R5-001 | P1 | src/ArabicShaper.cpp:nextCp | malformed/truncated UTF-8 could inspect bytes past the NUL terminator and emit multiple replacements | API text could invoke undefined lookahead and noisy output | new truncated-sequence test failed before the fix (`'??'` vs `'?'`) | check continuation bytes before lookahead and consume one malformed sequence | shaper 11/11, native 27/27, release profiles 4/4 (RAM 37408/37428/37524/37544; Flash 426655/426795/427995/428135) | resolved; heap/long-message soak remains`.

## 9. تعليمات الاستئناف المختصرة

1. اقرأ الحالة وسجل آخر جلسة، ثم git status والأجزاء المتغيرة منذ آخر commit.
2. ابدأ بأول مهمة غير مكتملة؛ إن اكتشفت P0 موثقًا قدمه وسجل سبب تغيير الترتيب.
3. راجع كل caller قبل إصلاح helper، وأنشئ اختبارًا صغيرًا يثبت المشكلة ثم الإصلاح عند تغيير المنطق.
4. حدث السجل وROADMAP بعد الدفعة؛ لا تحتاج المحادثة القديمة لفهم ما اكتمل وما تبقى.
5. في الرد النهائي: نتيجة مختصرة، دليل التحقق وحدوده، ثم اقتراح عملي واحد يخدم المرحلة الحالية.
