# تقدم خطة تثبيت واعتمادية ESP8266

## Active review handoff — 2026-10-01

The owner confirmed RC1 initial operation and requested continued firmware
development. The current comprehensive prayer/iqama, simplicity, and performance
review and progress log are in
`docs/superpowers/plans/2026-10-01-firmware-comprehensive-review.md`.
Entries below are historical; their completed markers do not close the new
review. Plan prepared only; R0/R1 execution is next, no new build/upload/tests.

## Latest handoff — source-only, owner testing
- Android commercial-hardening source resumed; DB serial/model mapping preserved.
  HMAC/key storage/password UI integrated, command sequencing/readback improved.
- Added persisted hour24 display setting and getTimeFormat discovery. P10
  clock/prayers use it; computation unaffected. EEPROM v6 migrates v4/v5 CRC
  images. Seven-segment main-clock option added in its own project.
- Build/native 12 checks passed before owner requested NO FURTHER TESTS.
  Android and both seven-segment builds passed. Last Android-only edits untested.
- No APK generation or firmware upload in this batch. Time option requires
  deploying corresponding firmware. Owner performs tests through Android Studio.
- Follow-up upload requested by owner: latest P10 firmware uploaded to COM5;
  flash hash verified. No additional tests were run.
- Commercial gates are not closed: physical checks/soak, gating P10 legacy
  writes, and stronger seven-segment authentication remain. Full details in
  Android DISPLAY_PROGRESS.md. End user-facing responses with one suggestion.

Owner feedback: Android changes work. Future Android workflow is SOURCE EDITS
ONLY, no APK generation; owner tests from Android Studio. Recommendations for
commercial hardening are recorded in Android DISPLAY_PROGRESS.md; they are
not an authorization to change seven-segment firmware or publish a release.

## 2026-09-05 — Android display integration
- Android font/digit picker added behind P10 capability probe; UTF-8 transport
  fixed for modern P10, old P10/seven-segment branches retained.
- Firmware /getDisplaySettings returns Display,1,font,digits; setFont saves
  font and digits together. Empty text may now be cleared.
- Android assembleDebug/testDebugUnitTest PASS; Masjed-v2.8-debug.apk ready.
- P104 build/upload COM5 PASS, hash verified; RAM 37204, Flash 399667 bytes.
- Seven-segment firmware reviewed only; Android sleep parameter aliases fixed.
- Pending: phone tests with both types, HMAC/password UI. Android project
  DISPLAY_PROGRESS.md records handoff and seven-segment review findings.

## 2026-09-05 — Numerals and digit style
- Owner approved Arabic letters; preserve their exact bitmaps.
- Proven clipped original numeral: small '0' columns [120,4,4,120,0] lacked
  its bottom stroke. Added complete 5x7 Western numerals (10x14 large).
- tools/build_digit_fonts.py prepares all three families and both styles;
  digit height assertions PASS. Only numerals differ between style variants.
- digitStyle defaults to 0, persists via CRC-checked EEPROM v4->v5 migration.
- setSettings?digits=0 or 1; signed v2 equivalent uses the same field;
  v2/getData reports digits. Existing v1 CSV response unchanged.
- Unicode Indic/Persian digits normalize before bidi ordering; 11/11 native
  tests PASS. Build PASS: RAM 37172, Flash 399427 bytes.
- Android UI remains pending. Proposal: sample preview before saving choice.
- COM5 upload PASS with hash verification; post-upload serial reached RTC OK
  and Setup done. Migration log was not captured, so preservation of actual
  values is not independently confirmed. Physical numeral review is pending.

## 2026-09-05 — Janat extraction correction uploaded

Follow-up: added teh-marbuta dots to Janat's own heh body at both heights
(one-pixel dots at 7px, 2x2 at 16px). Regenerated BasitFont editor copies.
Build and second COM5 upload PASS with hash verification. Final Flash 382835
bytes; RAM unchanged. Converter equality/height checks PASS. Other shared
legacy forms still require visual review; native shaper was unchanged.

- Source: GLCDFontCreator2/Janat7.h and Janat16.h, using the adjacent
  ArabicShaping.java contextual map. Firmware BasitFont7/16 regenerated.
- Proven 7px packing issue: all source bit-zero values are blank, with 98
  populated bit-seven bytes. Shift original artwork right one bit to fit
  rows 0..6 without losing the bottom row in the editor.
- ASCII uses existing Tahoma ASCII assets; Arabic uses Janat only. The old
  map deliberately shares some forms (e.g. heh/teh-marbuta); this is faithful
  extraction, NOT proof that all 127 slots have unique/correct Arabic artwork.
- Converter checks saved glyph equality against source and height bounds.
  Reviewed desktop previews tools/out/janat_fixed7.png and janat_fixed16.png.
- Splash title now starts at y=0. No EEPROM version/layout changes this turn.
- Editable copies from shipped headers: BasitFont7.ftsd/BasitFont16.ftsd and
  Tahoma7.ftsd/Tahoma16.ftsd
  inside GLCDFontCreator2; existing editable projects preserved.
- pio run PASS; native tests 10/10 PASS. RAM 37156 bytes, Flash 382851 bytes.
- Serial upload COM5 PASS, flash hash verified; MAC 24:4c:ab:4f:ed:96.
- 35-second post-upload serial observation: RTC read invalid; no exception
  messages observed. Same RTC warning existed before upload. Needs follow-up.
- HTTP connection to 192.168.1.177 timed out; active saved font not verified.
  Owner can select family 0 via /setFont?font=0&tk=good when connected to AP.
- Pending: physical visual review, correct remaining shared legacy forms,
  investigate invalid RTC read. Long reliability soak remains paused.

> هذا الملف هو سجل الحالة الحي المقابل لـ`docs/RELIABILITY_PLAN.md`.
> يحدث بعد كل دفعة عمل أو اختبار عتاد.

## الحالة الحالية

| البند | الحالة |
|---|---|
| حالة العمل | **SOAK TEST PAUSED BY OWNER — التركيز الحالي على الخط ثم تطبيق Android** |
| ESP8266 موصول | نعم — ظهر على `COM5` في 2026-09-01 |
| منفذ COM معروف | نعم — `COM5` |
| Upload مصرح به | نعم — تم الرفع إلى `COM5` في 2026-09-01 |
| تم تعديل Firmware | نعم — دفعات الاعتمادية النهائية مرفوعة ومختبرة |
| توجد نسخة رجوع | نعم — نسخة Firmware قديمة لدى المالك؛ سحب Flash ملغي بطلبه |
| Git working tree عند إعداد الخطة | نظيف قبل إضافة ملفي الخطة |

## مؤشرات الحالة

- `[ ]` لم يبدأ
- `[~]` جارٍ
- `[x]` مكتمل ومتحقق
- `[!]` متوقف بسبب مشكلة أو قرار مطلوب

## تقدم المراحل

### المرحلة 0 — Preflight وRollback

- [x] تحديد `COM5` وESP8266EX وFlash ‏4MB؛ MAC: `24:4c:ab:4f:ed:96`.
- [x] سحب Full Flash backup ملغي بقرار المالك؛ توجد نسخة Firmware قديمة للرجوع.
- [x] حفظ Serial boot log الخاص بالعطل وSerial boot log بعد الإصلاح في هذا السجل.
- [ ] حفظ Firmware baseline الحالي.
- [x] تسجيل Git commit المستخدم للبدء: `834481c` مع Working tree موثق.

### المرحلة 1 — الإقلاع وPROGMEM

- [x] منع أي وصول إلى RTC قبل نجاح `rtc.begin()`.
- [x] حماية `effectiveBrightness()` واستخدام `timeData` بدل قراءة I2C مباشرة.
- [x] تصحيح جميع استعمالات نصوص PROGMEM المكتشفة في مسارات التشغيل.
- [x] Build واختبارات بعد الإصلاح.
- [x] اختبار 20 إعادة تشغيل عتادية متتالية مع RTC: ‏20/20 ناجحة.
- [x] اختبار RTC بوقت فاسد: رفض آمن وRetry دون WDT؛ الفصل الفيزيائي مؤجل لوجود المالك.

### المرحلة 2 — Configuration وHTTP

- [x] Validation مركزي وحدود كل الحقول ذات النطاق المعروف.
- [x] تطبيق ذري للإعدادات دون حالة جزئية.
- [x] رفض النقل المعيب/المقطوع والتحقق من UTF-8 للرسالة والاسم.
- [ ] قرار/تنفيذ Authentication للإعدادات وOTA.
- [x] اختبارات ميدانية: قراءة صحيحة، رقم خارج النطاق، blur معيب، وتاريخ غير صالح.

### المرحلة 3 — الصلاة والإقامة

- [x] إصلاح Eqama cache بنسخ القيم الست دائمًا.
- [x] إصلاح نافذة نوم الجمعة لاستخدام `sleepGomaaTime`.
- [x] دعم مقارنة نوافذ الصلاة التي تعبر منتصف الليل.
- [x] اختبارات Native لمنتصف الليل وحدود التقويم؛ ‏3/3 ناجحة.
- [ ] مقارنة أوقات الصلاة مع مرجع معتمد.

### المرحلة 4 — RTC/I2C/Wi-Fi

- [x] التحقق من `DateTime::isValid()` ومن نطاق السنة قبل قبول قراءة RTC.
- [x] Retry غير حاجب كل 30 ثانية عند غياب/فساد RTC؛ شوهد ميدانيًا.
- [~] تسجيل `lostPower()` ورفض الوقت الفاسد؛ اختبار فصل/عودة RTC ما زال مطلوبًا.
- [x] توحيد DST وPower Save مع `timeData` المعروض.
- [x] تعافي Wi-Fi AP غير حاجب كل 60 ثانية؛ AP وAPI عادا بعد OTA و20 Reset.

### المرحلة 5 — الشاشة وWDT وOTA

- [x] قياس أقصى زمن لـ`loop()` دوريًا؛ 12.5ms عادي و3.61s أثناء OTA (<8s WDT).
- [x] استعادة مؤقت DMD الأصلي؛ المالك أكد أن الصورة والحركة ممتازتان.
- [x] معالجة بدء/كتابة/نهاية/إلغاء وفشل OTA.
- [x] اختبار OTA خاطئ (`UpdateError` بلا Reset) وصحيح (`UpdateOk` ثم إقلاع سليم).
- [x] لا Flicker ولا WDT في العرض والـAPI وOTA و20 Reset.

### المرحلة 6 — EEPROM وHeap

- [x] منع Commit دون تغيير بمقارنة البايتات المشفرة قبل `EEPROM.commit()`.
- [x] لم يضف Deferred commit حتى لا يقرّ التطبيق حفظًا لم يحدث؛ منع الـcommit المتطابق يحقق حماية الكتابة دون تغيير العقد.
- [x] CRC32 مع metadata خارج `Configuration` وهجرة تلقائية بلا رفع `EEPROM_VERSION`.
- [x] Heap telemetry كل دقيقة: Free/Max block/fragmentation/loop max.
- [ ] اختبار فساد Config وانقطاع الطاقة وقت الحفظ.

### المرحلة 7 — العتاد والإطلاق

- [x] اختبار RTC وضبطه من وقت البناء مرة واحدة ثم حذف كود الضبط المؤقت.
- [~] P10 العربي والحركة ناجحان بصريًا؛ تبديل اللغتين والخط الصغير ينتظر المالك.
- [ ] اختبار الصلاة بزمن مسرع.
- [x] اختبار Web API فعليًا عبر Wi-Fi مع تنظيف بروفايل الاختبار بعد الانتهاء.
- [~] Soak test لمدة 24 ساعة بدأ 2026-09-01 18:30 ثم أوقفه المالك؛ سيبدأ من الصفر بعد إنهاء الخط وAndroid ورفع النسخة التالية.
- [ ] Soak test لمدة 7 أيام.
- [ ] قرار Release أو Rollback.

## خط الأساس والنتيجة النهائية

| المقياس | Baseline 2026-09-01 | بعد الإصلاح | حد القبول |
|---|---:|---:|---:|
| RAM | 35,308 B (43.1%) | 35,428 B (43.2%) | لا زيادة غير مفسرة |
| Flash | 355,103 B (34.0%) | 363,503 B (34.8%) | أقل من حد اللوحة بهامش OTA |
| ArabicShaper tests | 7/7 | 7/7 | 100% |
| Prayer/state tests | 0 | 3/3 TimeMath | 100% للحالات المعتمدة |
| أقل Free Heap على العتاد | غير مقاس | 39,968 B أثناء OTA؛ 40,984 B بلا عميل | لا اتجاه هابط |
| أعلى Heap fragmentation | غير مقاس | 4% بعد ضغط Wi-Fi/Reset | لا اتجاه تصاعدي مستمر |
| Reset/WDT أثناء Soak | غير مقاس | صفر غير مقصود حتى بدء الـSoak | صفر |

## سجل المخاطر المفتوحة

| ID | الخطورة | المشكلة | الحالة |
|---|---|---|---|
| R-01 | P0 | نصوص PROGMEM تستخدم أحيانًا كمؤشرات RAM | مغلقة؛ Boot ميداني ناجح بعد الإصلاح |
| R-02 | P0 | `rtc.now()` يمكن استدعاؤها قبل `rtc.begin()` | مغلقة برمجيًا؛ اختبار RTC مفقود ما زال مطلوبًا |
| R-03 | P0 | مدخلات HTTP بلا Range validation مركزي | مغلقة ومختبرة ميدانيًا |
| R-04 | P1 | Eqama cache لا يراقب جميع أوقات الإقامة | مغلقة |
| R-05 | P1 | نافذة الجمعة قد تنتهي قبل `sleepGomaaTime` | مغلقة واختبار math ناجح |
| R-06 | P1 | RTC لا يتعافى بعد فشل الإقلاع ولا تتحقق القراءة | مغلقة برمجيًا؛ فصل فيزيائي مؤجل |
| R-07 | P1 | OTA بلا Authentication وفحص حالات كامل | فحص الحالات مغلق؛ Authentication ينتظر عقد التطبيق |
| R-08 | P2 | لا توجد اختبارات لمنطق الصلاة/RTC/API | مخفضة: 10 Native + API/OTA ميداني؛ المرجع الخارجي مؤجل |
| R-09 | P2 | مقارنات deadline محدودة غير آمنة تمامًا عند rollover | مغلقة لمسارات card/restart والنوافذ اليومية |
| R-10 | P1 | RTC يحمل وقت 2022 بسبب سطر اختبار قديم كان فعالًا مع DEBUG | مغلقة؛ RTC مضبوط والكود المؤقت محذوف |

## قرارات مطلوبة لاحقًا من المالك

- [ ] اعتماد نصوص الأحداث والأذكار العربية في `src/texts.h`.
- [ ] اعتماد خط 7px الحالي أو استبداله بالرسم القديم.
- [ ] تحديد هل Authentication الجديد يجب أن يحافظ على توافق كامل مع إصدار
  تطبيق الهاتف الحالي.
- [ ] تحديد مرجع أوقات الصلاة المقبول للمقارنة الميدانية.

لا تمنع هذه القرارات بدء إصلاحات P0، لكنها تمنع اعتماد Release النهائي.

## سجل الجلسات

### 2026-09-01 — إعداد الخطة

- [x] مراجعة معمارية كاملة للمشروع والمكتبات.
- [x] بناء هدف ESP8266 بنجاح.
- [x] قياس RAM وFlash.
- [x] تشغيل اختبارات ArabicShaper: 7/7 ناجحة.
- [x] إنشاء خطة التنفيذ وسجل التقدم.
- [ ] بدء تعديل Firmware — ينتظر قول المالك: **ابدأ**.

### 2026-09-01 — تشخيص Boot loop على العتاد

- [x] اكتشاف اللوحة على `COM5` عبر CH340.
- [x] التقاط Serial بسرعة 115200: `Exception (3)`, `rst cause:2`.
- [x] فك Stack إلى `splashScreen()` ثم `setup()`.
- [x] مطابقة `excvaddr=0x4024d023` مع عنوان `SPLASH_TITLE` في ملف ELF.
- [x] استبعاد WDT وهبوط الجهد كسبب مباشر لهذا الـBoot loop.
- [x] إلغاء سحب Flash backup بطلب المالك؛ نسخة رجوع قديمة متاحة لديه.
- [x] المالك قال **ابدأ**؛ تم الإصلاح والرفع والاختبار المبدئي.

### 2026-09-01 — إصلاح Boot loop والرفع على العتاد

- [x] نسخ `SPLASH_TITLE` من PROGMEM إلى RAM قبل استدعاء DMDESP.
- [x] تصحيح بقية نصوص الصلاة والأذكار و`RTC_FAIL_TEXT` المستخدمة من PROGMEM.
- [x] استبدال format-string flash calls بـ`snprintf_P()`.
- [x] منع `effectiveBrightness()` من قراءة RTC مباشرة قبل تهيئته.
- [x] Build ناجح: RAM ‏35,344 B (43.1%)، Flash ‏357,803 B (34.3%).
- [x] اختبارات Native: ‏7/7 ناجحة.
- [x] Upload ناجح إلى `COM5` مع تحقق Hash كامل.
- [x] سجل الإقلاع بعد الرفع: `Reset: External System` ثم `Web server started`،
      `RTC OK`، `adjustTime`، `Setup done`.
- [x] لا `Exception (3)` ولا إعادة إقلاع خلال فترة المراقبة الأولية (~60 ثانية).
- [~] التحقق البصري من تحديث شاشة P10 مطلوب قبل اعتماد تعديل مؤقت DMDESP.

### 2026-09-01 — دفعة الاعتمادية الثانية

- [x] تعطيل `rtc.adjust()` التجريبي الذي كان يكتب تاريخ 2022 عند كل إقلاع تشخيصي.
- [x] إضافة parsing صارم وحدود رقمية وتطبيق ذري لكل إعدادات HTTP.
- [x] رفض رسالة/اسم مقطوع أو UTF-8 غير صالح بدل تخزين قيمة جزئية.
- [x] إصلاح Eqama cache، نافذة الجمعة، وعبور نوافذ الصلاة منتصف الليل.
- [x] إضافة فحص قراءة RTC وتعافٍ دوري غير حاجب كل 30 ثانية.
- [x] منع `EEPROM.commit()` عند تطابق الإعدادات مع المخزن الحالي.
- [x] إزالة تحذيرات قطع نص رمضان/التاريخ برفع `fmtBuf` من 32 إلى 48 بايت.
- [x] Build ناجح: RAM ‏35,360 B (43.2%)، Flash ‏360,567 B (34.5%).
- [x] اختبارات ArabicShaper: ‏7/7 ناجحة.
- [x] Upload ثانٍ ناجح إلى `COM5` مع Hash verified.
- [x] Boot بلا Exception/Restart، ومحاولتا RTC retry عند 30 و60 ثانية بلا WDT.
- [!] الـRTC موجود لكنه يحمل 2022؛ الفيرموير رفضه بأمان. ينتظر ضبط الوقت مرة
      واحدة من تطبيق الهاتف لاستكمال اختبارات الصلاة والتاريخ.

### 2026-09-01 — استعادة Refresh شاشة P10

- [x] المالك أبلغ أن الصفوف تُرسم من أعلى لأسفل كأن Refresh rate منخفض.
- [x] ربط العرض مباشرة بحذف سطري `system_timer_reinit()` التجريبي من DMDESP.
- [x] استعادة السطرين الأصليين؛ DMDESP عاد بلا اختلاف وظيفي عن نسخة المستودع.
- [x] Build ناجح: RAM ‏35,360 B (43.2%)، Flash ‏360,583 B (34.5%).
- [x] Upload ناجح إلى `COM5` وHash verified.
- [x] مراقبة Serial قرابة 60 ثانية بلا Exception أو Restart بعد الاستعادة.
- [x] المالك أكد بصريًا أن الصورة والحركة ممتازتان.

### 2026-09-01 — النسخة النهائية واختبارات الضغط

- [x] ضبط DS3231 مرة واحدة من وقت البناء مع مراعاة DST، ثم حذف كود الضبط المؤقت.
- [x] إضافة CRC32 متوافق مع EEPROM الحالي ومنع الكتابة عند عدم تغير البايتات.
- [x] إضافة AP recovery، software-date rollover، وHeap/loop telemetry.
- [x] تقوية OTA ومعالجة الفشل/الإلغاء/النجاح دون ترك الشاشة متوقفة.
- [x] Build نهائي: RAM ‏35,428 B (43.2%)، Flash ‏363,503 B (34.8%).
- [x] Native tests: ‏10/10 ناجحة (7 shaping + 3 time/calendar).
- [x] API: `/getData` ‏24 حقلًا؛ رفض 999 وblur معيب وتاريخ غير صالح.
- [x] OTA خاطئ: `UpdateError`, لا Restart؛ OTA صحيح: `UpdateOk` وإقلاع سليم.
- [x] ‏20/20 Hardware resets ناجحة، والإقلاع الأخير `RTC OK` / `Setup done`.
- [x] Free Heap تحت الحمل 39,968–41,192 B، fragmentation ‏1–4%.
- [x] إزالة Wi-Fi profile المؤقت بعد الاختبار.
- [~] Serial soak logger يعمل في `logs/device-monitor-260901-183011.log`.
- [~] متابعة Codex كل ساعة حتى 24 ساعة ثم 7 أيام؛ الإشعارات للفشل فقط.

### 2026-09-01 19:33 — نقطة متابعة Soak بعد ساعة

- [x] مدة التشغيل المتصل منذ بدء الـSoak: نحو ساعة و3 دقائق.
- [x] الـSerial logger ما زال يعمل على `COM5`؛ 64 عينة دورية في السجل.
- [x] لا `Reset` أو `Exception` أو `WDT` في سجل الـSoak.
- [x] أقل Free Heap: ‏40,984 B؛ أقل Max Free Block: ‏39,544 B.
- [x] أعلى Heap fragmentation: ‏4%؛ أعلى `loopMax`: ‏11,727 us.
- [~] النتيجة مستقرة بلا اتجاه هابط للذاكرة؛ تستمر المتابعة حتى 24 ساعة ثم 7 أيام.

### 2026-09-01 22:48 — إيقاف الـSoak والتركيز على الخط

- [x] شُخّص انطفاء الشاشة: `brightness=1` مع Power Save كان ينتج سطوعًا صفريًا بسبب القسمة الصحيحة.
- [x] أثبت الاختبار الميداني السبب؛ تغيير السطوع إلى `2` أعاد الشاشة فورًا.
- [x] أضيف إصلاح دائم في المصدر يمنع تحويل `0` أو `1` إلى قيمة أخرى؛ Build ESP8266 ناجح بنفس RAM/Flash.
- [ ] الإصلاح الدائم لم يُرفع ولم تبدأ اختباراته بطلب المالك تأجيل الاختبار.
- [x] أوقف Serial logger يدويًا؛ لا يوجد اختبار Soak نشط الآن.
- [~] أولوية العمل التالية: الخط، ثم تطبيق Android، ثم استئناف الاختبار من الصفر.

### 2026-09-02 — نسخة تجربة اختيار الخط

- [x] تصحيح مولّد TTF: كان يضع صندوق حبر 20px داخل Bitmap ارتفاعه 16px ويقص أسفل الحروف.
- [x] إضافة اختيار مؤقت في RAM لثلاث عائلات عبر `/setFont` دون تغيير `EEPROM_VERSION`.
- [x] الخيارات التجريبية: الخط الحالي، Traditional المصحح، وTahoma؛ الحجم الصغير/الكبير مستقل.
- [x] Build ESP8266 ناجح: RAM ‏35,452 B (43.3%)، Flash ‏372,183 B (35.6%).
- [x] Upload ناجح إلى `COM5` وHash verified؛ الإقلاع وصل `RTC OK` و`Setup done` بلا Exception.
- [ ] Native/Soak مؤجلان بقرار المالك خلال مرحلة تجارب الخط.

### 2026-09-02 — أربع عائلات خط وBasitFont

- [x] قياس حدود Traditional أثبت أن همزة الألف تفرض الحجم 12؛ استبدال الألف
      المزخرفة بألف عادية سمح بالحجم 13 وصندوق حبر 15px بلا قص سفلي.
- [x] نقل Janat7/Janat16 القديمين إلى BasitFont7/BasitFont16 مع خريطة موثقة
      في `tools/legacy_janat_map.json` واستكمال الأشكال من أقرب رسمة Janat.
- [x] إضافة Tahoma وعائلة Arial Bold ثقيلة؛ نسخة 7px للبولد مولدة من Tahoma
      Bold لأن Arial Bold لا يرسم جميع الخانات المطلوبة بهذا الحجم.
- [x] توسيع `/setFont` مؤقتًا إلى `0..3` دون EEPROM أو رفع رقم الإصدار.
- [x] Build ESP8266 ناجح: RAM ‏35,452 B (43.3%)، Flash ‏377,063 B (36.1%).
- [x] Upload ناجح إلى `COM5` وHash verified.
- [~] ينتظر اعتماد الشكل بصريًا على P10؛ Native/Soak ما زالا مؤجلين.

### 2026-09-02 — توافق تطبيق Android ووصل الحروف

- [x] إثبات أن HTTP والربط بالشبكة سليمَان؛ `/getData` أعاد 200 وبيانات صحيحة.
- [x] تشخيص `setDateTime` ‏Error: التطبيق يرسل الأرقام خامًا بينما الفيرموير
      كان يحاول فك كل قيمة رقمية كـblur.
- [x] قبول الأرقام الخام أولًا مع fallback للقيم المشفرة، دون إضعاف الحدود.
- [x] إلغاء Traditional واعتماد BasitFont وTahoma وArial Bold فقط.
- [x] إزالة أعمدة الـside-bearing الفارغة من حواف الوصل فقط في الخطوط المولدة.
- [x] Build ناجح: RAM ‏35,452 B (43.3%)، Flash ‏373,383 B (35.7%).
- [x] Upload ناجح إلى `COM5` مع Hash verified وإعادة تشغيل عتادية.

### 2026-09-02 — تثبيت خطة المنتج التجاري v2

- [x] اعتماد HMAC-SHA256 بمفتاح 32 بايت فريد لكل جهاز وnonce مؤقت في RAM.
- [x] اعتماد QR pairing وحفظ المفتاح في Android Keystore.
- [x] فصل مفتاح HMAC عن كلمة Wi-Fi القابلة للتغيير.
- [x] رفض JSON واعتماد UTF-8 `key=value` line protocol صغير.
- [x] اعتماد Build Profiles ‏1x1 و2x1 و3x1 و4x1 وإلغاء 4x2.
- [x] توثيق الترحيل وOTA والاختبارات والتسليم بالكامل في
      `docs/COMMERCIAL_V2_PLAN.md`.
- [x] إلغاء QR الثابت لصعوبة الوصول إليه؛ اعتماد Pairing Code مؤقت يظهر على
      P10 وكلمة إدارة يشتق منها Android المفتاح ويحفظه في Keystore.
- [x] تبسيط إضافي بقرار المالك: إلغاء Pairing Code وكلمة الإدارة المنفصلة؛
      كلمة Wi-Fi واحدة تتغير إجبارياً أول مرة وتشتق منها HMAC، وزر Hardware
      Reset لمدة 10 ثوانٍ يعيد إعدادات المصنع عند النسيان.

### 2026-09-02 — تنفيذ Commercial v2 الأساسي

- [x] Build Profiles ‏1x1/2x1/3x1/4x1؛ الأربعة نجحوا.
- [x] EEPROM v4: كلمة AP، فرض تغييرها أول مرة، وحفظ اختيار الخط.
- [x] PBKDF2-HMAC-SHA256 (4096) + HMAC-SHA256/128 + nonce أحادي الاستخدام.
- [x] API v2 نصي بدون JSON، مع أداة اختبار `tools/api_v2_client.py`.
- [x] زر Factory Reset غير حاجب على GPIO2: ضغط 10 ثوانٍ بعد الإقلاع.
- [x] Native ‏10/10 وHMAC self-test ناجحان.
- [x] رفع 4x1 إلى COM5؛ Hash verified؛ الإقلاع وصل `Setup done` بلا WDT.
- [x] اختبار حي: `/v2/getData` ‏200 وتوقيع الرد صحيح؛ منع الإعدادات قبل تغيير
      كلمة المصنع أعاد 403 موقعاً كما هو مطلوب.
- [ ] اختبار تغيير كلمة Wi-Fi وزر GPIO2 يحتاج وجود المالك لتجنب فقد الاتصال.
- [ ] Signed OTA وAndroid خارج هذه الدفعة.

### 2026-09-02 — تصحيح تخطيط البطاقات الصغيرة

- [x] السطر العلوي: ساعة يساراً، ثم اليوم/الميلادي/الهجري/«الهاتف صامت» يميناً
      كل 5 ثوانٍ.
- [x] السطر السفلي: ترتيب منطقي Fajr إلى Isha؛ RTL يجعل الفجر يميناً والعشاء
      يساراً.
- [x] Build/Upload تصحيح التخطيط إلى COM5؛ RAM ‏45.4% وFlash ‏36.6%.
- [ ] اختبار بصري ميداني بعد الرفع.

### 2026-09-02 — رفع تصحيح تخطيط 4x1

- [x] Build/Upload ناجح إلى COM5؛ RAM ‏45.3% وFlash ‏36.6%.
- [x] إزالة تحذير UTF-8 في ساعة السطر العلوي باستخدام `HH:MM:SS` ASCII.
- [ ] تأكيد المالك بصرياً أن الفجر أصبح أقصى اليمين والعشاء أقصى اليسار، وأن
      بطاقات اليمين أصبحت مقروءة بالحجم الصحيح.

### 2026-09-02 — مراجعة مسارات النص

- [x] تشكيل بطاقة أعلى اليمين وشاشة البداية عبر `ArabicShaper`؛ كل النصوص
      العربية الأخرى تمر عبر `SmartText`.
- [x] رفع النسخة الأخيرة إلى COM5؛ Build ناجح بلا أخطاء.
- [x] إصلاح قص الصف السفلي في أرقام السطر العلوي: `y=-1` إلى `y=0`.
- [x] BasitFont7/16: استخدام Tahoma fallback للهمزة وأشكال القاف والألف
      المقصورة النهائية الناقصة؛ fallback=7 glyphs، ثم Build/Upload ناجح.

### 2026-09-02 — توحيد التشكيل والخط القياسي

- [x] مراجعة مخزون الحروف السياقية بالكامل؛ اتضح أن بعض خانات Janat القديمة
      كانت تربط الحرف النهائي بصورة وسطية (خصوصًا ف).
- [x] استعادة رسومات Janat الأصلية في BasitFont7/16 مع خريطة منفصلة لكل شكل
      ISO/INI/MED/FIN؛ Tahoma يُستخدم فقط للخانات السبعة غير الموجودة.
- [x] فحص بصري مولّد للارتفاعين 7 و16؛ لا رسم يتجاوز حدود الـbitmap.
- [x] Build ناجح: RAM ‏37,156 B (45.4%)، Flash ‏382,675 B (36.6%).
- [x] Upload ناجح إلى COM5 مع تحقق Hash كامل وإعادة تشغيل عبر RTS.
- [!] Native tests محجوبة محليًا لعدم وجود GCC/G++ في PATH؛ ليست أخطاء ترجمة
      في المشروع.

### 2026-09-05 — مرجع رسم الحروف اليدوي

- [x] إضافة `docs/BASIT_GLYPH_DRAWING_MAP.md` يربط كل خانة `0x80..0xFE`
      بالحرف العربي وشكله السياقي لاستخدامه داخل GLCDFontCreator2.
- [x] تعديل `GLCDFontCreator2/gui/CharTable.java` لإظهار الحرف العربي وشكل
      الخانة بجانب Dec/Hex داخل واجهة الرسم؛ تجميع Java ناجح.
- [x] إنشاء ملفات قابلة للفتح مباشرة في البرنامج: `GLCDFontCreator2/BasitFont7.ftsd`
      و`GLCDFontCreator2/BasitFont16.ftsd`، مع نسخ Tahoma بنفس الأسماء
      الحروف والأشكال داخل كل خانة.
- [x] تعريب حالة الخانة داخل التعليق إلى «منفصل/أول/وسط/نهائي» وإعادة تصدير
      الملفين بترميز UTF-8.
- [x] إعادة توليد ملفات التحرير الأربعة من الخطوط المشحونة فعليًا
      المستبدلة داخل `GLCDFontCreator2`؛ النتيجة 127 خانة لكل ارتفاع.
- [x] استبدال خريطة Unicode الوسيطة بمحول مباشر يقرأ `ArabicShaping.java`
      وينسخ 127/127 رسمة من خانات Janat الأصلية حسب الحالة؛ بدون Tahoma أو
      fallback، مع تصحيح تعليقات الخانات حتى `0xFE`.

## قالب تسجيل كل دفعة لاحقة

انسخ هذا القسم عند كل دفعة عمل:

```text
التاريخ/الوقت:
المرحلة:
التغييرات:
الملفات:
Build:
Native tests:
Hardware test:
RAM/Flash:
Free Heap/Fragmentation:
Serial/Reset evidence:
النتيجة: PASS / FAIL / BLOCKED
الخطوة التالية:
```

## 2026-09-05 - Commercial OTA gate

- [x] Added `/v2/update` with API-v2 HMAC authorization and profile/size
      validation.
- [x] Enabled ESP8266 BearSSL ECDSA verification through `Updater` before
      accepting the image.
- [x] Added `tools/sign_ota.ps1`; private signing key is outside the repo.
- [x] P10 4x1 build passes: RAM 46.1%, Flash 41.9%.
- [ ] Android update screen migration to `/v2/update` and customer field test.

## 2026-09-05 - Password-change disconnect handling

- [x] Increased the post-password-change restart grace period to 5 seconds.
- [x] Flushes the authenticated response before restarting the ESP8266 AP.
- [x] Android treats an EOF during `setNetwork` as a committed-restart state,
      stores the new derived key, and asks the user to reconnect.
- [ ] Upload this firmware and test the first-password-change flow on hardware.

## 2026-09-05 - Simplified customer password flow

- [x] Removed the mandatory first-login password-change gate.
- [x] Password change remains one button and changes only the AP password.
- [x] Hardware factory reset remains the recovery path and restores the default AP password.
- [ ] Upload and verify the simplified flow on the ESP8266.

## 2026-09-05 - AP-only control decision

- [x] Removed the application password prompt from the P10 control flow.
- [x] API-v2 keeps its compact POST format but no longer requires HMAC headers;
      access is limited by the clock AP password.
- [x] Password change is available only from Settings; factory reset restores
      the default AP password.
- [ ] Upload firmware and test the Android flow after reconnecting to the AP.

## 2026-09-05 - Final AP-only simplification

- [x] Removed the application password prompt entirely.
- [x] v2 commands use the compact POST protocol without HMAC enforcement.
- [x] Signed OTA still verifies the firmware signature, but no longer needs an
      application key in Android.
- [ ] Upload the new firmware and install the updated Android sources.

## 2026-09-05 - Transient HTTP EOF handling

- [x] ESP8266 flushes every v2 response before returning.
- [x] Android retries one ordinary v2 command once after a transient EOF/reset.
- [x] Password-change requests are never replayed; an interrupted restart is
      handled as a pending network change.
- [ ] Verify first launch and settings saves on hardware.

## 2026-09-05 - Android getData loop fix

- [x] Stopped repeated `getData -> getDisplaySettings` probing after modern mode
      is detected.
- [x] Added guards so the display profile handshake runs only once per session.
- [ ] Re-run the app and confirm one initial `getData` followed by normal updates.

## 2026-09-05 - Time format UI

- [x] Moved 12/24-hour selection from Settings to the Time screen.
- [x] Replaced the oval action button with an inline RTL radio-card (12/24).
- [x] Selection updates the clock immediately and remains hidden until the
      device capability probe succeeds.

## 2026-09-05 - Settings and technician controls

- [x] Added visible AP-password change action under Settings > Network.
- [x] Added confirmation before AP-password changes, test mode, and factory reset.
- [x] Test/factory-reset tools remain hidden and unlock only after seven taps on
      the Settings title for the current session.
- [x] Existing save-result feedback remains active through the normal `*Ok`
      response handling.
- [x] AP-password action now uses a distinct outlined rectangular style instead
      of the circular button style used by technician actions.

## 2026-09-05 - Firmware audit after UI update

- [x] No blocking `delay()` was found in the runtime loop; `dmd.loop()`,
      `server.handleClient()`, and `yield()` remain in the main loop.
- [x] EEPROM writes are centralized in `saveConfig()` and skipped when the
      configuration is unchanged.
- [x] The remaining dynamic `String` allocations are limited to HTTP request /
      OTA parsing, not the display loop.
- [x] Regenerated the Android P10 OTA asset and manifest from the signed
      release image; size and unsigned-image hash now match.
- [x] Added release profiles with `DEBUG=0` and removed the unused
      HMAC/challenge compatibility code after app migration.
- [x] Small-font top layout now adapts to 1x1/2x1/3x1/4x1 without drawing the
      clock or information card outside the panel bounds.
- [x] PlatformIO build passed for all four P10 profiles after the layout fix;
      RAM remains 45.9–46.0% and Flash 41.7%.
- [x] Android AP-password action now includes a small Wi-Fi icon; XML parsed
      successfully (Android Studio should perform the final resource build).
- [x] Removed the unused HMAC/PBKDF2/nonce challenge path from firmware and
      Android; signed OTA verification remains enabled.
- [x] Added separate `*_release` PlatformIO profiles with `DEBUG=0`.
- [x] Rebuilt and signed the 4x1 release image; Android OTA asset and manifest
      now match the same signed image (`size=434811`).

## 2026-09-28 - Stable OTA policy restored

- [x] Confirmed the Android customer flow still uploads the raw firmware to
      `/updateMain`.
- [x] Kept legacy OTA as the sole commercial path for old and new devices;
      `/v2/update` is engineering-only and no migration is required.
- [x] Documented the rule that signed trailers must never be appended to the
      raw `/updateMain` asset.
- [x] Simplified `tools/release_ota.ps1` to build and copy the raw commercial
      image only; it no longer requires or generates a signing key.
- [x] Rebuilt the 4x1 release successfully: RAM 37,136 B (45.3%), Flash
      430,583 B (41.2%); Android asset copied at 434,736 bytes.
- [ ] Native tests remain environment-blocked because `gcc`/`g++` are not
      installed on this Windows machine.
- [x] Added display double-buffer presentation to prevent clear-then-draw
      tearing during text refresh; 4x1 Release build passes.
- [~] 4x1 Release with double buffering uploaded to COM5 successfully; visual
      flicker confirmation is still pending because the release build has
      `DEBUG=0` and emits no serial boot log.
- [x] Release profiles 1x1, 2x1, 3x1, and 4x1 all compile successfully with
      the same display path; no profile-specific build regression found.
- [x] Added dirty-frame tracking so unchanged loops skip the framebuffer copy;
      marked the DMDESP timer flag `volatile` for timer/main-loop correctness.
- [x] Rebuilt all three non-4x1 Release profiles after the change successfully.
- [x] Updated Android raw P104 asset and uploaded the 4x1 Release to COM5;
      flash hash verification passed.
- [x] Added persistent card enable/order settings and Android editor; release
      rebuilt and uploaded to COM5 (MAC 24:4c:ab:4f:ed:96), hash verified.
## 2026-09-30 - OTA status on the panel

- [x] Shared legacy/v2 OTA callbacks show actual received bytes as a percentage
      when an expected file size is available. Multipart Content-Length is not
      used because it includes framing bytes.
- [x] Upload callbacks service `dmd.loop()` even when a percentage frame is
      skipped; double-buffer presentation is reused without extra allocation.
- [x] 100% is reserved for successful `Update.end(true)`; success remains
      visible during the existing five-second restart delay. Narrow panels
      use two lines for the success message.
- [x] Missing size preserves legacy compatibility (`...` during transfer);
      invalid size, oversize data, or mismatched final size rejects the upload.
- [x] Failure/cancellation restores the configured font and normal cards.
- [x] 4x1 Release build passed: RAM 37,544 bytes (45.8%), Flash 435,463 bytes
      (41.7%). Raw binary (439,616 bytes) copied to the correct Android
      project's P104 asset after the timer-refresh fix.
- [ ] Owner handles physical testing. First installation is handled by the
      currently running firmware; the new display behavior starts with the
      following OTA. No EEPROM version change, hardware upload, or APK build.
- [x] DMD refresh now runs from the existing 100-us timer during blocking
      multipart parsing/flash writes, preventing OTA scan starvation.
- [x] Replaced changing percentage text with a static OTA label and a compact
      filled bar; the bar is throttled and the verified-success screen is
      unchanged.
- [x] Replaced the on-panel progress bar with an intentional DMD pause during
      multipart transfer after hardware testing still showed network/display
      contention. Android remains the progress surface; DMD resumes only for
      the final result and restart.
