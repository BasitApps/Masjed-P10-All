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

### 2026-10-18 — P102 small-font layout and card selection

- [x] Narrow P10 clocks now render centered in the fixed top row; the lower
  row remains available for rotating cards.
- [x] In the Android card panel, P101/P102 small-font mode keeps only the
  fixed clock card out of the editable list. Day name, Gregorian date and
  Hijri date are visible, reorderable and independently disableable like the
  other lower-row cards. Wider P10 layouts retain their existing top-info
  policy.
- [x] P10 release builds for 1x1 through 4x1 pass. Android `testDebugUnitTest`
  passes. No firmware upload, APK build or hardware/settings change was made.
- [ ] Owner visual verification remains required on the real P102 display.

### 2026-10-13 — Preserve narrow-panel scroll phase on dynamic text

- [x] `SmartText` now preserves the normalized scroll-cycle position when an
  already-scrolling message changes, such as an iqama countdown changing from
  `13` to `12`; static-to-scrolling transitions and completed cycles still
  start normally.
- [x] Host regression covers 1x1, 2x1 and 4x1 widths in both directions and
  the default/explicit pixel-scroll paths. All four P10 release profiles build
  successfully. No firmware upload or settings/time change was performed.
- [ ] Physical owner verification remains required on the 2x1 and 1x1 panels;
  long text can still be clipped at the edges while it is moving by design.

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

### Comprehensive seven-segment source audit — 2026-10-11

- [x] Audited current `Masjed_TA_All_v2` M73/M73s main flows, shared contracts,
  display/audio, persistence, HTTP/OTA, and affected Android response handling.
- [x] Both release environments build with asset publication disabled; 25
  existing RTC/time cases, three C++ checks and three settings source checks pass.
  New host cases reproduce Friday audio restarts, truncated 120-minute iqama,
  stale prayer display, frozen test mode, incorrect sync age and TX-only MP3 waits.
- [x] Findings recorded in `docs/SEVEN_SEGMENT_AUDIT_2026-10-11.md`.
  Correction to the prior HTTP conclusion: the generated hardened header is
  not included by current M73/M73s source; both objects use stock Parsing-impl.
- [x] Audit-only round completed without firmware/Android source changes or publication.

### Approved seven-segment/P10 fixes and iqama 0–99 — 2026-10-11

- [x] Implemented audit findings 1–9 in the actual M73/M73s source and the matching
  P10 paths: hardened HTTP include, event-scoped audio, safe storage boot, RTC
  error prayer dashes, millis-paced screen test, TX-only MP3 and RTC sync age.
- [x] Both firmware APIs and the actual Android Studio UI/save path accept iqama
  0–99 only. Valid old 100–120 values migrate to99 without reset or layout/version
  change. Short 1/2-minute iqamas have explicit priority in the shared phase logic.
- [x] Six core headers now match byte-for-byte, including safe storage mounting.
  Rendering/audio-specific differences are retained; the prayer formula is unchanged.
- [x] Both seven-segment and all four P10 release builds succeed. Host verification:
  25 existing +14 new production-function cases, 11 multipart cases, 7 shared
  mount faults, 7 P10 real-struct persistence scenarios, MP3/Android map fixtures,
  three C++ suites and three settings source checks passed.
- [x] Android compilation and26 unit tests passed. Full lint remains blocked by
  three pre-existing missing English WhatsApp translations (120 warnings).
- [ ] Physical RTC/audio/display/save/OTA and long-run validation of this candidate.
  No hardware upload, Android BIN publication, APK build/install or release bump.
  Details and local runner locations: `docs/SEVEN_SEGMENT_AUDIT_2026-10-11.md`.

### Approved deeper firmware corrections — 2026-10-11

- [x] Closed all six findings from the second audit in actual P10/M73/M73s
  sources: pending-restart OTA protection, plain-body OOM rejection, verified
  M73 settings commit, whole-request multi-file OTA rejection, complete AP
  password reads and a versioned CRC32 M73 config envelope.
- [x] Independent review follow-ups fixed and tested: second-file OOM abort
  notification, status-aware password/startup lookups, failed reset compensation
  gating and time/prayer cache refresh on recovery. Final bounded source review
  found no remaining Critical/Important findings.
- [x] Host checks passed:27 firmware fault cases,16 actual-parser cases,39 prior
  RTC/prayer/API cases, three C++ suites, three settings behavior tests,7 P10
  persistence scenarios and7 shared mount faults. Existing MP3 packet/pacing,
  Android map/iqama fixtures and11 multipart cases also passed.
- [x] All four P10 release profiles and M73/M73s built successfully after their
  final source changes. Both M73 builds disabled Android publication explicitly.
  Six shared core headers and both parser overlays remain byte-identical.
- [x] M73 storage format2 retains four legacy layouts without reset/boot writes,
  upgrades on explicit save, and checks CRC before decrypt/migrate. Firmware
  release versions unchanged. Downgrade requires storage conversion/compatibility.
- [x] Report and build SHA256s: `docs/FIRMWARE_DEEP_FIXES_2026-10-11.md`.
  Test commands/requirements: `test/README.md`. No vendor library edit, hardware
  upload, Android BIN publication, APK build/install or new dependency.
- [ ] Physical RTC/save/OTA/display/audio, power-loss and long-run verification
  of this new candidate. Host/source/build checks do not close this release gate.

### P10 live iqama sequence check — 2026-10-11

- [x] Owner-approved `p10_4x1_release` uploaded to COM5; esptool flash
  verification passed and the board returned to `profile=4x1` with `rtc=1` and
  `timeValid=1`.
- [x] Controlled date/time jumps exercised pre-prayer (3 minutes), azan,
  countdown, last minute, iqama, sleep and azkar in both large and small font
  modes. Uptime increased throughout with no new reset; original time and
  settings were restored afterward.
- [x] Large-font countdown now uses `الإقامة بعد %d` and alternates with
  `الهاتف صامت`; small font keeps the minute wording. Post-prayer text is
  `لا تنس السنن` followed by `واذكار الصلاة`.
- [ ] Pixel-level visual confirmation still requires watching the panel because
  release firmware exposes health/state telemetry, not rendered-frame readback.
  M73/M73s physical test and power-loss/long-soak validation remain pending.

### Seven-segment quick solder inspection — 2026-10-12

- [x] Owner-approved 35-second display test implemented in sibling
  `Masjed_TA_All_v2/src/Masjed_TA.cpp` for both M73/M73s: one middle segment
  walks 16 grids (1s each), blank 1s, segments A–G walk all grids (1s each),
  blank 1s, then old digits 0–9 (owner requested 1s per digit). The three
  TA6932 chips run in parallel; test timing remains non-blocking and exits
  automatically without changing RTC/configuration/storage/version.
- [x] Production-function regression reproduces the old same-digit limitation,
  then passes the new isolated patterns, all three buffers, full-second timing,
  blank transitions, millis rollover and restarting the sequence.
  Runner: `python test/seven_segment_display_test.py`.
- [x] `ta73` and `ta73s` builds succeeded with `BASIT_PUBLISH_ASSETS=0`;
  existing firmware fault checks passed 27/27. Existing PrayerTimes unused
  static-declaration warnings remain. No hardware upload or Android publication.
- [ ] Physical inspection of patterns and observed solder-fault behavior.
  P10 shimmer experiments remain paused at the owner's request; the experimental
  millis-based P10 scan source/image was retained, not declared fixed or released.

### Seven-segment quick-test OTA — 2026-10-12

- [x] Owner-connected WiFi clock identified as M73-15, deviceId=15880401;
  pre-upload POST selfTest returned config/rtc/timeValid/display/wifi=1.
- [x] Uploaded only the tested ta73 image (412000B,
  SHA256 `2F0DBF19BBEE11CCC0068999ABDBE3AA07813BEDF93F1570EC5A46A220D57DE5`)
  via `/updateMain?size=412000`: HTTP200 UpdateOk. Health after AP returned
  showed Software/System restart, RTC/time valid, profile=7seg. Initial reconnect
  probe timed out; subsequent health/getData requests succeeded.
- [x] Complete getData response SHA256 matched before/after OTA without logging
  the AP password; settings unchanged. No P10/M73s/M71 upload or Android publish.
- [ ] Owner visual acceptance of the 35-second grid/segment/digit sequence.

### Seven-segment logical grid walk — 2026-10-12

- [x] Replaced the parallel electrical-index walk with one dash across the
  38 numeric grids used by `show()`: HH/MM/SS, DD/MM/YYYY, Fajr, sunrise,
  Dhuhr, Asr, Maghrib, Isha, temperature, iqama. Each numeric field walks from
  its highest place to units. Unused grids/separators are excluded from the walk.
  Current source maps all three Asr digits to tm2[3,2,4], not across tm2/tm3;
  no normal display wiring/mapping was changed to match an unverified description.
- [x] Full test now lasts 57 seconds: 38 single-grid frames, blank, seven segment
  frames, blank, digits 0–9. All frames remain one second; HTTP entry and normal
  RTC/configuration/storage/release versions are unchanged.
- [x] Production-function regression failed against the prior three-chip
  parallel walk, then passed the logical addresses, clearing of all other grids,
  complete second holds, two blank transitions, rollover and restart.
  Both ta73/ta73s builds passed with Android asset publication disabled.
- [ ] Owner visual acceptance of field order and solder-fault detection.
- [x] Live M73 deviceId=15880401 OTA returned HTTP200 UpdateOk. Uploaded ta73
  BIN=412096B, SHA256 `688BDB54F328A6C6CB17F6F283D2B91758514ADACD95E5BCA7E28817135D5CFD`.
  After reconnecting the saved WiFi profile, health showed normal software restart
  and selfTest config/rtc/timeValid/display/wifi=1. Full settings response fingerprint
  matched before/after OTA. No M73s/P10/M71 upload or Android asset publication.

### Seven-segment schematic-based visual correction — 2026-10-12

- [x] Read/rendered all five pages in the owner's three Date/Prayers/Eqama PDFs.
  Corrected dash traversal to left-to-right schematic positions: date now
  YYYY/MM/DD (tm1[13..6]), Eqama board S24/S23/S22/S21 (tm3[10,9,8,7]).
  Prayer rows and time order match the diagrams; Asr stays entirely on U2.
- [x] All test phases now keep unused GRID outputs blank. Physical segment A
  mirrors bit0 onto bit7 to support either JA/JH route; DP pins are NC in the
  diagrams. Segment/number tests corrected without altering normal show(),
  digits table, RTC, settings, storage, firmware versions or test entry.
  Duration remains 57s with 1s per number in the final phase.
- [x] Production-function test failed against previous order, then passed
  schematic traversal, wired-only buffers, both A routes, digits, timing,
  blanks, millis rollover and restart. Firmware fault checks passed 27/27;
  ta73/ta73s builds passed with BASIT_PUBLISH_ASSETS=0.
- [x] Wiring/reference table and caveats: `docs/SEVEN_SEGMENT_TEST_WIRING_2026-10-12.md`.
- [ ] Owner confirmation on the real front panel: schematic positions do not
  prove assembly orientation, actual jumper selection, or solder-fault behavior.
- [x] Uploaded only ta73 to M73-15 deviceId=15880401: HTTP200 UpdateOk,
  BIN412144B SHA256 `145C3DC88089F8F186FD921D6CB22AEC34269F41462A962A8936F3407E8BD694`.
  Saved-profile WiFi reconnect restored access; health/selfTest confirmed normal
  software restart, rtc/timeValid/config/display/wifi=1 and unchanged full
  settings-response SHA256. No P10/M73s/M71 upload or Android publication.

### Seven-segment date-first shorter test — 2026-10-12

- [x] Owner requested date before time and roughly half the dash traversal
  interval. Test now starts tm1[13..6] then tm1[0..5]; each dash holds 500ms.
  Blank separators, segment steps and final digits retain 1s each. Total 38s.
  Timing uses the previously displayed frame, so the final grid still holds
  exactly 500ms before the first blank; the blank holds 1s before segment A.
- [x] Actual-function regression failed on prior order, then passed date-first
  traversal, 500ms/1s boundaries, all existing patterns, rollover and restart.
  Firmware fault checks passed 27/27. Wiring notes/test README updated.
- [ ] Real-panel confirmation of the faster sequence.
- [ ] Splash redesign is advice only, not implemented: current startup has
  2450ms of blocking dash-group delays and a 6000ms contact-number deadline.
  Owner asked for an opinion, not a splash implementation in this batch.
- [x] Both ta73/ta73s built successfully with Android publication disabled.
  Live M73 deviceId=15880401 accepted HTTP200 UpdateOk for BIN412144B,
  SHA256 `4891E6536302B7048693DF1F84D48BACA92E892DC6DD37D3FBB60CE7156F769A`.
  Saved-profile WiFi reconnect, healthy RTC/time/selfTest and unchanged settings
  fingerprint confirmed. No P10/M73s/M71 upload or splash change.

### Seven-segment startup identity beside contact — 2026-10-12

- [x] Owner approved retaining contact `01030007330` while adding model/release
  during the same six-second startup hold. Existing tm1 contact bytes are
  unchanged. Model uses tm3[8,7], release uses tm3[10,9] in schematic order.
  Values derive from FIRMWARE_VERSION, so M73 shows 73/15 and M73s shows 73/14.
- [x] New digits normalize physical A for either JA/JH jumper. Two-digit release
  format is guarded at compile time; extend the layout before release 100.
  No changes to normal show(), settings/RTC, test sequence, version numbers,
  startup group animation or its existing delays.
- [x] Actual-function runner failed on the prior blank tm3 startup frame, then
  passed for both variants: unchanged complete phone frame, model/release,
  published buffers, 2450ms existing animation and six-second hold/rollover.
  Existing 38-second display sequence and 27 firmware fault cases also passed.
- [ ] Owner visual confirmation of simultaneous phone/model/release at boot.
- [x] Both ta73/ta73s builds passed with BASIT_PUBLISH_ASSETS=0. Uploaded only
  ta73 to M73-15 deviceId=15880401: HTTP200 UpdateOk, BIN412208B SHA256
  `B0BE97096B558BE2786296808B205234C4DE3C39F3345DFBAE005C6FCF5BB079`.
  After saved-profile WiFi reconnect, health/selfTest showed normal software
  restart, rtc/timeValid/config/display/wifi=1 and unchanged settings fingerprint.
  No additional restart, display test, M73s/P10/M71 upload or Android publication.

### Seven-segment startup number direction correction — 2026-10-12

- [x] Owner observed startup model/release as 37/51. The assembled front panel
  reverses the Eqama schematic's drawn digit order; normal show() already uses
  tm3[7,8] for temperature and tm3[9,10] for iqama. Startup now uses that order
  for model 73 and release 15 (14 on M73s). No phone, animation, hold duration,
  accepted dash-test traversal, normal display, settings/RTC or version change.
- [x] Actual-function test first failed on the previous swapped frame, then
  passed both M73/M73s variants and existing test timing/phone/published-frame
  assertions. Firmware fault checks passed 27/27. Reference notes corrected.
- [ ] Owner visual confirmation of 73/15 after the correction.
- [x] Both ta73/ta73s builds succeeded with Android publication disabled.
  Corrected ta73 BIN412208B SHA256 `EE20B9C08C3E0FF17B1233BF6767A0C784313C651BC3F2453C455D81E0680AA3`
  accepted by M73-15 deviceId=15880401: HTTP200 UpdateOk. Saved-profile WiFi
  reconnect restored access and unchanged complete settings fingerprint confirmed.
  Post-OTA RTC responds (rtc=1), but timeValid=0 and selfTest ok=0: time validity
  is NOT confirmed and must not be reported healthy. No date/time write performed.
  No P10/M73s/M71 upload or Android publish.

### Seven-segment splash dash-only and full-brightness behavior — 2026-10-12

- [x] Removed the A–G phase from startup. The nonblocking splash now renders
  only the nine date-first dash groups in about 1953ms, matching the previous
  total startup-animation time, then keeps the existing phone, model and
  release frame for six seconds.
- [x] Splash no longer exits early when configuration, RTC connection or RTC
  time validation is incomplete. Wi-Fi/server servicing and periodic RTC
  checks continue normally; the existing error renderer is shown afterward.
- [x] Brightness is forced to hardware level 7 for the complete splash and the
  scheduled/user brightness path is restored immediately before normal display.
  Both ta73 and ta73s display tests pass, firmware fault checks remain 27/27,
  and both PlatformIO release builds succeed. Firmware was not uploaded yet.
- [ ] Owner visual confirmation on the real M73 board, followed by cold-power
  and long-run checks.

### Seven-segment immediate identity startup — 2026-10-12

- [x] The phone/model/release frame is now published immediately after the
  three TA6932 chips initialize, before LittleFS, RTC, sensor and Wi-Fi setup.
  On M73 the startup buzzer begins with that frame.
- [x] The identity frame remains for six seconds while setup completes. Normal
  brightness is intentionally not applied during the hold; it is restored by
  the existing splash exit path. The manual dash test and normal runtime
  display remain unchanged.
- [x] Added an early-identity regression for both M73/M73s, including full
  brightness, six-second hold, configured-brightness restoration and rollover.
  Existing firmware fault checks remain 27/27; both release builds succeed.
- [ ] Upload this candidate and visually confirm that identity and buzzer begin
  together; then run cold-power and long-run checks. If setup ever exceeds six
  seconds, it must be split into loop states.

### Seven-segment nonblocking startup animation — 2026-10-12

- [x] Approved startup redesign: all 38 wired numeric grids cycle A–G at 150ms
  per frame, then nine dash groups at 100ms (date, time, Fajr, sunrise, Dhuhr,
  Asr, Maghrib, Isha, Eqama/temperature). Nominal animation 1950ms; unchanged
  phone 01030007330, model73 and actual release follow for a full 6000ms.
- [x] splashScreen now advances cooperatively from loop after RTC/AP/server
  setup, without delay or added storage writes. The same PROGMEM grid table
  serves manual test and startup. RTC polling continues at most once/second;
  config/RTC/time faults abort to existing normal error rendering. OTA/restart
  guards avoid display interference. Removed obsolete unused-grid boot marker
  and absolute-time expiry; elapsed timing works across millis rollover.
- [x] Regression first caught the blocking old splash, then passed both variants:
  exact frames, unused grids dark, both A routes, per-frame holds, complete contact
  and correctly oriented identity, fault handoff, RTC polling, OTA/restart guards
  and rollover. Manual 38-second test unchanged; firmware fault checks 27/27.
- [ ] Owner visual confirmation, controlled cold-power retention and long-run
  stability. These are not proved by host checks or one successful OTA.
- [x] Independent review reproduced a shortened contact hold when RTC/display
  operations take time. Timer anchoring moved after all three display updates;
  regression first failed with 100ms RTC +2ms/chip latency, then passed for both
  variants. Fault/OTA guards now tested during animation and contact hold.
- [x] Independent bounded recheck found no remaining Critical/Important issues.
  Final ta73/ta73s builds passed with Android publication disabled. Uploaded
  ta73 only to M73-15 deviceId=15880401: HTTP200 UpdateOk, BIN412048B SHA256
  `354C00B4D2262E1664D0A9F0302F2DBACD8E3397A716259FCF60C51CBF17B83C`.
  After saved-profile WiFi reconnect, health/selfTest confirmed normal software
  restart and config/rtc/timeValid/display/wifi=1; settings response SHA unchanged.
  No additional restart/time write, P10/M73s/M71 upload or Android publication.

### P10 splash timing and RTC startup sample — 2026-10-12

- [x] Kept the existing company-title and `01030007330` splash unchanged;
  anchored its ten-second wait to the moment the frame is actually presented,
  then continue normal initialization and handoff.
- [x] Removed the duplicate immediate RTC read in `setup()`; `startRTC()`
  already performs the first validated sample. Runtime sync remains every five
  minutes, with hour-change/long-stall and 30-second recovery paths unchanged.
- [x] Startup contract test and `p10_4x1`/`p10_4x1_release` builds pass.
- [x] DEBUG uploaded on COM5 with flash hash verified. Serial comparison:
  Wi-Fi on `loopMax=32111us`, stable Wi-Fi off `8488us`; transitional off
  `112558us` excluded. W/A DEBUG commands disable/restore the AP and inhibit
  automatic recovery during intentional off; AP was restored afterward.
  Loop duration is a proxy, not a direct measurement of scan jitter.

### P10 DP5125D timing experiment — 2026-10-12

- [x] Saved pre-experiment DEBUG BIN and driver source outside the repo.
  Added isolated `p10_4x1_dp5125d` profile for timing trials; latest candidate
  retains only explicit OE blanking/row transition, with SPI and LAT after
  data as before. Normal build profiles retain baseline.
  No scan ratio, SPI speed, brightness setting, storage or prayer logic change.
- [x] Real-refresh host check preserves bit order across 1-4 panels, both
  orientations and all four phases, checks blanking, LAT pulse and SPI modes.
- [x] Baseline and experimental builds succeeded; original BIN rebuilt to
  identical SHA256. Uploaded experiment to ESP8266 MAC24:4c:ab:4f:ed:96 on
  COM5 with flash hash verified. Owner reports shimmer gone but dim stray
  pixels around characters. Serial also captured Hardware Watchdog after
  `loopMax=9760us`; cause is undetermined.
- [x] Revised OE blanking to cover LAT falling plus 1us settle. Real-refresh
  check catches premature enable, then passes the revised path. Rebuilt and
  uploaded second candidate (431152B, SHA256
  `3041734952485FE5FB68179C08A225A55CA9B6022B88051954450FD5B2A4566D`).
- [x] Owner reports second candidate's stray pixels persist but glyphs remain
  steady. No reset in 65s serial capture, `loopMax=9798us` (not soak proof).
  Third trial removes overlap/GPIO byte transfer, retaining explicit OE high
  until LAT/data settle. Host check and experimental build pass; candidate BIN
  430928B SHA256 `26C38CA73A9200D3BBE70E78A10F69F3B089158865E0E1D4AC3E0ECBE128C4F1`.
  Upload at512000 failed before writing; retry at115200 succeeded with flash
  hash verified on the same MAC/COM5.
  Its 65s serial capture recorded `loopMax=8704us` and no observed restart.
- [x] Owner reports third candidate's stray pixels much reduced but not gone,
  and glyph shimmer returned. All three candidates rejected. Removed temporary
  profile/driver flag and archived the experimental test outside the repo;
  restored the saved pre-experiment driver/BIN under the owner's rollback
  authorization. Existing splash/RTC/debug Wi-Fi changes are retained.
  Driver text matches saved snapshot; `p10_4x1` build succeeds and reproduces
  SHA256 `9F46E6DF2F4BCA0071BB7E87754DABF16E8F5623C05CD6A3C2F0D64D75FE9230`.
  Original430896B uploaded on COM5 at115200 to the same MAC, flash hash verified.
  Controlled RTS reset logged External System, Web server started, RTC OK and
  Setup done; no extra restart in the short20s boot check.
- [ ] Root-cause scan/OE waveform capture and long-run reset investigation;
  no DP5125D timing change is approved for production.

### P10 scan timing diagnostics — 2026-10-12

- [x] Added isolated `p10_4x1_scan_diag` measurements with the original driver
  timings. Scan LAT gaps/duration and service/yield/HTTP/render maxima report
  every10s; intentional pause and diagnostic-print intervals are excluded.
- [x] Real-source host check passes: unchanged GPIO/SPI ordering with hooks
  on/off,1-4 panels, both orientations/all phases, gap/duration aggregation,
  pause exclusion and micros rollover.
- [x] Builds succeeded and diagnostic431488B BIN uploaded on COM5 at115200,
  flash hash verified (SHA256 `3D25C7B0214B0078F532C66614A912FE14D27378AA0C401E58386601EE36FE83`).
  Corrected DEBUG A to restart HTTP after restoring AP; W had stopped it.
- [x] First idle comparison had no peers: active-AP gapMax3.638-4.417ms,
  stable OFF3.080-4.327ms, zero gaps over5ms. Render loop maxima6-8ms do not
  imply that scans stop that long because SmartText scans inside rendering.
- [x] Repeated with phone peer1: four10s windows recorded gapMax33.933,
  33.520,3.606,43.561ms; HTTP maxima25.102,24.979,0.084,39.216ms.
  Counts over5ms9,4,0,2. Stable OFF gapMax3.691,3.459,4.211ms, HTTP<=34us,
  scans414-440us, no intervals over5ms. W/A transition intervals are excluded.
  AP/HTTP restored. These are software timings, not electrical OE waveforms.
- [x] SDK3.1.2 WiFiClient::flush waits for remote TCP ACK. Reply-latency host
  check failed on current sendV2, then passed after removing its redundant
  explicit flush (payload/status/content-type unchanged).
- [x] Reply fix uploaded with flash hash verified (431456B SHA256
  `A11CB688064561D74AEBD94BDC1151EB04ED3EA639F18105F265C0D373A325C2`).
  Phone connected and modification succeeded; owner reports no visual reduction
  in shimmer. Measured connected gaps28.244,20.575,17.086,17.404ms with HTTP
  18.403,11.091,10.226,9.059ms. Different request activity limits direct percentage
  comparison. Stable OFF gaps3.370-3.882ms had no over5ms intervals. AP restored.
- [x] Added build-only api/send/storage maxima and worst API path. Diagnostic
  431808B SHA256 `D5170C6906DFF15D33E387D45813BE47044255ECAF4C109B650A51FEB9F8D4E8`
  uploaded/hash verified. Its90s passive capture had peers0, so it did not
  isolate a live API request. Release still reproduces the same reply-fix BIN
  SHA256 `2748ADACB71162D759DBEFF7E344ADCB403D71DAA37A2F886746ED8EAB3B47D8`.

### P10 SDK timer scan experiment — 2026-10-12

- [x] Confirmed old P170 setup calls dmdInit, which attaches timer0 ISR to
  dmd.refresh and rearms40000cycles after each scan. Current foreground scans
  cannot run during blocked HTTP calls; this is an architectural difference.
- [x] Added isolated `p10_4x1_sdk_scan`: per-instance SDK software timer at1ms,
  unchanged SPI/data/row/LAT sequence. PWM setup moves to foreground startup,
  brightness updates and resume, because Core3.1.2 waveform setup may yield.
  Timer callback does not call PWM APIs, Serial or network functions. Pause
  disarms and blanks; resume restores current brightness; destructor disarms.
  This is a software timer, not a hard-real-time ISR or guarantee against flash
  stalls. Original/release profiles retain foreground scanning.
- [x] Real-source host checks pass for timer lifecycle/pause/resume, brightness
  caching, no waveform setup inside callback, and identical data/row/LAT handoff
  across modes (PWM setup is intentionally excluded from sequence comparison).
- [x] SDK candidate built/uploaded with flash hash verified (431936B SHA256
  `2F76B462ED247D960CAFFFDF1F60D42EADD2C1FC77BF80C7340D1E0D24FFAF97`).
  In100s capture, idle scan gaps2.992-4.170ms; active setDateTime gap14.926ms,
  HTTP26.911ms; getData gap6.176ms, HTTP14.063ms. No observed restart in this
  short capture. Owner reports shimmer persists, no stray pixels, phone works.
- [x] SDK experiment CLOSED/REJECTED: no visual improvement. Stop experiments
  following owner feedback about excessive testing time. Its isolated source
  profile remains experimental only; do not publish or enable it by default.
  Restored foreground-scanning `p10_4x1_release` without DEBUG/scan logs on COM5
  at115200; upload completed and flash hash verified. BIN427424B SHA256
  `2748ADACB71162D759DBEFF7E344ADCB403D71DAA37A2F886746ED8EAB3B47D8`.
  Retains splash/RTC changes and the redundant reply-flush removal. No Android
  assets, Git publication, settings/RTC writes or extra timing trial.
- [ ] Shimmer remains unresolved; electrical OE/CLK capture and long-run
  validation have not been performed. No claim of a hardware root cause.

### P10 whole-screen clearing hypothesis review — 2026-10-12

- [x] Traced clearDisplay/displayText/drawTopClock/drawTopInfo and SmartText to
  Bitmap writes, buffer presentation and hardware refresh. Small-mode scrolling
  area is y8..15; clock/info use separate top zones. Unchanged static text is
  drawn once, not on every scrolling step. Full clear is for explicit transitions,
  not each scroll/tick. setup enables double buffering; refresh reads displayfb,
  while clear/draw writes fb, and completed frames swap/copy at presentation.
- [x] Added/passed real-source host `p10_text_region_test.py`:1-4 panels,
  Arabic/English shaping and both scroll directions,400 scroll steps per case,
  static/change/empty/flash preserve top pixels and rendering refresh calls do
  not expose an incomplete back frame. Rasterization uses a stand-in canvas;
  this is not a physical flicker test or proof of board buffer allocation.
- [x] No firmware/library/default change or hardware upload: suggested regional
  clearing/back-buffer isolation already exists. Redraw-related CPU/scan/PWM
  timing remains a different hypothesis, not confirmed by this host check.

### P10 packed-pixel scrolling probe — 2026-10-12

- [x] Owner approved isolated shift-and-incoming-column probe. Existing
  production BIN/source backed up outside repository before edits.
- [x] Added flag-only SmartText path and `p10_4x1_pixel_scroll` profile; no
  driver, scan/OE/PWM/SPI cadence, brightness, prayer/storage/RTC/API change.
- [x] Test first failed for full glyph redraws during movement, then passed
  103168 byte-identical real-font frame/done samples: eight fonts,1-4 panels,
  Arabic/English, wrapping, changed/static/empty/flash. Existing host checks
  pass: firmware faults27/27, HTTP parser16/16, prayer wording, scanner/SDK,
  reply latency, capture cleanup, startup3/3, region isolation and both M73
  display fixtures. No physical test is represented by these results.
- [x] Both builds succeeded. Normal release remains byte-identical to saved
  original SHA256 `2748ADACB71162D759DBEFF7E344ADCB403D71DAA37A2F886746ED8EAB3B47D8`.
  Probe428320B SHA256
  `B553867E78EC43AEB33EDA9B341D7A850348E04C961618DAE4815306B7D3D7D6`
  uploaded at115200 on COM5, ESP8266 MAC24:4c:ab:4f:ed:96; flash hash verified
  and normal RTS reset requested. No Android assets/version or settings writes.
- [x] Owner reports shimmer reduced very substantially, intact letters and
  successful phone connection/settings changes. These are physical owner
  observations, not a quantified render/scan-latency measurement.
- [x] Accepted as default P10 text path after owner confirmation:
  `SMARTTEXT_PIXEL_SCROLL=1` in constants (override0 retains old rendering).
  Removed redundant experiment profile. Native comparison now covers default,
  explicit1 and explicit0; the default-path assertion first failed before
  adoption. No storage/API/brightness/prayer/scanner changes or new upload.
- [x] Default/on/off real-font comparison passes with the same103168-frame
  signature; region/prayer/startup checks also pass. Standard4x1 release build
  succeeds and reproduces the exact uploaded probe BIN SHA256 `B553867E…`,
  so no second hardware upload is needed.
- [ ] Long-run/reset stability and remaining shimmer are not proved resolved;
  physical confirmation applies to the connected4x1 panel, other sizes are
  covered by host pixel comparisons, not physical tests.

### P10 current-glyph lookup cache — 2026-10-12

- [x] Owner approved reducing repeated text/font lookup in incoming-column
  rendering. Saved owner-tested428320B BIN/source outside repo in
  `p10-glyph-cache-baseline-20261012`, SHA256 `B553867E…` for rollback.
- [x] Cache current text span and GLCD glyph address/width/pages. Adjacent
  columns reuse them; crossing a character/spacing span or changing font
  resolves a new span. reset/text changes invalidate it; direction/shaping
  changes already call reset. No per-frame heap allocation or framebuffer.
- [x] Adjacent-column test first failed for repeated width lookups, then
  passed. Default/on/off real-font comparison passes123648 identical frame/
  done samples across8 fonts,1-4 panels, Arabic/English, wraps, changes,
  empty/static/flash and reset/font/direction/shaping transitions.
- [x] All existing host scripts pass: firmware faults27/27, parser16/16,
  prayer wording, scan/SDK, replies, capture, startup3/3, region and M73 display.
  Standard4x1 release builds: RAM37436B (+24B), Flash424291B (+128B).
  BIN428448B SHA256 `C9B25E2A0D357BB1C6BD127CA3CD9EF87400C9BCBA56F48DBD9B58861DD193C2`.
- [x] Uploaded candidate428448B on COM5 at115200 to MAC24:4c:ab:4f:ed:96;
  flash hash verified and RTS restart requested. No time/settings writes.
- [x] Owner reports shimmer now very slight except a small twitch at initial
  entry of scrolling/static text; letters and phone/settings remain correct.
  This accepts the cache improvement, not a claim of zero shimmer.
- [x] Traced first-frame work: updateText shapes/measures and clears;
  first scrolling draw clears again and visits glyphs while fully offscreen.
  Owner approved the bounded entry optimization below. Physical timing and
  long-run/reset stability remain unproved.

### P10 first-frame rendering work — 2026-10-12

- [x] Saved prior owner-tested cache BIN/source externally in
  `p10-entry-baseline-20261012`, SHA256 `C9B25E2A…` for rollback.
- [x] Full-width, byte-aligned, in-bounds text regions clear with packed
  0xFF row bytes; other regions retain clipped Bitmap clearing. Omit repeated
  clear in the first scrolling draw (updateText already clears). Hidden left
  glyphs resolve width only, not bitmap offsets/pixels; placement, gap handling,
  speed, done flags and existing yield/scan servicing remain unchanged.
- [x] Entry regression first failed on pixel clearing, then passed; fallback
  entry clears once.124368 actual-font frame/done samples match across default/
  on/off paths, eight fonts and1-4 panels, including partial/negative/overflowing
  regions. All other existing host scripts pass, firmware faults27/27 and
  parser16/16. These are not electrical/physical latency measurements.
- [x] 4x1 release builds: RAM37436B unchanged, Flash424371B (+80B).
  BIN428528B SHA256 `F3D0A4677CC9AA48AC96FAD400B5C571B7435121B2F04D7A8B71EF074C5C9A83`.
- [x] Uploaded428528B to MAC24:4c:ab:4f:ed:96 on COM5 at115200; flash hash
  verified and RTS restart requested. No time/settings writes.
- [x] Owner confirms entry twitch disappeared and letters/phone settings are
  correct on this candidate. Accept the display optimization sequence on the
  connected4x1 board: packed scrolling, current-glyph cache and entry work.
  Retain current BIN SHA256 `F3D0A4677CC9AA48AC96FAD400B5C571B7435121B2F04D7A8B71EF074C5C9A83`.
  No further source changes/upload from this feedback, nor scanner/PWM,
  brightness, storage/RTC/prayer/API, version, Android or Git publication.
- [ ] Long-run/intermittent-reset validation and physical testing of other
  panel sizes remain separate release gates. Owner's short visual confirmation
  is not an electrical timing measurement or proof of whole-device stability.

### P10 12-hour soak/reset observation — 2026-10-12 (COMPLETE)

- [x] User requested long-run/reset test and connected PC to clock Wi-Fi.
  Confirmed live health4x1/rc4, RTC/time valid, heap36504B, reset reason External
  System from the existing uploaded candidate; no intentional restart.
- [x] Added read-only collector and5 passing host checks (parsers/uptime/ROM,
  DTR/RTS-before-open and simulated full-loop result). Existing4x1 release
  rebuild succeeds; same428528B SHA256 `F3D0A467…`, no firmware edit/upload.
- [x] The first collector attempt stopped after about49minutes; it was incomplete
  and showed no reset before stopping. It was not used as the release result.
- [x] Repeated the test from2026-10-12T03:40:05.603747Z to
  2026-10-12T15:40:05.603747Z (06:40–18:40 Africa/Cairo) against the exact
  4x1 BIN SHA256 `F3D0A4677CC9AA48AC96FAD400B5C571B7435121B2F04D7A8B71EF074C5C9A83`.
  The collector completed the full43200.2seconds:1366 Health samples,139
  successful selfTests, zero HTTP failures, zero selfTest failures, zero ROM
  boot markers, zero uptime drops, zero serial errors and RTC/timeValid healthy
  in every returned sample. Minimum heap36160B and maximum fragmentation4%.
- [x] One observation gap of about1650seconds (27.5minutes) occurred. The
  uptime increased across the gap (25490→27165), so no reset was indicated;
  however, the gap means the collector correctly returned `review_required`,
  not an uninterrupted `no_reset_observed` result. Network failure alone is not
  reset proof; ROM and uptime can be two sources for one reset.
- [x] Final report delivered. The heartbeat follow-up is being disabled after
  this completion. No firmware, Android asset, Git, OTA or settings-write
  publication was performed by this soak test.
- [ ] Physical power/signal integrity, other panel sizes, and OTA/settings-write
  stress remain unverified release gates.

### Editable Tajawal Bold 16px asset refinement — 2026-10-14

- [x] Rebuilt `GLCDFontCreator2/Fonts/TajawalBold16.ftsd` from the owner's
  `Tajawal-Bold.ttf` using RAQM/OpenType contextual shaping at high resolution.
  Replaced vertical clipping with shared optical bands (3 accent / 10 body /
  3 descender rows); preserved the 224-slot layout and nonjoining side bearings.
  Corrected all eight lam-alef forms: the preceding generator made only two
  unique bitmaps by using plain lam+alef for every variant.
- [x] Loaded through the editor's `SerializableFont` → `Font` path; all 224
  glyphs round-trip pixel-for-pixel against the staged header, with eight
  distinct lam-alef bitmaps and 805 connected initial/final edge-pair checks.
  Comparison, generator and previous FTSD backup are outside the repository in
  the task's `tajawal-refinement` visualization folder. No firmware integration.
- [ ] Owner visual approval and real-panel confirmation of this new font.

### P104 Tajawal large-font USB trial — 2026-10-14

- [x] Owner requested upload to USB-connected model104. Verified ESP8266 MAC
  `24:4c:ab:4f:ed:96` on COM5 and saved its first1MiB flash outside the repo
  in `tajawal-refinement/p104-upload/flash-before-font-trial.bin` for rollback.
- [x] Added opt-in `p10_4x1_tajawal_trial` profile. Only Basit/family0 large
  uses Tajawal; small and Tahoma stay as configured. Indic variant preserves
  current Basit numeral artwork. Exported glyphs match current editable FTSD.
- [x] Real Bitmap/SmartText comparisons pass155280 frames per mode across
  ten fonts (including Tajawal),1-4 panels, Arabic/English and dynamic changes.
- [x] Trial build and COM5 upload at115200 succeeded; esptool verified flash
  data hash and requested normal RTS reboot. RAM38572B, Flash434963B;
  BIN439120B SHA256 `8C4F634E605C52C698582A50ACF06ABCCEEB9E76E0645DACBBBBFB9CB1DC9001`.
  Uploaded BIN archived beside rollback. Version/storage layout unchanged;
  no Android asset or Git publication. PC could not reach clock HTTP, so no
  post-upload RTC/settings/API health claim is made from this run.
- [ ] Owner screen evaluation of Basit/family0 large text on this trial image.

### Editable Tahoma Bold 16px asset — 2026-10-14

- [x] Owner judged the Tajawal physical trial somewhat acceptable and requested
  a Tahoma Bold font. Created independent `GLCDFontCreator2/Fonts/TahomaBold16.ftsd`
  from installed `C:/Windows/Fonts/tahomabd.ttf` (verified family Tahoma/Bold).
  High-resolution OpenType extraction uses the shared16-row optical grid;
  existing Tahoma runtime fonts and the uploaded Tajawal trial remain unchanged.
- [x] Source covers all219 unique required codepoints. All224 FTSD glyphs
  load via the real editor classes and match staged pixels exactly;805 joining
  pairs connect and all8 lam-alef variants remain distinct. Preview/generator
  are external in the task's `tahoma-bold16` visualization folder.
- [x] Owner requested USB upload. Extended the existing opt-in P104 font trial:
  family0 large remains Tajawal; family1 large now uses Tahoma Bold, with the
  owner's existing Tahoma Indic digits retained. Small and normal release
  profiles keep their original selection. Runtime bitmaps match editable FTSD.
  Real-renderer checks pass186192 frames per mode across12 fonts and1-4 panels.
- [x] Build and upload to COM5/MAC24:4c:ab:4f:ed:96 at115200 succeeded;
  flash hash verified and normal RTS reboot requested. RAM38572B, Flash444931B;
  BIN449088B SHA256 `57F246CEE7D0054FD2A6C6A75EC5CBA0602F8F33870F87EAE768C2DCBE9E396A`.
  BIN archived externally in `tahoma-bold16/upload`; earlier Tajawal BIN and
  pretrial flash remain available for rollback. No settings/time, version,
  Android asset or Git publication changes; post-upload HTTP was not checked.
- [ ] Owner's physical-panel comparison of Tahoma Bold versus Tajawal.

### Tahoma Bold dot/hamza refinement and adoption — 2026-10-14

- [x] Owner requested further letter/connection cleanup and replacement of old
  Tahoma16, then identified missing dots and letter/join shape issues. Compared
  original OpenType masks with the trial:32 Arabic forms merged detached marks.
  Separate rasterization and optical placement now preserve source-component
  counts in all127 Arabic forms;10-row main bodies stay bold, with lower-dot
  clearance where needed. All2852 initial/medial/final/lam-alef edge pairs pass;
  SmartText adds no Arabic inter-glyph gap. No representable source
  counters are missing. ASCII/digit artwork remains as previously configured.
- [x] Updated `TahomaBold16.ftsd` and added editable Indic sibling. Both load
  through the editor and match runtime headers pixel-for-pixel. Adopted Bold
  for family1 large in all normal builds; Tahoma7 is unchanged and Tajawal is
  still opt-in for comparison. Updated import/export/helper/test consumers.
  Removed old runtime `Tahoma16.h`/`Tahoma16Indic.h`; archived their previous
  source FTSD/export files outside the repo in `tahoma-final/retired-fonts`,
  with runtime backups in the enclosing folder. These remain recoverable.
- [x] Import validates all8 active font exports. Real renderer passes155280
  frames per mode across10 active/trial fonts and1-4 panels; release4x1 and
  comparison4x1 builds pass. Release431440B SHA256
  `8177A850A8A8109987B2ADC09E8F3ABEEAE9A5DB5246A5BD47AB95746B5371A0`.
- [x] Uploaded comparison441648B (RAM38572B, Flash437491B) on COM5 at115200
  to MAC24:4c:ab:4f:ed:96; flash hash verified, normal RTS reboot requested.
  SHA256 `9F9E4E4923FCB7F7EC0DEAF90B829289D119E2572FC6F1E59802C9899B8E6D44`.
  No version/settings/time, Android asset or Git publication changes.
- [ ] Owner confirmation of refined glyphs on the actual panel; other hardware
  sizes and post-upload HTTP health were not checked in this font-only batch.

### One-shot P104 Arabic font bench — 2026-10-14

- [x] Owner approved a temporary visual test for Arabic dots/joins. Added
  opt-in `p10_4x1_font_visual_test`, inheriting the existing comparison build.
  After splash,18 centered static Tahoma Bold16 pages hold6s each: similar
  isolated letters/hamzas/lam-alef, then contextual words. No configuration,
  time, font artwork, version or storage-layout writes. Normal build excludes
  this path. During108s bench display, normal cards/fault/prayer indications
  are suppressed; RTC/software ticks, server/Wi-Fi and OTA guards continue.
  Completion restores configured font/layout and card entry once; reboot
  repeats the temporary test. This image is not for mosque deployment.
- [x] New real ArabicShaper/Bitmap/SmartText host check first failed because
  sequence was absent, then passes18 static128px pages,6s pixel holds,
  one-shot completion/restoration and rollover. Default4x1 and bench build
  pass; bench RAM38588B (+16B), Flash438899B. Existing renderer124368 frames
  per mode, startup3, firmware faults27, parser16, scan diagnostics/SDK/reply/
  capture/region, soak5 and both7-segment host variants pass.
  `p10_prayer_sequence_test.py` remains FAILED on a pre-existing text fixture:
  expected `لا تنس السنن واقامة الصلاة`, current owner source is
  `لا تنس السنن واذكار الصلاة`; neither wording nor fixture was changed here.
  Soak check was rerun successfully with PlatformIO Python (pyserial), after
  system Python lacked that dependency. Existing library compiler warnings
  remain; no new dependencies installed.
- [x] Uploaded443056B to COM5/MAC24:4c:ab:4f:ed:96 at115200; esptool verified
  flash data hash and requested RTS reboot. SHA256
  `616817D99F1952C9ED267493143C0563086EAC4BA55A4B38CF08F35845E06DAE`.
  Archived externally in `font-visual-test-20261014/P104-font-test.bin`.
  Previous comparison/release backups remain. No Android or Git publication.
- [ ] Owner observation of specific missing dots/bad joins on physical panel;
  post-upload API/RTC health was not checked in this USB font-test batch.

### Tahoma small7px optical refinement — 2026-10-14

- [x] Owner requested small Tahoma cleanup. Source comparison found73 Arabic
  forms with merged/missing marks or fragmented bodies in the prior bitmap.
  Refitted real Tahoma outlines to a common row4 joining rail, separately
  hinting dots/hamzas, preserving bowls and keeping marked edges away from
  neighbouring glyphs. Seven rows retained; no whole-font dilation. Hand
  hints keep alef stems thin and jeem/hah/khah final bowls open at this scale.
- [x] Updated runtime/export `Tahoma7.h` and `Tahoma7Indic.h`, and editable
  `GLCDFontCreator2/Fonts/Tahoma7.ftsd` / `Tahoma7Indic.ftsd`. Original ASCII,
  Western/Indic digit artwork retained byte-for-byte. No large-font, shaper,
  rendering, settings/time/version or storage changes. Backups, reproducible
  optical-fit scripts and preview are external in task folder `tahoma-small`.
- [x] New font checks first fail on old merged madda, then pass127 forms,
  detached marks/open bowls,8 distinct lam-alefs and2944 joined pairs per
  variant (zero neighbouring-mark merges), plus export/Indic parity.
  Real editor loads all224 seven-row glyphs. Import validates all8 exports.
- [x] Real FTSD/editor-export roundtrip exposed a pre-existing single-page
  packing bug:7px export shifted bitmap rows left by one bit. Corrected only
  `FontExport` packing for heights below8; multi-page behaviour unchanged.
  Roundtrip now passes both small/large Tahoma variants. Recompiled local
  editor class; restart an already-running editor to load the correction.
  Small-font helper outputs now target the existing `Fonts` directory.
- [x] Normal1x1/4x1 release builds pass. P104 RAM38572B unchanged,
  Flash427459B (+176B); BIN431616B SHA256
  `0B72DDD579C3F1AAB90FD444A9C322EF29DFD19D1FDB00B0EED41DB12BE26BA0`.
  Real renderer124368 frames per mode across8 fonts/1-4panels, font bench,
  startup, firmware faults27/parser16, scan/SDK/reply/capture/region, soak5
  and7-segment host checks pass. Existing `p10_prayer_sequence_test.py`
  still fails its outdated `واقامة الصلاة` text fixture; owner source remains
  `واذكار الصلاة`, unchanged. Existing vendor compile warnings remain.
- [ ] Physical small-font observation. No upload, serial access, Android asset
  or Git publication in this batch; connected clock still has prior font-bench
  image until owner requests the next upload. Host tests do not certify panel
  legibility or electrical behaviour.

### P104 small-Tahoma upload — 2026-10-14

- [x] Owner requested upload. Rebuilt `p10_4x1_tajawal_trial`: normal cards,
  no `P10_FONT_VISUAL_TEST`; accepted Tajawal/Basit large and Tahoma Bold large
  retained, with refined Tahoma small Western/Indic fonts. Fresh small-font
  tests and real editor roundtrips pass. RAM38572B, Flash437667B; BIN441824B,
  SHA256 `57DB695B8C8596D8E45FFD3116AC9D29BFB556EDA230032976EB2D095DCA918D`.
- [x] COM5 preflight MAC24:4c:ab:4f:ed:96 matches tested P104; no running soak
  process found. Uploaded at115200, esptool verified flash data hash and
  requested RTS reboot. BIN archived externally as
  `tahoma-small/P104-small-font-comparison.bin`; previous images retained.
  No filesystem erase, settings/time commands, version change, Android or Git
  publication. Only upload and its normal resets performed on hardware.
- [ ] Owner physical evaluation with Tahoma + small selected. Post-upload
  network/API/RTC state and display appearance were not independently checked.

### Bounded P10 eight-row comparison — 2026-10-14

- [x] Owner rejected small-font panel legibility despite structural checks:
  dot spacing makes thin strokes weak and seven-row bodies look unnatural.
  Approved a three-word8px probe (`الصلاة`, `الإقامة`, `المغرب`) before a full
  redesign, and removal of small-font hamza/madda distinctions. Created
  `TahomaP10_8_Trial` with hand-hinted thick stems/rails and open bowls;
  alef variants alias plain alef, waw/yeh hamza alias their plain forms,
  lam-alef variants alias plain ligatures and standalone hamza is blank.
  UTF-8/shaper and large fonts are untouched. Other glyphs are provisional
  seven-row fallbacks: this is NOT an approved complete replacement.
- [x] Generated matching runtime/export header and editable
  `GLCDFontCreator2/Fonts/TahomaP10_8_Trial.ftsd`. Opt-in
  `p10_4x1_small8_test` reuses the bench: old7px above/new8px below, three
  static words10s each after splash, then configured display restored once.
  Normal cards/fault/prayer indications are suppressed only during30s bench;
  RTC ticks, Wi-Fi/server and OTA guards continue. Reboot repeats the probe.
  No production font selection, settings/time/storage/version changes.
- [x] Prototype test first failed on missing font; small bench first failed
  the three-page bound. They now pass aliases/ASCII parity and word widths
  36/42/41px with100/111/113 lit pixels (each >=1.35x current glyph ink),
  both populated screen rows, stable holds, one restore and timer rollover.
  Ink counts do NOT certify perceived brightness/naturalness on hardware.
  Real editor export parity passes7/8/16px; real renderer139824 frames per
  mode across9 fonts/1-4panels passes. Other local firmware27/parser16,
  startup, scan/SDK/reply/capture/region, soak5 and7-segment checks pass.
  Existing `p10_prayer_sequence_test.py` still fails the outdated `واقامة`
  wording fixture; current owner `واذكار الصلاة` is unchanged.
- [x] Both comparison/bench builds pass. Non-bench comparison BIN retains
  prior SHA57DB695B… exactly. Bench RAM38588B, Flash439715B, BIN443872B
  SHA256 `62B593BAB0596A1C9ECE3C9CC877992ED284B5881A7365AC024D3D9DB42C371D`.
  Generator/pixel-gap preview and BIN archived externally in `tahoma-small`
  (`build_trial8.py`, `trial8-comparison.png`, `P104-small8-three-words.bin`).
- [x] Uploaded to verified COM5/MAC24:4c:ab:4f:ed:96 at115200; esptool
  verified flash data hash and requested RTS reboot. No filesystem erase,
  settings/time commands, Android or Git publication. Previous BIN retained.
- [ ] Owner physical comparison before extending the font; post-upload
  API/RTC health and perceived brightness were not independently checked.

### Complete P10 Tahoma Bold 8px small-font trial — 2026-10-14

- [x] Replaced the three-word prototype with a complete 8-row Arabic family:
  all 127 contextual forms are populated from the reviewed Tahoma Bold 16px
  artwork using 2x2 max-pooling, while existing Tahoma7 ASCII and digit
  pixels remain byte-identical. Small-font-only aliases map alef hamza/madda
  variants to plain alef, waw/yeh hamza to their plain forms, and all
  lam-alef variants to the plain ligature; standalone hamza is blank.
- [x] Added matching runtime headers and editable
  `GLCDFontCreator2/Fonts/TahomaP10_8_Bold.ftsd` / Indic sibling. The
  FontCreator batch now regenerates both assets. Full coverage, alias,
  width, ASCII-parity, real-editor roundtrip, and renderer/scroll tests pass.
- [x] Added opt-in `p10_4x1_small8_full_test`, which uses the new family only
  for configured Tahoma small text; large fonts, Basit small text, settings,
  storage, Android assets, and release selection remain unchanged. Build:
  RAM38572B, Flash437507B, BIN441664B, SHA256
  `F8F9FC22EA4A85F515877096C768F6FF26E094B4D127871AF2977E614E2B7826`.
- [x] Uploaded that opt-in trial to the verified P104 on COM5
  (MAC `24:4c:ab:4f:ed:96`) at 115200; esptool verified the flash hash and
  performed a normal RTS reset. The BIN is archived in the external
  `tahoma-small` task folder. No settings/time/filesystem/Android/Git change.
- [ ] Physical-panel comparison and legibility check are still required before
  making the new font the default release small Tahoma family. The existing
  prayer-sequence fixture remains outdated and was not changed.

### P10 small-font weight correction — 2026-10-14

- [x] Owner rejected the first full Bold trial because 2x2 max-pooling filled
  internal counters and reduced letter clarity. The direct regular TTF render
  was also rejected as too sparse at 8px, so it was not uploaded.
- [x] Built a medium-weight complete family from the same reviewed Tahoma Bold
  source using a 2-of-4 block threshold. All 127 forms, hamza aliases,
  lam-alef aliases, ASCII parity, FTSD roundtrip, and 13-font renderer/scroll
  checks pass. Large fonts and Arabic shaping remain unchanged.
- [x] Uploaded the medium trial to COM5/MAC `24:4c:ab:4f:ed:96` at 115200;
  esptool verified the flash hash and performed the normal RTS reset. BIN:
  `P104-small8-medium-trial.bin`, SHA256
  `55037A0C4C302DC3F15358B2CE99449547D39FF0ED60C625F2D4453B139BAB1D`.
- [ ] Owner must compare the medium trial on the panel before release adoption.
  The prior Bold image remains archived only for rollback.

### Rejected small-font experiments — 2026-10-14

- [x] Owner rejected both the full Bold and reduced Medium Tahoma 8px trials
  as less clear than `BasitFont7`. The normal P104 firmware was restored and
  uploaded to COM5/MAC `24:4c:ab:4f:ed:96`; esptool verified the image hash and
  performed a normal RTS reset. No settings or stored data were changed.
- [ ] Keep `BasitFont7` as the preferred small-font choice unless a future
  redesign starts from a fresh visual specification rather than further
  thinning/dilating the Tahoma bitmap.

### Small-font selection finalized — 2026-10-14

- [x] Small text now always uses `BasitFont7` / `BasitFont7Indic`, regardless
  of the selected large-font family. The family selector continues to choose
  Basit or Tahoma for the 16px large font only.
- [x] Removed the obsolete normal-card `P10_SMALL8_MEDIUM_TEST` build target;
  the three-word visual probe remains opt-in and is not a production path.
- [x] `p10_4x1_tajawal_trial` build passed: RAM 38572B, Flash 434675B. No
  firmware upload was performed.

### Original Basit font restored and uploaded — 2026-10-14

- [x] Removed the Tajawal large-font override; family 0 now uses the original
  `BasitFont16` / `BasitFont16Indic` and BasitFont7 small-font family.
  Tahoma remains available only when family 1 is selected.
- [x] Built and uploaded the P104 image to COM5/MAC `24:4c:ab:4f:ed:96`;
  esptool verified the flash hash and performed a normal RTS reset. BIN SHA256:
  `C8759FF9CBB428EBA6892DD10260B9368A13EF92C92DD8258FF47F7ECDA2E47D`.
- [x] Android display label changed from `Basit / Janat` to `بسيط`; the
  Android debug APK was rebuilt separately. No APK installation/publication.

### Arabic final-form lookup corrected; owner artwork preserved — 2026-10-15

- [x] Corrected the shared generator/map: teh-marbuta is RIGHT-joining;
  isolated slot155 and final slot158 are used, while156/157 remain reserved
  at their existing positions. FE94 pre-shaped input also resolves to158.
  No runtime renderer, scrolling, configuration or API changes.
- [x] Python's positional reference now respects the current letter's joining
  property, including non-joining hamza. New independent oracle failed before
  the fix, then passed1327 positional cases plus explicit words, harakat,
  lam-alef, ASCII and runtime pre-shaped/digit cases:1343 C++ /1339 Python.
  The checked-in map reproduces exactly from its generator.
- [x] SHA256 comparison of all46 files in runtime fonts, editable `Fonts`
  and `Exports` confirms every byte unchanged. No bitmap normalization,
  Indic synchronization, font export or editable-project regeneration was
  performed. Owner glyph/numeral changes remain intact; previously diagnosed
  asset differences between digit variants remain outside this correction.
- [x] Updated specification/editor-builder labels to identify the reserved
  teh-marbuta slots. Existing FTSD files are untouched.
- [x] Real Bitmap/SmartText scroll regression passed across13 fonts and1–4
  panels:201648 samples per mode, default and explicit pixel-scroll agree.
- [x] All four release profiles built successfully (1x1/2x1/3x1/4x1);4x1
  RAM38572B, Flash424451B. Editor-builder Java source also compiles.
- [ ] No hardware upload, Android asset publication or Git publication in
  this batch; physical observation of final-form joins remains pending.

### Narrow-panel alert and alternating-text corrections — 2026-10-15

- [x] Reproduced both defects against production rendering before editing:
  oversized flashing alerts were centered/clipped, and timed alternation
  replaced scrolling text before its complete pass on 1–4 panel profiles.
- [x] SmartText keeps blinking for fitting alerts and continuously scrolls
  oversized alerts through the existing pixel engine. Fonts, glyph artwork,
  scan timing, regional clearing and buffer handoff are unchanged.
- [x] Large-font paired displays start at the first message. Each scrolling
  message completes its pass; each fitting message holds for five seconds
  from its first rendered frame. Explicit display/config resets restart the
  pair; static hold arithmetic handles millis rollover. Prayer-stage changes
  still take priority over a running message.
- [x] Replaced obsolete prayer source-string assertions with real
  `displayText`/`checkPrayers`, SmartText, shaping and Bitmap execution:
  12 profile/group runs pass, including384 alert/font/language combinations,
  both texts in each pair, live clock/count changes, transitions to short/empty
  alerts, normal-card completion and every prayer phase. The fixture covers
  six active font/digit variants, Arabic/English, 1–4 panels and20/70ms scroll.
- [x] Existing pixel-scroll regression passes201648 samples per mode across
  13 fonts. Text-region isolation, independent Arabic shaping and startup
  checks pass. A bounded independent code review found no actionable defect;
  its own test execution was blocked by Windows DLL error623, so execution
  results are from the parent run with the approved local compiler.
- [x] All four release builds pass. RAM/Flash bytes:1x1=38488/423519,
  2x1=38512/423671,3x1=38548/424787,4x1=38556/424923.
  Pre-existing PrayerTimes/Bitmap compiler warnings remain outside this fix.
- [ ] Physical confirmation of this corrected image remains pending. No
  firmware upload, device time/settings change, Android asset publication,
  firmware version bump or Git publication in this batch.

### Owner's revised Basit7 copied to Indic — 2026-10-15

- [x] Copied all214 non-digit records from the current `BasitFont7.ftsd`
  into `BasitFont7Indic.ftsd`, preserving its identity and all10 original
  digit records.71 non-digit records changed. Java serialization roundtrip
  verifies each entire glyph record, including width, pixels and comment.
- [x] Refreshed only the Indic editor export and runtime header. All214
  runtime non-digits match the actual editor export of the owner FTSD;
  all10 runtime Indic digits match their previous values exactly.
- [x] Pixel-scroll regression passes201648 samples per mode over13 fonts
  and1–4 panels. Default `pio run` succeeds:RAM38788B,Flash428427B.
- [ ] Existing Western `BasitFont7.h` exports are shifted down one pixel
  relative to their editable FTSD (e.g. zero columns62/65 vs124/130).
  Preserved the owner's Western FTSD/export/runtime files as requested;
  no normalization of those files, device upload or Git publication.
  Before-change Indic project and headers are archived outside the repo in
  `basit7-sync-20261015` under this task's visualization directory.

### Small-font top-row alignment and zone isolation — 2026-10-16

- [x] Reproduced the owner observation: both current small-font headers have
  global ink mask0xFE (row0 blank), top-info layout reserved4 right pixels,
  and oversized clock glyphs could write into the adjacent info zone.
  Host tests failed row-zero/zone isolation and the3x1 clock-fit case first.
- [x] Top rendering now removes only globally blank font rows, cached per
  font. Current fonts move up one pixel; already bit0-aligned fonts do not
  move. Lower-row positioning and all font/editor assets remain unchanged.
- [x] Removed the right margin. Info aligns its actual ink to the right edge.
  A shared8-row Bitmap scratch buffer renders clock/info, then copies only
  the owning zone into the back buffer. Wide glyphs cannot overwrite the
  other half or the lower row. The allocation is once-only,32–128 bytes
  depending on panel width; allocation failure safely skips top drawing.
- [x] Clock retains hours/minutes, dropping period then seconds only when
  necessary. Long info uses shorter weekday/date/silent formats after the
  existing Hijri fallbacks; hard clipping still protects zone boundaries.
- [x] All12 top-line test groups pass across1-4 panels, including6144 clock/
  card fits, both digit styles/languages, all hours/weekdays/Hijri months,
  an oversized synthetic font and an already bit0-aligned font. Bottom rows
  remain byte-identical. Prayer-sequence, pixel-scroll, region and startup
  regressions also pass. Hardware refresh latency is not established here.
- [x] All four final release builds pass. RAM/Flash bytes:
  1x1=38528/423967,2x1=38552/424151,3x1=38620/426067,
  4x1=38644/426259. Existing PrayerTimes warnings remain.
- [ ] No hardware upload, clock/settings write, Android assets or Git
  publication. Physical confirmation of this candidate remains pending.

### Owner-requested P104 USB upload — 2026-10-16

- [x] Identified the attached COM5 clock by chipID0x4FED96 /
  MAC24:4c:ab:4f:ed:96, matching the previously verified4x1 P104 board.
  Rebuilt `p10_4x1_release`, then uploaded430416B through esptool at115200.
  Data hash verification passed and the tool issued the normal RTS reset.
  BIN SHA256:`2E2D29A02A728E11DAC4F40444FC55B163D3FF11AAD198ADAA2091E6F4D3723E`.
- [ ] Post-upload HTTP health could not be read: PC Wi-Fi remained
  disconnected, and Windows blocked netsh reconnect on location permission.
  This is not evidence of clock network failure. Owner must confirm top-row
  appearance on the panel. No settings/time/filesystem/Android/Git writes.

### Owner's one-pixel margins and simpler clock format — 2026-10-16

- [x] Removed both adaptive clock-deletion branches (period and seconds).
  Clock/info use a fixed64px split on3x1/4x1;3x1 info uses its remaining32px
  with existing compact formats plus day/month and shorter weekdays when
  needed. The established1x1 HH:MM format remains unchanged.
- [x] Simplified top-zone copying to one measured ink rectangle. Clock ink
  starts at x=1; info ends at screenWidth-2, leaving one blank outer pixel.
  Region clipping, font-aware top-row lift and untouched lower rows remain.
- [x] Revised tests first failed old margins/3x1 seconds retention, then
  all12 groups passed.6144 font/language/date fit cases now assert the
  margins and two clock colons on2-4 panels. Actual prayer-display and
  lower-region regressions pass; all four release builds pass.
- [x] Final build RAM/Flash bytes:1x1=38528/424463,2x1=38552/424599,
  3x1=38644/426259,4x1=38644/426259. Existing library warnings remain.
- [x] Reconfirmed COM5 chipID0x4FED96/MAC24:4c:ab:4f:ed:96 and uploaded
  the430416B P104 release at115200. Flash data hash verified; normal RTS
  reset followed. BIN SHA256:
  `5E075E8E7B226F54A953EE662812F3C4712B764E2BFD65D2931C20E3ED015C0D`.
- [ ] Owner visual confirmation remains pending. No font artwork, device
  settings/time, Android assets or Git publication.

### Build-profile cleanup and Android-compatible firmware names — 2026-10-16

- [x] Retained four production environments and one4x1 serial-debug environment;
  default builds now select `p10_4x1_release`. USB speed is115200 as used in
  the verified uploads. Retired development duplicates and font/scan experiments.
- [x] Release artifacts now match the Android catalog:1x1=`updateP101.bin`,
  2x1=`updateP102.bin`,3x1=`updateP103.bin`,4x1=`updateP104.bin`.
  Debug emits `updateP104_debug.bin` and is excluded from Android packaging.
  Manifest generation and packaging consume the same new filenames.
- [x] Packaging offers1-4/A interactively or `-Profile All`/individual profile;
  builds the complete selection and validates all outputs before copying any,
  then verifies SHA256 per Android asset. The former always-P104 destination
  bug is removed. Seven disposable-directory packaging checks pass, including
  paths with spaces, single/all/interactive selection and failure/no-copy cases.
- [x] Created and read back `C:/Users/m_nag/Desktop/Prepare P10 Android.lnk`;
  target is Windows PowerShell with the real Android project path and the
  packaging script. Window remains open to show results or errors.
- [x] All five actual PlatformIO builds pass. Release BIN sizes for1-4 panels
  are428592/428736/430416/430416B; debug is433872B. Existing Bitmap/PrayerTimes
  warnings remain. Full real-compiler packaging to a disposable Android tree
  passes for all four profiles, including every post-copy SHA256 check.
- [x] All20 documented/startup/packaging host invocations pass. The soak unit
  check initially lacked `serial` in Python314; its five tests pass under the
  existing PlatformIO Python environment with pyserial (no installation needed).
  Independent bounded review found a relative-path destination defect; a new
  test reproduced it, and resolving the asset path before changing directories
  fixes it. All seven packaging tests pass after that correction.
- [x] No clock upload, actual Android asset replacement, catalog version bump
  or Git push. The shortcut replaces selected Android assets only when run;
  rebuilding the APK and managing release-version numbers remain manual.

### Language-independent digit/Latin spacing — 2026-10-17

- [x] Reproduced touching digits after the owner removed embedded side bearings:
  the old rule spaced only Latin pairs in English and only digit pairs in Arabic.
  The new tight-font pixel test failed before the production change.
- [x] Added one shared `needsTextGap` rule for adjacent ASCII letter pairs and
  adjacent digit pairs, independent of UI language or scroll direction. Static
  app drawing/measurement and SmartText drawing/measurement/incoming columns
  use it. Arabic joins, letter/digit boundaries, punctuation and explicit spaces
  retain their existing behavior. No font/editor bitmap or EEPROM changes.
- [x] Tight7/16px synthetic fonts pass measured-width, exact pixel-gap, static
  threshold, every-frame scroll-cycle and connected-Arabic tests in default,
  disabled and enabled pixel-scroll modes. Existing real-font host fixtures
  include the shared rule and preserve production function extraction.
- [x] Bounded independent review found no actionable issue. Broader host checks
  passed except the hidden-entry performance assertion, which caught unnecessary
  clearing of new Latin gaps outside the screen. Gap coordinates are now checked
  before clearing; rerunning the pixel-scroll test passes201648 samples per mode
  across13 real fonts and1-4 panels, with no hidden glyph/rectangle work.
- [x] Final four release rebuilds pass; RAM remains38528/38552/38644/38644B
  for1-4 panels. Existing library warnings remain. The tight-font recheck passes
  all three scroll modes after the off-screen guard; the other20 host invocations
  pass collectively after rerunning the one failing pixel-scroll test.
- [ ] No hardware upload, Android assets or Git publication. Physical
  confirmation of this image with the owner's revised artwork remains pending.

### Owner-authorized temporary scan diagnostics — 2026-10-17

- [x] Verified COM5 P104 identity (chip0x004fed96, MAC24:4c:ab:4f:ed:96)
  and saved a complete4MiB flash backup before the temporary upload. The
  installed image announces P104-rc4 (430208B), whereas current source builds
  rc12; this difference limits comparison with the owner's original image.
- [x] Enabled existing DMDESP_SCAN_DIAGNOSTICS using a task-local configuration
  and separate build folder. No production source/profile or scan/OE/PWM
  behavior changed. DEBUG4x1 build passed:RAM39136B,Flash430415B,BIN434560B.
  Upload at115200 passed flash hash verification.
- [x] Five-minute passive serial capture recorded30 windows/299183 scans,
  all AP mode2/peers0 and no API/send/store activity. Mean gaps1001–1002us;
  maximum gap7786us;37 gaps over5ms. Maximum rendering stage8141us and
  scan duration985us. These software timings do not prove visible shimmer,
  electrical OE behavior or a causal relationship between window maxima.
- [x] Restored the exact original first1MiB from the flash backup at115200;
  write hash and subsequent executable-region digest verification passed.
  Original filesystem/EEPROM region0x300000–0x3FBFFF also verifies unchanged;
  this region was not rewritten. No RTC/settings write, Android publication,
  firmware-version publication or Git push. COM5 closed after normal reset.
- [ ] Phone-connected/app-closed and physical pre-azan/static/moving comparisons
  remain pending: no connected-peer window or owner visual response arrived
  during this bounded capture. Final RTC/API/screen health was not observed.
  No fix is claimed. Report, raw summaries, manifests and local recovery images:
  `C:/Users/m_nag/.codex/visualizations/2026/10/17/01a14900-8f6f-7a02-ac4f-427e59861ad9/scan-diagnostics/`.

### Pre-azan scene reproduced with owner observation — 2026-10-17

- [x] Owner requested the pre-azan scene. A task-local source copy calls the
  real checkPrayers branch for180s, supplying a synthetic phase/prayer in RAM
  and restoring the real prayer cache after every call. RTC/software time,
  saved prayer schedule, configuration and production sources are unchanged.
  Native checks cover all1440 event minutes/midnight and cache restoration;
  runner checks recovery after success, upload failure and capture failure.
- [x] Temporary4x1 build passed:RAM39288B,Flash430871B,BIN435024B. Upload at
  115200/hash verification passed; boot reports RTC OK. Current saved display
  settings select small Basit/Indic digits. Owner confirmed the same strong
  shimmer, then increased shimmer after connecting the phone; serial records
  peers0 followed by peers1. No disconnection reversal was captured.
- [x] Eighteen scene windows recorded no instrumented API/send/store activity.
  Entry gap/render maxima7813/8435us. Excluding entry/connection-transition
  windows leaves8 windows per state: no-peer maximum gaps5361–6005us and
  refresh duration509us; one-peer gaps5528–6980us and refresh duration2048us.
  Refresh duration includes preemption/yield; window maxima are not paired
  events. These results reproduce the symptom, not a sole electrical cause.
- [x] Exact original first1MiB restored with write hash verified; subsequent
  executable and original filesystem/EEPROM digests both match. COM5 closed
  after normal reset. No RTC/settings writes, Android assets or publication.
- [ ] Frame-update versus unchanged-frame comparison remains pending. No
  rendering/scan/OE fix attempted. The source diagnostic is rc12 versus the
  original rc4 image; final restored API/RTC/screen health was not observed.
  Report, owner observations and recovery logs are in sibling `preazan-probe/`
  under the same task artifact directory as the preceding scan capture.

### Frame-update / held-frame / resume comparison — 2026-10-17

- [x] Owner approved continued diagnosis with the phone disconnected. Task-local
  source runs ACTIVE1/HOLD/ACTIVE2 for60s each. HOLD skips rendering and retains
  the completed frame while normal scan/network/RTC/software time continue.
  Production sources, SPI/OE/PWM, saved settings and RTC are unchanged.
- [x] Native checks pass60000 held iterations without repaint/clear, resumption,
  cache restoration, millis rollover and invalid-RTC abort; recovery self-check
  covers success/upload failure/capture failure. Build passes:RAM39352B,
  Flash431039B,BIN435184B; USB115200 upload/hash verified; boot RTC OK.
- [x] Owner reports usual shimmer in ACTIVE1, clearly reduced but residual
  shimmer in HOLD, then usual shimmer again in ACTIVE2. Held-buffer CRC remains
  64fed83a across HOLD. All scan windows report peers0/no API/send/store activity.
  Excluding stage-transition windows gives5 windows per stage: maximum gaps
  5718/1345/5514us, counts over5ms6/0/4, rendering maxima5092/65/5086us.
  This supports contribution from frame-update work; drawing cost versus actual
  buffer swapping is not isolated. Serial printing may affect residual shimmer,
  and its excluded software intervals are not an electrical timing guarantee.
- [x] Exact original executable restored and write hash verified. Independent
  executable and original filesystem/EEPROM digests both match; COM5 closed
  after normal reset. No RTC/settings writes, assets or release publication.
- [ ] Back-buffer rendering without presentation remains untested. No fix or
  sole electrical cause claimed. Final restored API/RTC/screen not observed.
  Report, observations, generated probe and recovery logs: sibling
  `frame-hold-probe/` in the same task artifact directory.

### Cross-model release-readiness audit — 2026-10-17

- [x] Completed a bounded read-only audit of P10, M73/M73s, Android and
  located historical market firmware. Report:
  `docs/RELEASE_READINESS_AUDIT_2026-10-17.md`. M71 firmware stays DEFERRED;
  O72's bundled identity is verified but matching source remains unidentified.
- [x] Fresh four P10 release and two M73 builds pass. Android34 JVM tests,
  release lint and unsigned release APK build pass (24 lint warnings).
  Three M73 native tests pass. All12 bundled BINs pass integrity checks;
  all six modern Android assets differ from the fresh firmware builds.
- [x] Verified remaining storage/credential failure paths with source review
  and small extracted-source host fault models. Documented Android legacy
  capability/error mismatches. Existing settings-fault tests cannot execute
  because the delegated P10 test harness is missing; Android location test
  fixture also needs correction. No physical fault frequency is claimed.
- [ ] Market-wide final release is not ready: close the ranked report findings,
  finish P10 shimmer diagnosis/owner acceptance, validate exact candidates on
  hardware, then assign release versions, package verified assets and sign/test
  Android upgrade. Shared modern API structure is not full behavioral parity.
- [x] This audit changed documentation only. No production fixes, version bumps,
  serial/network hardware operations, Android asset replacement, APK install,
  signing or publication. Evidence is in the task artifact `release-audit/`.

### Unified modern release 13 packaging — 2026-10-17

- [x] Set the numeric firmware version to13 for all four P10 profiles. Because
  market M73/M73s units already advertise15/14, their unified forward target is
  16 (a target13 would be rejected as a downgrade). P10 startup markers are
  P101-13/P102-13/P103-13/P104-13; M73/M73s markers are M73-16/M73s-16.
  M73 checkUpdate now advertises16.
- [x] Rebuilt `p10_1x1_release`, `p10_2x1_release`, `p10_3x1_release`,
  `p10_4x1_release`, `ta73` and `ta73s`. The corrected publisher now maps the
  actual P10 output names and stages all six outputs before replacing assets.
- [x] Updated the Android modern catalog to latestVersion13 for P101–P104 and
  latestVersion16 for M73/M73s, and refreshed catalog unit-test expectations.
- [x] Copied the six verified BINs into Android `app/src/main/assets`. Each
  asset is byte-identical to its corresponding build and contains the expected
  version marker. SHA256 details are in the task artifact
  `release-audit/firmware-modern-verification.json`.
- [x] Android `testDebugUnitTest`, `lintRelease` and `bundleRelease` pass after
  packaging. No APK/AAB was uploaded, no hardware was flashed, and the older
  models remain unchanged.
- [ ] Physical OTA/display/settings validation of the exact P10 release13 and
  M73 release16 images, including the known P10 shimmer, remains required.

### P101 clock heartbeat colon — 2026-10-18

- [x] The fixed P101 top-row `HH:MM` clock now toggles its colon every second;
  P103 and P104 keep their existing display behavior.
- [x] The static large-font `HH:MM` clock on P102 uses the same fixed-position
  colon blink; scrolling clock text is unchanged.
- [x] During small-font iqama countdowns on P101/P102, the lower line now
  alternates between the countdown and «الهاتف صامت»; other small cards remain unchanged.
- [x] Sunrise sleep is disabled after its iqama on all P10 profiles; other
  prayers retain the configured sleep behavior.
- [ ] Physical visual check on P101 remains pending; no firmware upload was done.

### P101/P102 splash layout — 2026-10-18

- [x] Narrow splash uses a short centered `Basit` identity; only the phone
  number scrolls when the selected width cannot contain it.
- [x] Invalid RTC status on P101/P102 now renders `اضبط` and `الوقت` on two
  fixed rows (or `SET`/`TIME` in English) instead of alternating one line.
- [x] All four P10 release environments build successfully after the change.

### Prayer sleep boundary — 2026-10-18

- [x] Fixed the off-by-one phase boundary that skipped the sleep window when
  `sleepPrayerTime=1` and shortened longer sleep windows by one minute.
- [x] The display window now remains active through the corrected sleep and
  following azkar minute; the focused contract test passes.
- [x] All four P10 release environments build successfully after the fix.
- [x] The corrected P101–P104 BINs were copied to Android assets and verified
  byte-for-byte by SHA256.
- [ ] Physical P102 verification after uploading the new BIN remains pending.
- [x] The post-Friday-label P101–P104 BINs were synchronized to Android assets
  and verified byte-for-byte by SHA256.
- [x] The matching `updateP101.bin`–`updateP104.bin` assets were copied to the
  Android project and verified byte-for-byte by SHA256.
- [ ] Physical visual check on P101/P102 remains pending; no firmware upload
  was done.

### Friday event label — 2026-10-18

- [x] Friday's Dhuhr event now uses the `Jumaa` label in both the pre-azan and
  azan windows; the prayer-times card keeps the regular `Dhuhr` slot label.
- [x] All four P10 release environments build successfully after the change.
