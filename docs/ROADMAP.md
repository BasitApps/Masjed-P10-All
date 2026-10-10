# ROADMAP — Arabic UTF-8 Support Project

> Status markers: `[ ]` pending · `[~]` in progress · `[x]` done
> Keep the "Current step" line accurate at all times — it is the first thing
> any new session reads.

**Current step:** 2026-10-08: both firmware projects now share identical
`CivilTimeContract.h` for RTC decoding, calendar validation, supported years,
12/24-hour conversion, DST and Hijri. Both also share compact POST extraction,
numeric conversion and calculation/settings validation; current host checks,
all eight P10 builds and both 7-segment builds pass. Android was unchanged in
this batch; its prior Java compilation passed.
Physical tests are deferred until the owner returns to the computer.

**Previous step:** 2026-10-08: P10 now distinguishes RTC disconnect (`RTC E1`)
from invalid RTC time (`اضبط الوقت` in Arabic, `TIME E2` in English) while stopping cards; 7-segment keeps its
slot-specific `E1/E2/E4` diagnostics. P10 and 7-segment now share the technician-only
`POST /resetDefault` contract. The Android button is visible in technician mode
for both profiles, locks during the restart window, and both firmware families
build successfully. The 7-segment reset path verifies format/save/readback before
acknowledging; P10 flushes the acknowledgement before its delayed restart.
The remaining work is physical release validation and long-duration soak, not a
new API variant. Previous review evidence follows.

The 7-segment diagnostic display now distinguishes RTC/I2C disconnect (`E1`),
invalid RTC time (`E2` plus blinking zero time), and missing temperature sensor
(`E4`) using active slots only; prayer and iqama slots are preserved. The source
harness passes 25/25 and ta73/ta73s release builds pass.

Previous current step: 2026-10-07: P10 comprehensive review batches 1–3 are
implemented. Native production-path checks pass 32/32, multipart checks pass
11/11, display checks 6/6, text scroll checks 0 failures, and release-tool
checks 4/4. Four release profiles build successfully. The local parser overlay
adds bounded deadlines, argument/field limits, stale-plain reset and checked
allocations without modifying `lib/` or the installed SDK. P10 4×1 release was
uploaded on COM5; esptool verified the flash hash and reset the board. Initial
API/RTC checks passed after native WlanConnect. One release OTA reset early
(Power On; RTC then invalid); one monitored DEBUG OTA passed with UpdateOk
and normal Software/System restart. Release was restored with flash hash
verification. Owner confirmed RTC battery was removed; lost-power/invalid time
after RTC supply interruption is expected. Owner then fitted the battery;
per-chunk OTA yield replaced watchdog-only feeding. Three release OTA attempts
without UDP peer passed, with valid RTC and unchanged settings after normal
reboots; latest398048B candidate is on the board. Long/intermittent-fault proof
remains unresolved: a follow-up OTA of a temporary version13 candidate with UDP
peer reset at progress1 (ROM rst cause:1 / health Power On), leaving rc4/12 on
the board and valid RTC/settings. Version changes were reverted; publication
was withheld pending diagnosis. Subsequent same-old-Android-BIN OTA without
opening COM5 failed at1% with Hardware Watchdog, not Power On. Reviewed12/398048B
restored via USB; network-only OTA of it passed with UDP0..99 and UpdateOk,
normal software reboot, valid RTC/time and unchanged settings. Android asset
and manifest now match that reviewed BIN for owner tests; owner's forced map13
remains, but actual BIN version12 is NOT final release13. No APK built/installed;
rerun Android Studio before phone testing. Intermittent-root proof, final version,
7-segment and 24/7 soak still pending. The first 7-segment correction batch is now
implemented: raw RTC/OSF/BCD/calendar validation, verified time/config rollback,
matching base-date DST offset, nonfinite prayer dashes, Friday azan and single
date toggle per sampled second. External real-source checks24/24 and existing
checks3/3 pass; ta73/ta73s builds SUCCESS24.063s. No assets/version/upload changes.
Next bounded batch: remaining 7-segment API readers and settings failure paths,
then physical validation of its own board before publication. P10 code-review
batches complete does NOT mean the whole
review is closed. The 7-segment ta73 candidate was uploaded to the owner’s
M73 board and returned UpdateOk/getData after reboot; M73s/P10 assets remain
untouched. The upload proves this image boots on that board, not long-term
power/reset reliability.
hardware/release plan is closed. Older closed plan stays closed.

Previous current step: 2026-10-03: Development test harness removed after the owner
accepted the manual verification workflow; only firmware builds and hardware
smoke tests remain. API cleanup completed. The commercial migration
contract is now the root-path compact POST API plus `GET /checkUpdate` and
`POST /updateMain`; unused legacy GET handlers and the retired signed `/v2/update`
implementation were removed from both firmware paths. The Android assets were
regenerated from the verified release builds. Hardware smoke testing of one P10
and one 7-segment unit remains before publishing this cleanup.

Previous current step: 2026-10-02: Owner declared the comprehensive firmware plan
complete after the 24-hour manual soak. The plan and its progress log are now
closed; later visual/manual findings should be opened as new work rather than
reopening the completed plan. The plan is
`docs/superpowers/plans/2026-10-01-firmware-comprehensive-review.md`.
R0/R1 documentation is complete/partial and R2 has its first calculation fix:
PrayerTimes method parameters are now explicit, with Makkah Isha selectable
between the compatibility method rule and fixed 90 minutes. The firmware-only
field is documented for later Android work in `docs/ANDROID_DEFERRED_PLAN.md`.
R4 also normalizes the Iqama countdown across midnight. Native tests are 26/26
and all four release profiles build; independent numeric reference and owner
hardware checks remain. Prayer and Hijri caches now key on the complete
Gregorian date to avoid stale results at month/year rollover, and short loop
stalls now catch up bounded software-second ticks.
Prayer-window boundary predicates are now centralized in a small tested phase
classifier without changing the existing Friday/eqama behavior.
The seasonal clock rule is now shared by P10 and 7-segment (last Friday of
April through last Thursday of October, inclusive), while each profile keeps
its legacy configuration field name.
Malformed/truncated UTF-8 input is now bounded in the shaper and covered by a
regression test; valid font output remains unchanged.
`firmware-rc1` remains the rollback reference; its four-profile SHA-256 manifest
is stored under `release/firmware-rc1/`. Commercial release is not yet authorized.

The P10/7-segment compatibility matrix is recorded in
`docs/SEVEN_SEGMENT_COMPATIBILITY.md`. PrayerTimes event indices and the
explicit method table (including Makkah method-rule/fixed-90 selection) now
match in both firmware paths; P10 correction is in the current branch and the
7-segment correction is commit `8996bab`. Fixed-date, DST/RTC, and hardware
checks remain before release.

The separate 7-segment path now uses the same tested prayer-phase boundaries,
cross-midnight minute helpers, and Gregorian-to-Hijri conversion as P10;
`ta73`/`ta73s` builds pass. The source still contains owner working-tree
changes and no firmware upload was performed.
The 7-segment path now also exposes the compact v2 control contract used by P10
(`getData`, date/time, iqama, location, settings, password, test, reset). The
Android app selects v2 after the mandatory firmware update for both profiles;
M73/M73s update targets are version 6 and their OTA assets were regenerated.
An edge-case test also fixed the shared contract so iqama `0` disables all
post-azan phases instead of entering sleep when a sleep duration is configured.
Live AP verification initially reported `rtc=0`, but the legacy encoded
`/setDateTime` request recovered the I2C/RTC path without a power cycle.
Follow-up health reported `rtc=1` and a valid sync age; physical RTC
disconnect/reconnect testing remains for the owner.

The release OTA verifier now handles BearSSL's 64-byte parsed EC point by
adding the SEC1 prefix only for verification; the signing tool emits raw
P-256 `r||s` signatures. All four release profiles build and native tests are
32/32 green. Signed OTA was verified live on 4x1 by successful flash write,
reboot, and post-reboot `health` (`rtc=1`). The multipart client still sees a
timeout/reset instead of the success body, so Android should treat a closed
OTA socket as pending and confirm with post-reboot health before reporting
failure; this is the remaining OTA follow-up.

Previous step: 2026-09-30: card text review. Corrected Hijri date logical
order for the RTL shaper in both the rotating small-font info line and the
Hijri card used on narrow panels, and standardized the large Gregorian card
to `YYYY-MM-DD`. The Basit hamza glyph was redrawn for both 7px and 16px
families (including Indic digits). In 12-hour mode the 64px clock zone now
shows a shaped Arabic `ص/م` marker to the left of the time (or `AM/PM` in
English); it is omitted on
narrow panels to prevent clipping. The top Hijri line tries `day month year`
and falls back to a shorter month form only when its measured width requires
it. Release builds pass for 1x1, 2x1, 3x1, and 4x1; native tests pass 19/19
when the bundled GCC path is supplied. Text caches now invalidate when the
display language changes.
Long Hijri month names still use a compact top-line fallback so they cannot
overlap the clock zone; full names remain in scrolling cards.
Owner-confirmed 24-hour reliability soak is complete; no additional numeric
telemetry was supplied, so the ledger records completion without inventing
uptime or heap measurements.

**Commercial API:** المسار المعتمد هو POST compact root paths (`/getData`,
`/set...`) مع `GET /checkUpdate` و`POST /updateMain`. الأجهزة القديمة تُرحّل
إجباريًا من التطبيق قبل استخدام الإعدادات الجديدة؛ لا توجد مسارات GET قديمة
للتشغيل التجاري.

## Phase 0 — Cleanup & Continuity Infrastructure
- [x] Delete `lib/RTClib - Copy/` and `tmp_dst_check.py`
- [x] Update `.gitignore` (`__pycache__/`, `*.pyc`, `tools/out/`)
- [x] Add `.clang-format`
- [x] `git init` + baseline commit + tag `v2-baseline`
- [x] Handoff docs: AGENTS.md, docs/ROADMAP.md, docs/DECISIONS.md, docs/FONT_SPEC.md
- [ ] Branch `feature/arabic-support`

## Phase 1 — Font Tooling (`tools/gen_font.py`)
- [x] Parser for existing GLCD font headers (Janat7.h / Janat16.h) → extract glyph bitmaps per ASCII slot
- [x] Mapping table: legacy ASCII slot ↔ Arabic glyph form identity (see FONT_SPEC)
- [x] Mode `extract`: rebuild extended fonts from existing Janat artwork (works; awaits owner mapping JSON)
- [x] Mode `ttf`: render glyphs from an Arabic TTF via Pillow (methods: `direct` hinted / `supersample`)
- [x] Mode `preview`: render sample Arabic strings to PNG using generated font data
- [x] Outputs: `lib/DMDESP/fonts/BasitFont7.h` + `BasitFont16.h` and
      `Tahoma7.h` + `Tahoma16.h` (GLCD format, first=0x20 count=0xE0)
  - generated from `C:/Windows/Fonts/trado.ttf` (Traditional Arabic) via `direct` method
  - `src/arabic_glyph_map.h` emitted alongside (Unicode presentation form → slot)
- [x] Reference shaper in Python verified: lam-alef, form selection, RTL word
  order, digits kept LTR inside Arabic lines
- [x] Fix TTF size probing: reject ink boxes taller than the target bitmap.
  The previous 16px candidate selected a 20px ink box and clipped descenders;
  corrected preview: `tools/out/preview_trado16_fixed.png` (not yet shipped)
- [ ] **STOP: owner visual review of previews before continuing**
  - see `tools/out/preview_trado16_direct.png` (GOOD), `preview_trado7_direct.png` (weak — 7px Arabic from TTF is inherently rough)
  - decision needed: 7px strategy → (a) extract owner's Janat7 art via mapping JSON, (b) accept TTF 7px, (c) drop small-font mode
  - alternative fonts ready to try: arial.ttf, tahoma.ttf, segoeui.ttf, majalla.ttf, simpbdo.ttf (all cover the 127 forms)

## Phase 2 — Runtime Shaper (`src/ArabicShaper.{h,cpp}`) — DONE
- [x] UTF-8 decoder
- [x] Joining classes (DUAL / RIGHT / NONE) + transparent codepoints
- [x] Form selection (isolated/initial/medial/final), lam-alef ligatures
- [x] Visual reorder pass (reverse RTL runs; keep digit/punct runs LTR)
- [x] Unicode presentation-form → font-slot lookup (PROGMEM, generated by gen_font.py)

## Phase 3 — Native Tests (archived) — DONE before harness removal

The temporary native harness and its test files were removed on 2026-10-03
after manual firmware verification. The historical results below are retained
only as project history; the supported workflow is firmware build plus hardware
smoke testing.
- [x] `[env:native]` with `toolchain-gccmingw32` (PATH needs
      `~/.platformio/packages/toolchain-gccmingw32/bin` prepended on Windows)
- [x] `test/test_shaper/`: golden vectors generated from Python reference
- [x] Parity check vs Python reference implementation

## Phase 4 — Firmware Integration — DONE (build SUCCESS: RAM 43.1%, Flash 34.0%)
- [x] Rewrite `src/texts.h` with real Arabic UTF-8 (**wording = best-effort
      draft; OWNER MUST REVIEW event/azkar strings**)
- [x] `config.language` field (0=ar, 1=en) + English day/month/prayer-name strings
- [x] English fixed-text coverage for prayer events, Iqama, azkar, calendar events,
      temperature, RTC and OTA messages; English scrolling runs right-to-left
- [x] `EEPROM_VERSION 4`, message 240 / masjedName 100,
      `BUF_LARGE`/`BUF_TEXT` 256, static blur buffers in `handleGetData`
- [x] Wire shaper into `SmartText` (`setShaping`, srcCache + shaped buffer)
- [x] `applySettings()`: BasitFont7/16 or Tahoma7/16 + shaping toggle by language
- [x] API: `lang` param in `/setSettings` (expose in `/getData` pending:
      language not yet in the CSV — app reads settings back? add if needed)
- [x] App contract note: app sends plain UTF-8 through existing blur transport

## Phase 4B — Selectable Font Families
- [x] Stable contract: Basit/Janat and Tahoma families, each with 7px and 16px variants;
      small/large selection remains independent
- [x] Persistent `/setFont?font=0..1&lang=0..1&digits=0..1` selector; legacy Arial EEPROM values migrate to Basit.
      changing EEPROM layout/version; selection resets to family 0 after reboot
- [x] Family 0 renamed to BasitFont7/BasitFont16, built from legacy Janat
      artwork; absent contextual forms reuse the nearest Janat form
- [x] Tahoma generated with joining-edge bearings; Arial Bold removed from the product build.
      Traditional rejected during physical review
- [x] `fontFamily`, `language`, and `digitStyle` are persisted; Arabic remains
      the default display language
- [x] Validated `font`, `lang`, and `digits` parameters are exposed by the
      firmware and Android selector
- [x] Basit ASCII letters/punctuation use the Tahoma-derived artwork while
      Arabic slots remain Janat-derived; Western/Indic digits stay under the
      dedicated digit generator. Regeneration is reproducible with
      `tools/build_digit_fonts.py` then `tools/build_english_hybrid.py`.
- [x] Generated font headers now label every width/glyph block with its ASCII
      character or Arabic Unicode/form name; `tools/annotate_font.py` applies
      the same labels to editor-exported headers.
- [x] All eight runtime font variants (Basit/Tahoma, 7/16px, Western/Indic)
      have matching editable `.ftsd` projects in `GLCDFontCreator2`.
- [x] Added `tools/import_fontcreator.py` and `build_fonts_release.bat`:
      validate all eight FontCreator exports, regenerate labeled production
      headers atomically, then build the four ESP8266 release profiles.
- [x] Removed unused legacy DMDESP font headers from the production tree;
      retained only runtime families and Janat7/Janat16 source fonts needed by
      the reproducible font-generation tools

## Phase 5 — Verification & Delivery
- [x] `pio run` passes; `pio test -e native` passes (10/10)
- [x] Fix: copy all runtime-used `PROGMEM` strings to bounded RAM buffers (D9)
- [x] Fix: guarded `effectiveBrightness()` and removed its direct RTC read (D8)
- [x] Restore original DMDESP timer calls after hardware showed slow top-to-bottom
      row painting with their removal (D12; D7 superseded)
- [ ] Owner wording review of texts.h strings (events/azkar/splash)
- [x] 7px BasitFont extracted from Janat7; other families use generated 7px
      assets
- [x] Hardware test by owner: P10 image and movement confirmed excellent;
      API, invalid/valid OTA, and 20 resets passed
- [x] Reliability soak: owner confirmed the 24-hour manual test complete
- [ ] Merge `feature/arabic-support` into `main`
- [ ] Update README.md (new Arabic pipeline, app protocol notes)
- [x] Package `P104-rc1` for 1x1, 2x1, 3x1, and 4x1 with SHA-256 manifest;
      tag `firmware-rc1` (no hardware upload performed)

## Restart Bug — Root Cause & Fix
**Symptom:** Firmware restarts continuously after boot.

**Proven root cause:** `SPLASH_TITLE` is stored in flash (`PROGMEM`) but was
passed directly to DMDESP as if it were a RAM string. The physical boot log
reported `Exception (3)` and `excvaddr=0x4024d023`; that address matched the
`SPLASH_TITLE` symbol exactly, while the decoded stack pointed to
`splashScreen()` → `setup()`.

**Fix:** Copy `SPLASH_TITLE` and all other runtime-used flash strings into
bounded RAM buffers before passing them to RAM-string APIs; use `snprintf_P()`
for flash format strings. `effectiveBrightness()` was also changed to use
cached `timeData`, so it cannot access the RTC before `rtc.begin()`. The fixed
firmware was uploaded to COM5 and reached `RTC OK` / `Setup done` without an
exception or repeated boot during the initial serial test. See D8 and D9 in
DECISIONS.md.

**Display follow-up:** Removing DMDESP's original timer calls caused visibly
slow top-to-bottom row painting on the real P10. Both calls were restored and
the vendored library is back to its repository version; see D12.

## Known Pitfalls (do not relearn these the hard way)
- `drawChar` uses `(uint8_t)ch` — bytes ≥0x80 work with extended fonts without DMDESP changes.
- Shaped output length ≤ input length (UTF-8 2 bytes/glyph → 1 byte/glyph out); size buffers accordingly.
- Do NOT reverse digits or `:` inside Arabic lines (times look wrong).
- `renameFirmware.py` pre-build script must stay wired in platformio.ini.

## 2026-10-04 - Optional ultra night power save

- [x] Added an opt-in `eco` setting to the shared compact API without changing
      the P10 EEPROM layout; it reuses the existing Isha/Fajr sleep window.
- [x] P10 exposes a persisted night-brightness level and shows one dim breathing
      pixel in ultra mode while keeping
      RTC/Wi-Fi active; 7-segment shows one dim breathing dash in the
      temperature field instead of a text `ECO` marker.
- [x] Android exposes the same checkbox for both display families; it is off
      by default and `0/0` sleep offsets disable the window.
- [x] P10 release/build verified; 7-segment ta73/ta73s builds verified.
- [ ] Owner: install firmware/app and verify entry/exit around a short test
      window, OTA behavior, and the visible heartbeat on both displays.

## 2026-09-28 - Card scheduler
- [x] Added EEPROM sidecar preferences without changing `EEPROM_VERSION`.
- [x] Added validated card enable mask and order with `/getCardSettings` and
      `/setCardSettings`; legacy `/getData` and `/updateMain` remain unchanged.
- [x] Added P10 Android card settings dialog with enable/disable and ordering.
- [x] 4x1 release built and uploaded to COM5; RAM 45.4%, Flash 41.4%.
- [x] Native scheduler/shaper/time tests pass (17/17) with the PlatformIO GCC
      toolchain path supplied on Windows.

## 2026-09-28 - Narrow small-font information layout
- [x] On 2x1/1x1 small-font profiles, day/Gregorian/Hijri cards render in the
      lower row instead of being skipped; the clock remains fixed on top.
- [x] 4x1 small-font top-right rotation remains unchanged.
- [x] Firmware builds pass for `p10_4x1`, `p10_2x1`, and `p10_1x1`.
- [ ] Physical verification on 2x1/1x1 remains with the owner; no upload done.

## 2026-09-28 - Sunrise iqama
- [x] Sunrise iqama defaults to 10 minutes when legacy EEPROM contains zero.
- [x] v2 API now returns the saved sunrise iqama and the runtime honors it.
- [x] Built and uploaded `p10_4x1` to COM5; esptool hash verification passed.
- [ ] Owner should verify the sunrise event window physically.

## 2026-09-28 - RTC bus recovery
- [x] P10 uses 100 kHz I2C and bounded SDA/SCL recovery before retrying DS3231.
- [x] RTC reads/writes stay guarded; the main loop does not restart on RTC loss.
- [x] Applied the same guarded recovery pattern to `Masjed_TA_All_v2` 7-segment
      source; owner still needs the physical disconnect/reconnect test.
- [x] Added a startup-only local RTC fault indicator: one P10 pixel or one
      unused 7-segment position blinks without changing the normal layout.

## 2026-09-28 - Display geometry profile
- [x] Added one compile-time `DisplayProfile` for display zones and dimensions.
- [x] Routed P10 top-line, small-font, card-placement, and diagnostics drawing
      through the profile without changing the API or EEPROM layout.

## 2026-09-28 - Technician health diagnostics
- [x] Extended the existing authenticated `/v2/health` response with firmware
      identity, reset reason, uptime, heap metrics, RSSI, AP clients, RTC state,
      and time since the last RTC sync.
- [x] Added a no-flash-write technician-only diagnostics action in Android.
- [ ] Owner should build/upload this source revision and verify the diagnostics
      dialog on a connected P10 clock.

## 2026-09-28 - Production reliability checks
- [x] Added authenticated `/v2/selfTest` for configuration, RTC, display init,
      AP mode, heap, and profile checks.
- [x] Added ESP8266 reset-info detail to the existing health response.
- [x] PlatformIO already contains compile profiles for 1x1, 2x1, 3x1, and 4x1
      plus matching release profiles; physical/build verification remains with
      the owner.
## 2026-09-30 - Local OTA progress display

- [x] Draw received-file progress inside the shared OTA upload callback, with
      a small fixed font and at most one changed frame per 150 ms.
- [x] Android supplies optional file `size` on the unchanged `/updateMain`
      endpoint; old apps still work with an indeterminate local indicator.
- [x] Hold at 99% until firmware validation succeeds, then show 100% followed
      by a static Arabic/English success message until the scheduled restart.
- [x] Restore the normal display font and cards on failure or cancellation;
      success takes priority over cards, prayers, splash, and test displays.
- [x] Build 4x1 Release; prepare the raw Android firmware asset.
- [ ] Owner: install once, then update again to check progress/success on P10;
      also check cancellation and small-panel layouts. No hardware upload,
      Android build/APK, or runtime tests performed in this step.
- [x] After hardware showed persistent OTA flicker, disabled DMD output for
      the transfer path; progress is intentionally app-side and the panel
      resumes only for the final result.

## 2026-10-04 - OTA progress synchronization

- [x] 7-segment OTA sends the percentage calculated from bytes accepted by
      `Update.write()` to a temporary UDP listener supplied by Android.
- [x] Android no longer uses socket-write progress or HTTP polling; it receives
      one lightweight UDP update per changed percentage.
- [x] Production TA build disables verbose serial logging to keep OTA size
      within the small-device budget.
- [x] Android keeps a compatibility fallback for legacy `/updateMain`
      handlers that reboot before the HTTP response: a fully transmitted
      image (or UDP 99% from newer firmware) advances to the restart stage
      instead of showing a false connection-reset failure.
- [x] Fixed Android multipart OTA streaming so the file-part wrapper flushes
      without closing OkHttp's complete request stream; this prevents a
      truncated multipart boundary and reboot-without-install behavior.
- [x] Firmware check-update versions are aligned with the Android asset map:
      P104=11 and TA M73=15.
- [x] OTA failures now return compact diagnostic codes (`begin`, `token`,
      `write`, `end`, or `aborted`) while the Android UI remains user-friendly.
- [x] Android update flow now carries the target version, logs the device's
      current version, and displays `current • target` before upload; the M73
      target is aligned to release 15 to avoid offering the same image again.
- [x] Android OTA client now uses explicit long read/write timeouts so a
      slower P10 flash write is not cut off by OkHttp's 10-second default.
- [x] UpdateActivity rechecks the live version and refuses to re-upload when
      the clock is already at or above the target release.
- [x] The update confirmation dialog now shows current/target versions before
      opening OTA; the update screen stays focused on progress only.
- [x] Wi-Fi process binding now debounces a lost network for 2 seconds and
      ignores duplicate callbacks, reducing unnecessary bind/unbind churn.
- [ ] Owner: retry OTA after a power cycle and verify matching progress on the
      phone and 7-segment display.

## 2026-10-05 - Android connection retry

- [x] Added a styled Retry / Wi-Fi settings dialog for connection failures.
- [x] Retry validates the selected clock SSID, rebinds Wi-Fi, and checks the
      model through `/checkUpdate`, refreshing its session token after reboot.
- [x] Reconnection preserves edited forms and never automatically replays a
      failed save. Removed recovery `getData` calls that could overwrite edits.
- [x] Network callbacks run on the main handler and ignore released requests.
- [x] Android Java/resources compilation passed; no APK was created.
- [ ] Owner: check disconnect/retry, wrong Wi-Fi, preserved edits, and clock
      reboot on P10 and 7-segment; no firmware upload needed for this change.

## 2026-10-05 - P104 manual OTA: truthful acknowledgement and release 12

- [x] Reproduced the package mismatch in files: Android target 12 but both
      P104 source and the previous asset advertised 11. Built a real release
      12 and published `updateP104.bin` (397840 bytes); checked its embedded
      checkUpdate string and matching SHA256 against the build artifact.
- [x] Corrected publisher routing: P104 is built from Masjed-P104-Esp-v2,
      not Masjed-P10-All-v2; the latter still supplies P101/P102.
- [x] Removed false success from socket-write byte counts. HTTP `UpdateOk`
      or valid peer UDP 99% confirms acceptance; a closed connection without
      acknowledgement now shows an unconfirmed result. This supersedes the
      complete-upload legacy success fallback recorded on 2026-10-04.
- [x] UDP progress validates the source IP, range, and monotonicity; terminal
      UI cannot be overwritten by already queued progress frames.
- [x] Added low-volume debug serial logs for OTA begin, write milestones,
      end, and complete. Android records transmitted bytes and last flash
      percentage separately; response-body read failures follow the same path.
- [x] P10 build and Android Java/resource compilation passed; no APK created.
- [ ] Reset cause is not confirmed by the phone log alone. COM5 was idle
      during capture. PC Wi-Fi is disconnected and Windows refused saved-AP
      connection without location permission; live OTA remains pending a
      manual connection to Basit-Masjed-5238166 and serial capture of upload.

## 2026-10-05 - Intermittent Android OTA network loss

- Owner clarified that target 13 in the app is intentional to repeatedly
  upload the current release-12 image. Preserve this testing override;
  the previous target/image mismatch observation is not a diagnosis of
  these deliberately forced attempts.
- Owner reported one connection abort among six attempts. The failing
  attempt received UDP 0, sent 397840 bytes, and then lost Wi-Fi without
  flash acceptance. This does not prove either flash failure or ESP reset.
- [x] Pinned Android checkUpdate/upload TCP sockets and UDP progress socket
      to the selected clock's Wi-Fi Network, after SSID validation. Other
      Activities' process-wide binding changes cannot reroute these sockets.
- [x] Closed the progress socket if its setup fails; HTTP acknowledgement
      remains usable without UDP. No polling, automatic upload retry,
      firmware-version change, or firmware upload was introduced.
- [x] Android `:app:compileDebugJavaWithJavac` passed (three deprecation
      warnings); no APK built. Source/API docs updated.
- [ ] Owner: repeat OTA using Android Studio. If the abort recurs, capture
      P10 COM5 OTA begin/write/end/reset logs during that exact attempt;
      network pinning cannot prevent actual AP loss or an ESP reboot.

### Live USB capture during owner OTA retries

- COM5 was available through pyserial even though Win32_SerialPort returned
  no entry. Captured serial at 115200 with DTR/RTS held false; did not flash,
  power-cycle, or request an ESP reset. Phone ADB GET checkUpdate returned
  `update,P,104,12,good` over its clock Wi-Fi interface.
- Three uploads were captured reaching `OTA end: received=397840
  expected=397840`, `OTA Success`, and `OTA complete: ok=1 error=none`.
  Captured subsequent successful boots reported Software/System restart.
- A later attempt showed `OTA Start`, `OTA begin: ok=1 expected=397840
  free=2744320 heap=28536`, then booted again with `Reset: Power On`
  before a 25% milestone or success acknowledgement. No watchdog/exception
  report was visible. This localizes that interruption to an ESP reboot,
  not merely Android routing; it does NOT prove a supply fault.
- Owner confirmed that the Power On reboot was spontaneous, with no manual
  power disconnection or reset. Next check is supply stability and
  reset/enable wiring under OTA;
  do not add another speculative software fix. RTC lostPower was also
  logged on normal successful boots, so it is not independent proof of a
  fresh power failure during the interrupted upload.
- Serial capture stopped and port released. No source changes, new
  firmware release, APK, or new build were needed for this diagnostic run.

### USB-isolation result reported by owner

- Owner confirmed ESP power comes from the screen's supply. With ESP USB
  disconnected, five OTA attempts succeeded and the sixth aborted.
  Android logged sent=397840/397840, flashProgress=0, confirmed=false;
  Wi-Fi network 839 was lost and replaced by 840. USB is therefore not
  a necessary trigger; the previous capture's Power On reset must not
  be generalized into a proven supply diagnosis for every attempt.
- Existing POST /health reports the boot reset reason and details, so
  we can read the latest reset without reconnecting ESP USB or adding
  telemetry firmware. The ADB phone is presently on ME; requested clock
  AP reconnection only, with no extra update/reset that would overwrite
  the latest boot diagnosis. Power/regulator checks remain pending.

### Post-failure health read, without ESP USB

- After owner reconnected, the PC could reach the clock directly even
  though the ADB-connected phone remained on ME. GET checkUpdate reported
  P104 release 12. Read-only POST /health with body v=2 returned:
  reset=External System, resetInfo=External System, uptime=167 seconds,
  heap=35432, maxBlock=35184, fragmentation=1%, rtc=0, profile=4x1.
- Latest reported reset is external, not watchdog or exception. Inspect
  the board's reset/enable circuit and power/regulator wiring before
  changing OTA code; Power On from the earlier attempt is a separate
  observation, not proof of the same electrical cause. The snapshot
  cannot detect a transient voltage dip or a previous reset overwritten
  by another boot.
- RTC is currently not detected/healthy; this is a separate observed
  issue, not yet a proven cause of OTA interruption. Need board type and
  wiring/supply details from owner. No settings, firmware, or APK changed.

### Owner correction: manual recovery after OTA hang

- Owner clarified that, during the failed USB-disconnected OTA, ESP appeared
  stopped/unresponsive. Owner then disconnected and restored clock power;
  the subsequently queried External System reason is from that recovery,
  NOT evidence of the OTA failure's original reset cause. Withdraw the
  inference that this health snapshot localizes the cause to reset wiring.
  It also does not rule out WDT, an exception, a hang, or supply trouble
  before the recovery. Need capture BEFORE owner power-cycles the clock.
- RTC was accessible from the mobile app and worked after setting time.
  rtc=0 was a firmware health snapshot, not proof of a disconnected RTC;
  owner reported incorrect/unset time rather than physical module loss.
- Source observation only: DMD pause stops refresh and blanks output but
  leaves its 100-microsecond SDK timer armed. Potential isolated hypothesis
  for OTA interaction, NOT a verified cause. No DMD/OTA code changed.

## 2026-10-05 - Owner-approved DMD timer suspension experiment

- [x] DMDESP pause now disarms dispTimer, and resume rearms the same
      100-microsecond timer only when paused. The OTA success, failure,
      and abort recovery paths already use these functions. This is an
      isolated hypothesis test, not a confirmed root-cause fix.
- [x] Documented necessary library-core exception in DECISIONS D59. No
      normal refresh rate, EEPROM, API, or update-version change.
- [x] p10_4x1 build passed: RAM 37264 bytes (unchanged), flash 393735
      bytes (+48), binary 397888 bytes. Copied that binary into Android
      assets/updateP104.bin and verified matching SHA256:
      42d60fc13e41f44e55199e414d2ff99112f3678f9bf041d8e438e5ecba1e5de2.
      Updated the previously stale asset manifest. App forced target 13
      stays unchanged; actual image remains 12. No APK built.
- [x] Initial COM5 identity check was blocked by another program. Owner
      closed it; esptool confirmed chip ID 0x4fed96 (5238166) and MAC
      24:4c:ab:4f:ed:96. Serial upload on COM5 then passed, including
      flash data hash verification (397888 bytes, 2026-10-05).
- [ ] Post-upload HTTP check timed out because the clock AP was not
      reachable from the PC. Serial upload success is verified, but
      timer suspension and resolution of the intermittent hang still
      require the owner's repeated mobile OTA/display test.
- [ ] After serial installation, owner must run the app from Android Studio
      so it includes the new asset, then repeat OTA. Uploading the old app
      would restore the old timer behavior. If a hang recurs, capture it
      before manually cutting clock power; do not claim proven repair.

### DMD timer experiment: failure reproduced (owner log 06:18)

- Owner ran the new 397888-byte image. UART recorded OTA begin ok=1
  at 06:18:53.960 (heap 28536), boot-ROM characters at 06:18:54.028,
  then application startup at 06:18:55.919, reset=External System.
  Android saw connection abort, flashProgress=0, and Wi-Fi 848 -> 849.
- Therefore SDK display-timer suspension alone does NOT resolve this
  failure. Keep it labelled an experiment, not a commercial reliability
  fix; no additional speculative firmware changes made in this turn.
- Unlike the earlier post-manual-recovery health read, this serial log
  directly covers the failing upload and subsequent boot. Owner confirmed
  the reset was automatic with serial monitor kept open and no reset or
  power intervention. It proves an upload-time reboot; electrical cause
  still needs isolation and must not be claimed proven from the enum alone.
- ROM characters were captured at application baud 115200; the ROM
  normally uses 74880. A correctly decoded ROM reset/boot-mode capture
  or controlled power/reset-circuit isolation is the next diagnostic,
  not another timeout change or false-success fallback.

### Owner's 74880-baud capture (06:25-06:31)

- Attachment 9fa82614-e672-403f-ad8c-24fb11539dbd/Pasted text.txt records
  COM5 at 74880. Readable ROM boots: 06:25:43.708 rst cause:2,
  boot mode:(3,6); 06:31:13.523 rst cause:2, boot mode:(3,5).
  Cause 2 is external reset; the first boot-mode value 3 is normal
  flash boot, and the second value is not by itself an error.
- No readable hardware-WDT reset or boot checksum error is present.
  Later application text at 115200 is garbled as expected at 74880;
  it cannot be decoded back from the pasted replacement characters.
  Repeated RF-calibration lines alone do not classify uploads as failures.
- The file does not include matching Android result/timestamps for these
  two readable boots. Need identify whether the 06:31:13 boot corresponds
  to the failed upload rather than monitor opening/manual intervention.
  Do not treat the initial monitor-open reset as fault evidence.
- No new code/build/upload in this diagnostic turn. Next isolation needs
  board/regulator/wiring details or a controlled independent power test,
  not another speculative change to the OTA transport.

### Owner wiring clarification

- ESP is directly wired to P10 signals, gets 5V through a 3.3V regulator,
  and takes its ground from P10 header pins. Regulator model, capacitor
  network, reset/enable pull-ups, and exact wiring remain unknown.
- This is not proof of a bad supply or ground. Next single-variable
  electrical check: with power off, arrange a short dedicated common
  ground return for ESP/regulator to supply negative rather than relying
  on the panel header return; preserve a shared ground with P10 and do
  not apply 5V to ESP's 3.3V supply or GPIO pins. Need regulator model or
  wiring photo before recommending component values or another supply.
- Manufacturer supply guidance: 3.3V with 500mA current capability;
  assess regulator transients/thermal limits, not only nominal rating.
  Reference: https://documentation.espressif.com/esp8266_hardware_design_guidelines_en.html

## 2026-10-05 — Simple runtime / Android completion batch

Owner approved one continuous batch, excluding dual-copy storage. Preserve
EEPROM/LittleFS formats, configuration and firmware version numbers. No APK.
Do not claim the intermittent electrical/reset OTA investigation resolved.

- [x] Skip identical P10 framebuffer publication without another framebuffer.
- [x] Use identical half-open midnight-safe night-window rules in both displays.
- [x] OTA stages follow upload/end/acceptance; no sent-bytes success fallback.
- [x] Common localized save/error messages and immediate numeric validation.
- [x] Remember verified selected clock; reuse only on that exact Wi-Fi SSID.
- [x] Large-font layout audit (especially OTA), build and progress handoff.

Existing rolling displays, font rendering, storage CRC and unchanged-write
guards are retained. No broad refactor, new dependency or test framework.

Implementation and verification:
- P10 `TimeMath.h` and 7-segment `PrayerPhaseContract.h` have the identical
  night helper, inclusive start/exclusive end and midnight normalization.
- P10 compares existing buffers before publication. TA6932 avoids identical
  segment writes and emits control-only updates for brightness/on-off.
  Reset force-sends and resets the published cache. D60 documents driver edits.
- Android uses common Arabic/English saved/failure/Wi-Fi messages. Numeric
  fields validate immediately and before saving (iqama 0..120, prayer/Friday
  sleep 0..120, night offsets 0..720); coordinates reject nonfinite/out-of-range
  data. Existing UTF-8 byte limits/password validation remain.
- Last clock SSID is stored only after DB/model identity verification. Reuse
  requires current SSID match and performs checkUpdate normally; otherwise
  the chooser remains available. Check/update sockets and gateway use the
  same selected Wi-Fi Network, not a mobile-data default gateway.
- OTA has four states and failed-stage mark; 98 starts finalization, 99/HTTP
  UpdateOk confirms acceptance. UDP acceptance advances without waiting for
  an HTTP timeout. Completion after the existing 5-second grace is not a
  verified reboot; next normal version check remains the verification path.
- OTA/settings choices use content heights; common dialogs scroll if needed.
  Existing floating circular save buttons/tab styling are preserved.
- Android compileDebugJavaWithJavac passed (no APK). All layout XML parsed;
  each ScrollView has exactly one child. Live large-font checks remain owner-led.
- P10 p10_4x1: build passed, RAM 37264 unchanged, Flash 393847 (+112 versus
  preceding timer experiment). Bin 398000 bytes. ta73/ta73s builds passed:
  RAM 37648/38084, Flash 403555/407911. Driver cache costs 56 bytes overall.
- Copied P104, M73 and M73s binaries into the correct Android assets and
  compared SHA256 with each build. P104 manifest refreshed. Actual firmware
  versions and owner's intentionally increased Android target13 unchanged.
- No hardware upload: serial.tools.list_ports returned no ports and PC
  checkUpdate timed out. Reconnect USB/AP for upload; no success claimed.
- Existing deleted tests, dirty font files and other owner changes preserved;
  no broad automated suite added per owner's earlier request. Diff checks for
  touched firmware code clean; whole-tree whitespace findings belong to the
  existing edited font and unrelated MainActivity blank lines.

Next owner checks: blank/out-of-range input, saved toast only after board ACK,
last-clock reopen versus another Wi-Fi, large-font dialogs, and night-window
exit at the selected minute. Intermittent early OTA reset is still a separate
unresolved hardware/firmware diagnostic, not fixed by this UI/runtime batch.

### Always-on AP follow-up — 2026-10-05

- [x] Removed 7-segment ten-minute expiry and its unused timer/flag.
- [x] Verified P10 has no scheduled AP shutdown; no P10 code change.
- [x] Preserved startup Wi-Fi cleanup, saved passwords and night/ultra modes.
- [x] Built M73/M73s and refreshed Android assets; SHA256 matches each build.

Build results: RAM 37648/38084 unchanged; Flash 403491/407847 (-64 bytes
each). Android assets updateM73.bin/updateM73s.bin are 407648/412000 bytes.
No version, EEPROM layout or Android UI changes; no APK built.
Hardware deployment remains pending: no serial ports detected and PC
checkUpdate timed out. The beyond-ten-minute live AP check is not yet run.

### Exact-size OTA and clear failures — 2026-10-05

- [x] P10/M73/M73s require decimal positive size, reject overflow/no space,
  write only within that size and reject incomplete uploads.
- [x] Finalize only after the whole multipart request; strict Update.end(false).
  Safe rejection/abort releases the updater without committing the image.
- [x] Async 7-segment preserves the active request and cleans up on disconnect.
- [x] Android translates firmware rejection reasons into Arabic/English;
  partial-send interruption is distinguished from full-send/no-confirmation.
- [x] Single upload request, ACK-only success and five-second restart retained.
  No MD5, new tests/dependencies, EEPROM reset or version-number changes.
- [x] Final builds succeeded: p10_4x1 RAM 37308 (+44), Flash 394147 (+300);
  ta73 RAM 37736 (+88), Flash 404379 (+888); ta73s RAM 38156 (+72),
  Flash 408751 (+904), relative to the preceding AP batch.
- [x] Android compileDebugJavaWithJavac passed, no APK. Updated assets and
  verified build/asset SHA256: P104 398304, M73 408528, M73s 412896 bytes.
  P104 manifest size/hash refreshed, actual version 12 unchanged.
- [ ] Hardware deployment/manual OTA: no serial ports found; PC checkUpdate
  timed out. Size/error behavior is code-reviewed and built, not live-tested.

Owner manual checks after reconnect/upload: one normal OTA; reject a short
file against its larger declared size; reject an oversized file against a
smaller declared size; reject missing/invalid size. No bad request should
trigger restart/acceptance, and a subsequent normal upload must still work.
Do not treat this batch as resolving the intermittent early OTA reset.

### Android unified waiting card — 2026-10-06

- [x] Owner approved waiting and save feedback in the same top position.
  Reused shared notice geometry and existing loading layout: petrol/gold,
  native spinner, contextual Arabic/English text and font-scale wrapping.
- [x] Wait appears after 300ms; fast replies cancel it, completion closes it
  immediately. Existing modal editing protection and request guard retained.
- [x] Activity-owned cleanup prevents old callbacks dismissing a new wait;
  errors have priority. No new requests/dependencies or firmware changes.
- [x] Android Java/resource compile passed without APK; existing deprecation
  warnings only. Details recorded in Android API.md.
- [ ] Owner phone preview, large system font and rotation check.

### Android save feedback — 2026-10-06

- [x] Owner approved the compact top card + non-stacking successes and longer
  error feedback. Implemented in Android classes/Snake.java and its existing
  layout/background, with vector success/error icons, shared localized text.
- [x] ACK-only save logic retained; native fade, safe top insets, wrapping
  text/system accessibility duration and Activity cleanup included.
- [x] Android compileDebugJavaWithJavac succeeded; changed XML parses, and
  text/icon contrast meets 4.5:1/3:1 on the opaque petrol surface. Existing
  deprecation warnings remain; no APK/new test suite was built.
- [ ] Owner phone/large-font/landscape visual check pending. No firmware or
  asset updates are needed for this UI-only change.

### Android contextual success text — 2026-10-06

- [x] Owner approved naming the saved section. Seven Arabic/English success
  texts mapped to firmware ACKs rather than the current tab; password keeps
  its reconnect instruction. Existing top-card design/timing retained.
- [x] Java/resource compile succeeded without APK; translation pairs checked.
  No firmware, API or asset changes; no additional requests/tests added.
- [ ] Owner visual check on phone remains pending.

### Android personal GPS location — 2026-10-06

- [x] Owner approved one “إضافة مكاني” button below the existing country,
  governorate and city selectors, with an optional name and GPS-only dialog.
- [x] Native foreground GPS, precise-permission flow, settings/retry,
  fresh-fix/accuracy checks and timeout; no map, internet, background service
  or new dependency. GPS stops on cancel/background/destruction.
- [x] One locally stored place per clock, selectable alongside bundled cities.
  Use marks a draft; only page Save sends the existing /setLocation with
  ct=0. Both firmware implementations already accept raw coordinates.
- [x] Unified Android board-location restoration prevents asynchronous
  Spinner callbacks / approximate city matching from replacing a personal
  location. Full local precision survives the firmware's four-decimal reply.
- [x] Hidden coordinate rows no longer take space. Arabic/English strings
  and scrolling dialog follow the existing styling / large-font approach.
- [x] Android Java/resource compile succeeded without APK; changed XML parses
  and all 17 new Arabic/English strings are paired. Existing whitespace in
  unrelated MainActivity lines was preserved; deprecated compatibility APIs
  produce warnings, not build errors.
- [x] Focused independent code review found no confirmed blocker; GPS
  freshness is evaluated at capture, and the dialog resizes on rotation.
- [ ] Owner offline GPS, cancel/permission/settings/background/large-font
  checks and Save/readback on P10 + 7-segment remain pending.

No firmware/API/database/asset/version change; no APK. Implementation and
verification detail is recorded in the Android project's API.md. Existing
Egypt places retain priority; expanding/validating administrative data is
separate deferred work, not covered by this implementation.

### ESA reference comparison — 2026-10-06

- [x] Owner approved three-location comparison. Compared Cairo, Alexandria
  and Aswan against the dated ESA daily table using existing Android city
  points and independent GeoNames city reference points (NOT handset GPS).
- [x] Compiled the actual P10 and 7-segment prayer libraries with their
  current Egypt/Asr/DST configuration and native minute rounding. All 36
  computed values per implementation matched between the two paths.
- [x] For each coordinate source: 16/18 displayed times match ESA; remaining
  differences are +1 minute at Alexandria Fajr and Aswan Dhuhr. No blanket
  correction, formula or database change is justified by this one-day sample.
- [x] Inputs, dates, sources, full results and limitations recorded in
  `docs/LOCATION_VALIDATION.md`; temporary probes stay outside product code.
- [ ] Actual handset GPS and Save/readback on both boards remain owner-led.
  City-table agreement does not certify an individual mosque's GPS point.

No firmware/Android runtime/asset/version modification or APK in this batch.

### Reviewed Egypt data additions — 2026-10-06

- [x] Owner accepted the observed one-minute reference difference; no formula fix.
- [x] Six reviewed GeoNames towns appended as Android city IDs 290–295.
  All 257 prior places, countries/governorates and serial/model tables preserved;
  SQLite integrity checked. Total now 263 places / 261 Egypt.
- [x] Existing installed cities retain priority; missing bundled IDs are appended
  at read time, so no clearing app data or resetting the selected place is needed.
- [x] Idempotent INSERT-only append tool and LOCATION_DATA.md provenance added
  in Android; localized public source dialog reused in Settings. No new selectors.
- [x] Android Java/resources compilation passed without APK. No firmware/API/
  version change. Historical rebuild_db.py is not an append/migration tool.
- [ ] Owner phone check: new towns visible, prior selection/local GPS preserved.
- [ ] Further villages/ambiguous towns deferred until position/admin review.

### Egypt town classification and second batch — 2026-10-06

- [x] Android appended 15 references (IDs 296–310); total 278 / 276 Egypt.
  Seven existing IDs moved to reviewed governorates: 39, 115, 116, 118,
  121, 131, 132. Existing IDs/names/coordinates and all serial/model data retained.
- [x] Owner confirmed Nashart in Qillin and Abu Ismail near Qillin; both Kafr
  El Sheikh. Owner explicitly requested province-only, not a Nashart point change.
- [x] Installed DB governorates synchronize once without data clearing/schema
  change. Both firmware model restore paths choose governorate via stable city ID;
  no automatic setLocation request. Existing names/points retain priority.
- [x] Guarded append/update tool applied 15/7, then 0/0; SQLite integrity and
  retained fields checked. Source/license dialog updated; Android compile passed,
  no APK or firmware/version/prayer changes.
- [ ] Owner phone check: corrected places/new cities, personal point, other countries.
- [ ] 128 (64 الخاشعه) and remaining modern cities/uncertain points remain open;
  Android LOCATION_DATA.md is the authoritative handoff. Do not claim all Egypt
  finished or replace points/merge stable IDs without a separate owner decision.

### Firmware naming cleanup — 2026-10-08

- [x] Removed the misleading legacy `winterTime` name and aligned both
  firmwares on `summerTime`/`isDST`; corrected remaining configuration spelling
  errors without changing EEPROM layout or API keys.
- [x] P10 and ta73/ta73s release builds pass; the independent RTC/time harness
  passes 25/25.

### Descriptive time/API names — 2026-10-08

- [x] Replaced ambiguous `dst`, `summerTime`, `calcMethod`, and `wayMethod`
  identifiers with `timezoneOffset`, `daylightSavingTime`,
  `prayerCalculationMethod`, and `asrMethod` in both firmwares.
- [x] Updated Android and the wire fields to `timezoneOffset`,
  `daylightSavingTime`, `prayerMethod`, and `asrMethod`; EEPROM layout is
  unchanged.

### API parser compatibility fix — 2026-10-08

- [x] Increased the root POST parser key capacity to accept the 18-character
  `daylightSavingTime` field; Android time-save requests are no longer rejected
  as `bad_body`. The small API client validator uses the same limit.
- [x] P10 release build passes; no EEPROM layout, display behavior, or API route
  was changed.

### Cross-display verification — 2026-10-08

- [x] Owner manually confirmed time save on both P10 and 7-segment clocks.
- [x] All eight P10 environments (development and release, 1x1 through 4x1)
  build successfully after the parser fix; RAM remains below 44.1% and Flash
  below 38.2%.
- [x] ta73 and ta73s both build successfully; no additional API or EEPROM
  change was needed.
- [x] 7-segment now exposes the same technician-only `/health` and `/selfTest`
  POST contract as P10; its LittleFS persistence remains intact and its HTTP
  request loop is now synchronous like P10.
- [x] After the AP returned, live post-OTA `health` and `selfTest` both returned
  `ok=1`, `rtc=1`, `timeValid=1`, `display=1`, and `wifi=1`; the board reported
  a normal `Software/System restart`.
- [x] Android Java compilation passes; live PC-to-P10 API probing was blocked
  because Windows Wi-Fi was disconnected and Location permission prevented
  automatic connection to the saved AP profile.
- [ ] Final release gate remains owner-led: long soak, power/reset behavior,
  and final commercial firmware/version publication.

### Unified storage, HTTP and technician diagnostics — 2026-10-08

- [x] P10 configuration persistence now uses two LittleFS slots (`config-a.bin`
  and `config-b.bin`) with sequence selection, schema version and CRC. A failed
  write leaves the previous valid slot intact; a blank/unmountable new
  partition is formatted once, and existing EEPROM data is not migrated because
  updated units are reconfigured by the Android app.
- [x] 7-segment HTTP handling now uses the synchronous `ESP8266WebServer`
  contract, including compact raw POST bodies and streamed multipart OTA. The
  public route names and response fields are unchanged.
- [x] Android technician mode now exposes Health and Self-Test for both P10 and
  7-segment; customer mode keeps both controls hidden.
- [x] P10 all eight environments, 7-segment `ta73`/`ta73s`, and Android Java
  compilation pass. No APK or firmware was published from this batch.
- [ ] Physical 7-segment OTA and LittleFS power-loss validation remain owner-led
  before commercial release.
- [x] 7-segment captive-portal probes now return `204 No Content`; the old
  confirmation page is gone. Android remains responsible for launching the app.

### Unified legacy OTA transport — 2026-10-08

- [x] Android tries the market Multipart contract first (no `tk`, `size`, or
  UDP), then retries once with `tk`/`size` for intermediate firmware that
  explicitly rejects the legacy request; the progress bar follows bytes sent.
- [x] P10 and 7-segment OTA receivers accept the market format and commit the
  actual received image with `Update.end(true)`; optional `size` remains
  tolerated for transition, but is no longer required.
- [x] Updated `updateP104.bin`, `updateM73.bin`, and `updateM73s.bin` assets;
  P10, ta73/ta73s, and Android Java builds pass. No APK was built.
- [ ] Physical OTA validation on one market M73-1 unit and the connected
  7-segment board remains pending before release publication.
- [x] A live M73-15 board was probed with the legacy request; it returned
  `UpdateError:incomplete` and remained on the old async image. This confirms
  that this intermediate board needs a one-time USB bootstrap; it does not
  invalidate the market M73-1 source contract above.

### Shared calendar core — 2026-10-08

- [x] Gregorian-to-Hijri conversion now lives in the shared
  `CivilTimeContract.h` contract in both firmware projects; P10 and 7-segment
  keep their existing display-specific storage and rendering.
- [x] P10 `p10_4x1`, 7-segment `ta73`, and `ta73s` builds pass after the change.
- [x] Build size is effectively unchanged: P10 422,811 bytes text,
  `ta73` 402,675 bytes, and `ta73s` 407,043 bytes. The change targets source
  parity and maintenance safety, not a risky binary-size rewrite.
- [ ] Continue with the next shared pure helper only after comparing both
  implementations; do not merge display/API/storage code into one abstraction.

### Shared prayer-method core — 2026-10-08

- [x] The prayer-method table and Makkah 90-minute option now use the same
  `PrayerMethodContract.h` in P10 and 7-segment.
- [x] Both firmwares retain their existing PrayerTimes calls and display paths;
  only the method contract and field names were unified.
- [x] P10 `p10_4x1`, `ta73`, and `ta73s` builds pass. Existing compiler warnings
  are from the vendored PrayerTimes headers and the prior 7-segment config read.
- [ ] Keep the old P10 `PrayerMethodConfig.h` until the full source/reference
  audit is complete; remove it only after confirming no external test/tool uses it.

### Saudi Makkah-Isha option — 2026-10-08

- [x] Android now exposes the fixed-90-minute Makkah Isha option only when the
  selected country is Saudi Arabia and the Makkah calculation method is chosen.
- [x] Other countries automatically send `makkahIsha=0`; the existing firmware
  API field is reused without a new route or schema change.
- [x] Arabic and English labels/options were added; Android Java compilation
  passes. The bundled database now includes Saudi Arabia, so the option is
  available for Saudi locations only.

### Shared prayer-phase core — 2026-10-08

- [x] `PrayerPhaseContract.h` now owns the shared phase, minute-delta and night
  window helpers in both firmware projects; P10 `TimeMath.h` keeps only its
  general time helpers.
- [x] P10 `p10_4x1`, 7-segment `ta73`, and `ta73s` build passes with unchanged
  display paths and effectively unchanged binary size.

### Saudi location catalog — 2026-10-08

- [x] The Android asset now includes Saudi Arabia with Riyadh, Makkah and
  Madinah as selectable governorate/city entries.
- [x] Existing location rows remain authoritative; database version 6 inserts
  only missing bundled rows on an existing installation.
- [x] The three coordinates are city-center references for prayer calculation,
  not a replacement for the user's personal GPS location.
- [x] Removed two unused legacy DST helper definitions from the 7-segment
  source; active DST logic remains in the shared civil-time contract.
- [ ] The Android owner should compile and verify the country/governorate/city
  cascade on a device; no APK was built.

### RTC/calendar unification — 2026-10-08

- [x] `CivilTimeContract.h` is byte-identical in both projects and now owns
  Gregorian month lengths, calendar validity, the supported 2025..2099 range,
  12/24-hour conversion and raw DS3231 decoding.
- [x] Both RTC readers preserve I2C ACK/short-read checks and chip detection
  separately from oscillator-stop/calendar validity. No guessed time is written.
- [x] Both time-setting APIs and P10 prayer-cache validation use the shared
  calendar rules. Rendering, settings layout and API field names are unchanged.
- [x] Removed the unused 7-segment query validator and its unused request-facade
  members; compact root POST handlers remain the active commercial path.
- [x] The same temporary C++ check passes against both header copies: leap
  years, invalid dates/years, 12-hour AM/PM, invalid BCD, OSF, century flag,
  DST transition instants, Hijri and prayer-method/phase helpers.
- [x] All eight P10 and both 7-segment builds pass; no upload or Android
  firmware-asset publication is part of this batch.
- [x] POST field extraction is now shared as described below. Number parsing
  and settings-validation rules remain the next bounded comparison.

### Compact POST reader unification — 2026-10-08

- [x] Both projects use byte-identical `CompactApiContract.h`, preserving the
  existing UTF-8 line protocol, escape format and 16-field/19-character key limit.
- [x] Both reject duplicate keys, malformed escapes, embedded NUL and incomplete
  values before mutating settings. 7-segment no longer silently truncates a
  long numeric/password value or falls back to an alias after a primary overflow.
- [x] Host checks pass for the shared body/field helpers and the actual
  7-segment optional/required/alias integer, float and password readers.
- [x] Android settings now send only the shared `psai`/`psbf` sleep fields;
  removed ignored `sae`/`sbf` duplicates. The 7-segment request now has 15
  fields including `v`, below the shared 16-field limit. Java compilation passes.
- [x] Fresh builds after the final changes: P10 8/8 and 7-segment 2/2 pass.
  P10 4x1 release: RAM 37,364 bytes, Flash 421,731 bytes; ta73s:
  RAM 36,296 bytes, Flash 409,211 bytes. Existing library warnings remain.
- [ ] Final hardware/Android save tests remain owner-led; firmware images were
  built locally but not uploaded/published, and no APK was built.

### Numeric/settings contract unification — 2026-10-08

- [x] Both copies of `CompactApiContract.h` now normalize ASCII, Arabic and
  Persian digits in place, accept decimal separators and whitespace consistently,
  reject numeric overflow/underflow and nonfinite values, and leave the target
  setting unchanged on failure. Removed the separate P10 parsers and redundant
  7-segment normalization/parsing, including its 40-byte temporary buffer.
- [x] Both use byte-identical `ClockSettingsContract.h` for prayer/iqama,
  coordinates, timezone, DST, Hijri correction, 12/24h and night-window limits.
  Display, audio, password and record-version checks remain device-specific.
- [x] 7-segment now validates the full candidate before loading it into runtime
  and before writing it to LittleFS. A corrupt old record is rejected through
  the existing boot recovery/default path; valid records/layouts are unchanged.
- [x] RED/GREEN checks reproduced unsupported localized P10 numbers and accepted
  corrupt 7-segment coordinates, then passed against the final real code.
  Actual P10/7-segment readers, settings validators, POST, RTC/calendar helpers,
  existing three Python persistence checks and three native 7-segment checks pass.
  Temporary checks are outside the repository; no test-framework/dependency added.
- [x] Fresh final builds: P10 8/8, 7-segment 2/2. P10 4x1 release:
  RAM 37,364 bytes / Flash 421,875 bytes; ta73s: RAM 36,296 / Flash 409,355.
  RAM unchanged; the 144-byte Flash increase includes stronger validation.
- [x] M71 inspected: its separate `Masjed-71` source/version 9 uses GET and
  shift/latch hardware. Current Android sends POST only; its existing M71 asset
  is not sufficient. See `SEVEN_SEGMENT_COMPATIBILITY.md`; no M71 image changed.
- [x] Boundary fix completed 2026-10-09: Android sends full `tblCities.id` values,
  including Cairo IDs 400100+ and Saudi IDs 504001..504003, but both location
  readers now accept 0..INT32_MAX and store `int32_t`. P10 v6 records migrate
  after raw CRC verification, preserving every field without a boot write.
- [ ] **DEFERRED by owner:** Dedicated M71 firmware is a separate future task.
  Do not use M73 firmware on M71. See
  `superpowers/plans/2026-10-09-m71-api-upgrade.md`; no M71 action in this batch.
- [ ] No upload, version bump, Android asset publication or APK in this batch.
  Owner is away; physical save/RTC/OTA and long-run checks remain deferred.

### Remaining P10/M73 contract alignment — 2026-10-09

- [x] Full location IDs now roundtrip in both firmware projects, without catalog
  edits. P10 configuration version 7 uses the old record's two padding bytes;
  raw v6 CRC is checked before moving the text/password/font/hour tail. Invalid
  records remain rejected. Current passwords, texts, cards and settings survive.
- [x] Shared factory defaults match Android: iqama `{15,10,10,10,8,10}`,
  timezone 2, night offsets 30/30, placeholder coordinates 10.05/10.05.
  Existing valid configurations are not reset. M73-specific audio/display
  defaults and P10-specific fonts/cards remain separate.
- [x] POST body capacity is shared at 512 bytes (maximum 511 including fields,
  excluding final NUL). M73 releases 256 bytes of body-buffer RAM. Both require
  literal `v=2` and use `ok=1` in `/getData`; errors agree on malformed=400,
  oversized=413, arguments/RTC/password=422, storage=500 and busy=503.
- [x] P10 requires all six time fields and both coordinates; malformed input
  cannot silently reuse previous/default values. M73 selfTest now uses its full
  committed-configuration validator, not only brightness checks.
- [x] Restart grace is five seconds in both clocks. M73's deadline comparison
  handles `millis()` rollover; accepted OTA writes yield cooperatively in its
  current synchronous HTTP handler. No extra polling or progress simulation.
- [x] Nine focused native checks pass: real location handlers, shared defaults,
  restart rollover, raw-record migration/corruption, maximum escaped `/getData`
  (1054 bytes), real selfTest predicate and both real POST body handlers.
- [x] All five shared headers are byte-identical (include guards/comments aligned,
  no prayer formula changes). Shared parser/calendar regressions pass against
  both copies; existing M73 persistence checks 3/3 and native checks 3/3 pass.
- [x] Final builds after header alignment: P10 8/8 and M73/M73s 2/2 pass.
  P10 4x1 release RAM 37,380 / Flash 422,163 bytes; M73s RAM 36,056 /
  Flash 409,435. Existing PrayerTimes warnings remain. Logs are local
  `.pio/unification-final-build.log` in each repository, not release artifacts.
- [ ] Final device tests after this source batch: location 504003, preserved P10
  settings, reset defaults, date/time and short Android OTA on both clocks.
  Earlier intermittent-reset hardware gate remains open; no publication claim.
- [x] M71 has its own deferred plan. No M71 changes/build/upload, firmware release
  version bump, Android BIN publication or APK in this batch.

### Android OTA real progress / single status — 2026-10-09

- [x] Owner approved reusing the existing P10/M73 `up` UDP progress channel.
  Android now binds its temporary listener to the selected clock Network,
  validates the peer/message, ignores regressing/old-attempt updates and closes
  the listener on response/failure/retry/destruction. No polling or extra HTTP.
- [x] Local CountingRequestBody bytes are diagnostics only, never visible flash
  progress. The existing single multipart upload and format retry are retained.
  Real p=0..97 drives the bar; p=98/99 means final validation, not success.
  HTTP UpdateOk alone permits 100% and the existing five-second restart UI.
- [x] Missing/late progress (>2 seconds) uses an indeterminate indicator without
  delaying/canceling upload. No simulated percentage, socket-failure success,
  post-reboot probing or new firmware requirement for the optional telemetry.
- [x] Arabic/English status and percentage now share one fixed prominent field
  above the bar; step labels distinguish transfer/write from final validation.
  Advisory warning remains separate, title fixed; text wraps and scrolls.
- [x] New packet checks failed first, then passed; entire existing Android unit
  suite 7/7 and Java/resource compile pass. No APK, board upload, asset publication,
  firmware code or version changes. Owner edits preserved.
- [ ] Phone check on both clocks: real progress, UDP-stall fallback, UpdateOk,
  unconfirmed failure and large-font visual layout. Prior reset gate stays open.

### Android OTA completion polish — 2026-10-10

- [x] Owner approved hiding the bar/percentage after the five-second restart UI,
  not after merely sending the file. The 100% display still requires UpdateOk.
- [x] Removed redundant changing stage sentences and their six unused resource
  strings in each language. Stages now carry the state; the first is
  «نقل وتثبيت التحديث» / «Transfer and install update». Only real percentages,
  errors and already-current information use the compact message field.
- [x] Completion hides the bar and number; the final stage becomes the green
  success message. Failures explicitly show their message even after a hidden
  percentage. UDP, multipart transfer and acknowledgement logic are unchanged.
- [x] A small host check runs the actual Activity transition methods against
  view stand-ins: it failed first on visible completed progress, then passed
  restart=100%, completed=hidden bar/number, failure=visible error.
- [ ] Phone visual check remains owner-led; no APK or firmware change.
- [x] Kept a separate gold power warning throughout transfer/restart/completion:
  «لا تفصل الكهرباء حتى تعمل الساعة مجددًا». It also remains on unconfirmed
  failure after transferred bytes; the five-second UI does not prove reboot.
- [x] Actual-method host checks cover completed and failed-warning visibility;
  Java/resources compile and Android unit suite 7/7 pass after this polish.

### Android Wi-Fi binding and lint cleanup — 2026-10-10

- [x] Network binding now requires the selected clock SSID on an available Wi-Fi
  network; the device-list screen no longer binds an arbitrary Wi-Fi first.
- [x] Removed the five blocking lint errors without changing UI behavior.
  Android unit tests now pass 8/8; `compileDebugJavaWithJavac` and `lintDebug`
  both pass. Remaining lint output is warnings only.
- [ ] Physical phone test: switch between the clock AP and another Wi-Fi while
  opening Main/OTA and confirm the app keeps using the selected clock.

### Android initial connection and UI/UX audit — 2026-10-10

- [x] Main binds to the selected clock network before its initial getData.
  Missing network uses the existing connection dialog, not a premature request.
- [x] Main retains the verified board model from the device-list result for
  reconnects, with DB fallback and firmware-name validation. A stale DB model
  no longer changes the identity used when reconnecting an already-open clock.
- [x] Java/resource compile, existing unit suite 8/8, lint (0 errors,
  125 warnings) and installDebug passed. No firmware/BIN/version changes.
- [x] Static UI audit: scrollable page bodies reserve space for floating Save;
  labels/dialogs use wrapping and line spacing; navigation/move controls have
  adequate declared touch targets. No redesign or new interface elements.
- [x] Accessibility follow-up: danger_red now reuses the existing red token
  (#D96C5A, measured 4.76:1 on #0F2525 rather than 2.40:1). updateNavUI sets
  native selected state on all six tabs; phone XML confirms exactly one selected.
- [x] Owner unlocked the phone. Inspected all six tabs at font scales 1.0 and
  2.0, including scrollable page bottoms; Save remained visible on every page.
  Password dialog inspected and dismissed without changing credentials. All
  screenshots are ignored local Android .gradle/ui-audit-20261010 artifacts.
- [x] Large-font audit reproduced truncated bottom-tab labels. Their shared
  style now allows two centered lines, preserving normal-size icons/layout.
  Phone screenshot confirms complete labels at 2.0 (long Arabic words wrap).
- [x] Found and reproduced a startup crash while changing font scale during
  Splash: its delayed permission callback used an unregistered launcher after
  Activity destruction. checkAppPermissions now ignores finishing/destroyed
  instances. Repeated the same cold-start/font-change case: Main opened and no
  new AndroidRuntime fatal appeared. Existing unit suite 8/8, build/install and
  lint (0 errors/125 warnings) passed after the final code change.
- [x] Owner approved adaptive appearance rows and separators. At fontScale >=1.3
  the existing label sits above its full-width option group; normal text keeps
  the horizontal layout. Three shared SectionDivider views separate the four
  settings with 8dp margins. Checked radio state, listeners and save API unchanged.
  Phone width/Save assertions failed first on the old layout, then passed for
  all four groups at 2.0 (stacked) and 1.0 (horizontal); screenshots inspected.
  Java/resource build, existing tests 8/8 and lint (0 errors/125 warnings) pass.
- [x] Location selectors now place the label above the full-width Spinner at
  fontScale >=1.3; existing margins, selections and location-save logic remain.
  Real-phone assertions passed at 2.0 (stacked) and 1.0 (horizontal): four
  selectors, equal tab heights and visible Save. Temporary animator-duration
  changes were restored with the original font 1.0.
- [x] Settings adjustment inputs and the eight appearance choices have minimum
  48dp touch height. Search clear uses a 48dp hit area with unchanged 20dp glyph;
  text has sufficient end padding. No new UI elements or dependencies.
- [x] Bottom tabs retain 24dp icons and existing selection/separators. Tabs fill
  the same row height; large-font labels reserve two lines to align their icons.
  Normal-size layout is preserved. Main explicitly uses platform adjustResize;
  phone screenshot confirms Message input and Save above the visible keyboard.
- [x] Final Java/resources build, existing unit suite 8/8, lint (0 errors/
  125 warnings) and installDebug passed. No release APK exported.
- [ ] Landscape/tablet/TalkBack audio and large-font keyboard cases not tested.
- [x] Restored the phone's original font scale 1.0. No Save/OTA/factory reset,
  firmware changes, version bump or BIN publication in this UI audit.

### Android approved navigation icons and selection polish — 2026-10-10

- [x] Owner approved the six-icon preview. Main now uses six small native
  VectorDrawables: clock, iqama stopwatch, location pin, settings gear, text
  with pencil and cards grid. All keep the existing 24dp size and 2-unit stroke;
  no dependency, new tab or navigation behavior was added.
- [x] Selected tab background uses existing bg_main. The gold indicator moved
  from the top to a 2dp rounded bottom underline, inset 10dp horizontally and
  4dp vertically. Existing separators, press feedback and native selected state
  remain. Unselected icon/title share secondary_text rather than alpha-white.
- [x] Icons beside labels are decorative for accessibility; explicit null
  descriptions avoid duplicate descriptions and new lint warnings.
- [x] Java/resources, existing tests 8/8, lint (0 errors/125 warnings) and
  installDebug passed. Phone screenshot confirms the final dark background,
  gray inactive icons and bottom gold line. Time/iqama selection was inspected;
  200% phone assertions pass for equal tab heights and location Save visibility.
  Original font/animation settings restored. No Save/OTA/firmware/release APK.
- [ ] Landscape, tablet and four-tab M73 visual comparison remain owner-led.

### Android Git checkpoint and final firmware source audit — 2026-10-10

- [x] Android v2.9 saved in local Git: c356ccd (current UI/workflow/assets),
  then ec90766 (latest owner DB change; SQLite quick_check=ok). Only the local
  location_verify.png screenshot remains untracked. No push or APK export.
- [x] Current P10/M73 core contracts verified byte-identical (five headers).
  P10 four release sizes and M73/M73s builds pass. No upload, version bump or
  Android BIN publication occurred in this batch.
- [x] Audit findings and minimal follow-up recorded in
  docs/FIRMWARE_FINAL_REVIEW_2026-10-10.md. Real-source host checks reproduced
  negative temperature digit index and brightness-cache mismatch in M73;
  call tracing confirms prayer sleep is calculated but never rendered.
- [x] Fixed the bounded batch: M73/M73s temperature range and prayer sleep are
  safe/active, ultra brightness reapplies after exit, M73 HTTP now uses the
  reviewed P10 pre-allocation/multipart overlay, and the old runAsync call was
  removed. P10 no-op storage writes and repeated sleep-frame redraws are avoided.
- [x] Physical M73 sleep/brightness/OTA check and owner soak accepted by owner;
  final manual test completed without an unexpected reboot.
- [x] Historical intermittent P10 OTA reset accepted as passed by owner after
  the final manual test without an unexpected reboot.
  M71 remains deferred. Do not mark the whole project production-complete.

### Final owner acceptance — 2026-10-10

The owner confirmed the final manual test completed without an unexpected
restart. The firmware performance/memory review is closed for this batch.
