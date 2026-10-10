# M71 compact API upgrade — deferred implementation plan

> **For agentic workers:** Use `superpowers:executing-plans` for inline execution
> when the owner authorizes this separate task. Steps use checkbox progress.

**Status:** DEFERRED by the owner on 2026-10-09. Do not implement, build,
publish or flash M71 as part of P10/M73 unification.

**Goal:** Let the current Android app update and configure M71 over Wi-Fi.

**Architecture:** Keep M71's shift/latch display driver and GPIO mapping.
Port the compact root POST contract and reuse the already reviewed pure
calendar/prayer/parser/settings helpers, not M73's display firmware.

**Tech stack:** ESP8266, Arduino/PlatformIO, existing M71 display/RTC libraries,
Android Java/OkHttp. No new JSON, signing or authentication layer.

**Spec:** Owner's 2026-10-09 instruction to defer M71 independently;
`../../SEVEN_SEGMENT_COMPATIBILITY.md` documents the current incompatibility.

## Source map and constraints

- Firmware: `C:/Users/m_nag/Desktop/Arduino/Projects/Masjed-71`;
  `src/masjed-71.cpp`, `src/config.h`, `platformio.ini`, `copyFirmware.py`.
- Android: `C:/Users/m_nag/Desktop/Android_Projects/MasjedClockWiFi`;
  `DeviceList.java`, `MainActivity.java`, `UpdateActivity.java`,
  `classes/DeviceIdentity.java`, and `app/src/main/assets/updateM71.bin`.
  Java paths are under `app/src/main/java/com/basitapps/masjedclockwifi`.
- Current advertised identity is `update,M,71,9,good`; Android target is 9.
  Recheck both before execution. New source/BIN/target must use the same version
  greater than the deployed one (10 if 9 is still current).
- M71 has `shiftOut`/data/latch/clock display hardware. Never use `updateM73.bin`.
  M71s is a different variant and is outside this plan unless separately approved.
- Keep GET `/checkUpdate` and multipart POST `/updateMain` usable from old M71.
  The old upload code already sends `UpdateOk` after `Update.end(true)`.
- Configuration uses the same root POST field names/results as P10/M73.
  Preserve the AP password as the only local access boundary.
- Do not alter the approved PrayerTimes formula, sold-device records or existing
  location IDs. Preserve valid settings through any record-layout migration.
- Do not build/install an APK. Preserve the owner's dirty worktree.

## Review focus

Pin these conditions in the owning task's tests: wrong model/image must never
be uploaded; malformed/oversized/duplicate POST fields must not change settings;
lost RTC power must differ from missing RTC; interrupted upload must not report
success; a reset before the acknowledgement reaches Android remains unconfirmed.
Power/EN/RST and shift/latch activity during OTA require physical checks, not
an inference from a successful compile.

## Task 1 — Dedicated M71 firmware

- [ ] Read the M71 project instructions, all callers of the target routes,
  settings layout, OTA handler and build copy scripts. Disable automatic asset
  publication for development builds before running PlatformIO.
- [ ] Write and run failing native checks using the actual M71 handlers:
  `ct=504003` roundtrips, malformed fields preserve the previous configuration,
  six date/time fields are required, RTC errors return `rtc`, save failures
  return `storage`, and factory iqama defaults are `{15,10,10,10,8,10}`.
- [ ] Add root compact POST handlers for `/getData`, `/setDateTime`, `/setEqama`,
  `/setLocation`, `/setSettings`, `/setPassword`, `/testMode`, `/resetDefault`,
  `/health` and `/selfTest`. No `/setMessage` or P10 font/card routes.
- [ ] Reuse `CompactApiContract.h`, `ClockSettingsContract.h`,
  `CivilTimeContract.h`, `PrayerMethodContract.h` and `PrayerPhaseContract.h`.
  Apply exact RTC validation, verified writes and save rollback without changing
  the display slots/driver. Keep full signed-32-bit storage for city IDs.
- [ ] Run the native checks and `pio run -e esp12e`: exit 0, no new warnings,
  acceptable RAM/Flash usage. No upload or Android asset copy in this step.
- [ ] Review the diff and commit only this task's files after validation.

## Task 2 — Android integration and release candidate

- [ ] Assert that M-71 selects only `updateM71.bin` and an M/71 identity;
  reject M/73 identity before upload. The current shared POST client is reused.
- [ ] Add M71 to the managed/technician capability selection only when its
  new firmware provides those routes. Preserve buzzer-versus-MP3 behavior.
- [ ] Build the dedicated M71 BIN and record its SHA256, actual identity/version
  and build environment. Copy it to the Android asset only with publication
  authorization; update the M-71 target to the same real version.
- [ ] Run `gradlew.bat :app:compileDebugJavaWithJavac`: exit 0. No APK.
- [ ] Review and commit the firmware/Android changes separately; never commit
  `.pio` output or publish a forced-test version as a real release.

## Task 3 — Physical compatibility gate

- [ ] On a real M71 running the market image, confirm identity, then perform
  OTA from Android using `/updateMain`; require HTTP `UpdateOk`, normal restart
  and the new identity/version. Without a board, leave this gate pending.
- [ ] Verify unchanged display wiring, RTC/time, AP password and settings;
  exercise time, iqama, location, settings, technician checks and factory reset.
- [ ] Repeat a few short uploads; check a lost connection/aborted upload does
  not cause false success or force an unrelated firmware. Long soak is owner-led.
- [ ] Publish only after these checks; record actual observations and limitations
  in this file and `../../ROADMAP.md`. Never claim hardware support from build alone.
