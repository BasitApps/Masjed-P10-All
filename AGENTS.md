# AGENTS.md — Project Guide for AI Assistants

> **Latest unification handoff (2026-10-09):** Read the latest section of
> `docs/ROADMAP.md` and `docs/FIRMWARE_REVIEW_2026-10-07.md` first. P10/M73 now
> share five byte-identical core headers; current M73 HTTP is synchronous.
> City IDs are int32_t; P10 raw v6 records migrate after CRC to config version 7
> without reset or boot write (not a firmware release-version bump). Common
> factory defaults, body/error validation and five-second restart grace align.
> M71 is explicitly DEFERRED in its own plan; never flash an M73 image to it.
> This batch does not upload hardware, publish Android BINs or build an APK.
> Physical save/RTC/OTA and intermittent-reset validation remain pending.
>
> **Current review handoff (2026-10-07):** Start with
> `docs/FIRMWARE_REVIEW_2026-10-07.md`. The P10 source audit and four release
> builds are complete; batches 1–3 are implemented and locally verified.
> Preserve owner changes, the approved prayer formula and simple root POST API.
> P10 4×1 release was uploaded on COM5 and flash hash verified. Native Windows
> WlanConnect connected the saved profile; initial API/RTC/selfTest passed.
> One release OTA failed with spontaneous reset; one USB-monitored DEBUG OTA
> succeeded with UpdateOk and Software/System restart. Release was restored.
> Owner confirmed RTC battery removed: lost-power/invalid-time afterward is
> expected if RTC supply is interrupted; do not treat it as a detection defect.
> Owner then fitted the battery and set time. WRITE now calls yield each chunk
> rather than only wdtFeed; three release OTA attempts without UDP peer passed
> with UpdateOk, normal restart, valid RTC and unchanged settings. This latest
> candidate is on the board; see report for BIN SHA and short-test limitations.
> Follow-up publication gate FAILED: attempted OTA of version 13 with UDP peer
> reset at progress 1 (ROM rst cause:1, health Power On); board remained rc4/12,
> RTC/time/settings valid. Thus per-chunk yield is NOT a complete fix for the
> intermittent reset. Temporary version changes were reverted; Android BIN was
> not replaced. Inspect 3.3V supply and RST/EN physically before more speculative
> fixes/publication. Owner subsequently reported phone OTA success and confirmed
> programmer TX/RX/GND/DTR/RTS; need clarify whether it remained connected during
> that success. Android phone authorized via ADB; no prior OTA log available.
> Current health reports normal software restart/RTC connected, but no timeValid
> field (do not assume same BIN); Android asset remains older398304B, owner map
> now13 (preserve). Do not open COM5 during the next controlled phone comparison.
> Owner confirmed programmer remained connected. Network-only Windows OTA of
> older Android asset failed at p=1 with Hardware Watchdog (different from the
> previous Power On). Restored reviewed398048B via USB115200/hash verified;
> same reviewed BIN OTA then passed with UDP progress0..99/HTTP200 UpdateOk
> without opening COM5; final RTC/time/settings valid and normal software reboot.
> Reviewed BIN/manifest now copied to Android for owner tests (SHA10d58801…,
> actual version12). Owner map13 is intentionally forced testing, NOT release13.
> No APK built/installed; owner must rerun Android Studio to replace old assets.
> Intermittent-reset proof, final version publication, 7-segment and soak pending.
> First 7-segment RTC/API parity batch is now implemented in its own repository:
> exact DS3231/OSF/BCD/calendar reads, verified write+config rollback with runtime
> uncertainty latch, same base-date DST offset, NaN prayer dashes, Friday azan
> and no duplicate date toggle. Real-source checks24/24, existing checks3/3 and
> ta73/ta73s builds pass. No upload/Android assets/version change for 7-segment.
> Next bounded batch: remaining 7-segment API readers and settings failure paths;
> physical RTC/OTA validation remains pending. Do not copy P10 yield/parser
> overlay into async callbacks blindly. Read report's latest section for limits.
> Follow-up API batch now stages Eqama/location/settings/password in a candidate
> and adds a small v2 request lock for the shared async body/response buffers.
> ta73 built successfully and was uploaded to the connected M73 board; UpdateOk,
> AP return and getData/profile=7seg were confirmed. No M73s/P10/Android asset
> publication. Android's raw octet-stream POST is the tested body contract.
>
> **Historical handoff (2026-10-01; plan closed 2026-10-02):**
> `docs/superpowers/plans/2026-10-01-firmware-comprehensive-review.md` for the
> comprehensive prayer/iqama/correctness/performance review and its progress log.
> RC1 was uploaded and initially confirmed working by the owner; development
> continues. Arabic UTF-8 shaping and the eight Basit/Tahoma font variants are
> already implemented. Legacy-encoding descriptions below are historical.
> PrayerTimes index 1 is Sunrise, 4 is Sunset, 5 is Maghrib; verify the current
> application mapping rather than following the outdated index description.
> Makkah Isha now has a firmware-only `makkahIsha` mode (0=method rule,
> 1=fixed 90 minutes); Android UI work is deferred in
> `docs/ANDROID_DEFERRED_PLAN.md`.

> **ملخص عربي:** ده مشروع فيرموير ESP8266 لشاشة LED Matrix (P10) لمسجد.
> الشاشة 128×16 بكسل (4 لوحات P10). المشروع حالياً بيستخدم ترميز عربي مخصص قديم
> (كل خانة ASCII في الخط بترسم شكل حرف عربي معيّن، والنصوص مخزنة معكوسة).
> شغّالين على مشروع تطوير لتحويل النظام لعربي UTF-8 حقيقي عبر "Shaper" وقت التشغيل.
> اقرأ `docs/ROADMAP.md` عشان تعرف وصلنا لفين وإيه الخطوة الجاية،
> و`docs/FONT_SPEC.md` لمواصفة الخط الجديد، و`docs/DECISIONS.md` لأسباب القرارات.

## Project Overview

ESP8266 firmware (PlatformIO / Arduino framework) driving a P10 LED matrix wall
(128×16 px = 4 panels wide × 1 high) inside a mosque. Displays: rotating info
cards (clock, dates, prayer times, messages, azkar...), prayer-time event
windows (azan/iqama countdown/sleep), all configurable from a mobile app over a
WiFi AP REST API. OTA firmware updates supported.

## Toolchain & Commands

| Task | Command |
|---|---|
| Build firmware | `pio run` (or VSCode PlatformIO: Build) |
| Upload (serial) | `pio run -t upload` |
| Serial monitor | `pio device monitor` (115200) |
| Font tool | `python tools/gen_font.py --help` (Phase 1 deliverable) |

- Board: `esp12e`, platform `espressif8266`. See `platformio.ini`.
- `renameFirmware.py` runs pre-build to rename output artifacts — do not break it.
- DEBUG flag lives in `src/constants.h` (`#define DEBUG 0/1`).

## Repository Layout

```
src/
  Masjed-P104.cpp   Main firmware: setup/loop, cards, prayer windows, web API, OTA
  globals.h         Data structs: Configuration (EEPROM), caches, TimeData...
  constants.h       Timing/display constants, buffers sizes, DEBUG macros
  texts.h           All fixed strings (UTF-8)
  SmartText.{h,cpp} Flicker-free text engine on top of DMDESP (scroll + flash)
  ArabicShaper.*    UTF-8 -> shaped glyph codes engine
lib/                Vendored libraries (treat as read-only unless noted):
  DMDESP/           LED matrix driver + GLCD-format bitmap fonts (fonts/*.h)
  PrayerTimes/      Prayer time calculation
  RTClib/           DS3231 RTC driver
  Adafruit BusIO/   I2C helper used by RTClib
tools/              Python dev tools (font generator, previews)
docs/               ROADMAP / DECISIONS / FONT_SPEC (project continuity docs)
```

## Key Runtime Concepts

### Loop structure (src/Masjed-P104.cpp)
- `loop()`: `dmd.loop()` every iteration → per-second block (`tickSecond()` →
  `syncRTC()` every 5 min or on minute rollover) → `checkPrayers()` decides if
  a prayer window is active (sets `dispState.stopCards`) → otherwise
  `handleCards()` cycles through the 12 card functions in `CARDS[]`.
- Cards are functions (`cardClock`, `cardPrayers`, ...). Each calls
  `displayText(...)` which delegates to `SmartText`. When a card finishes it
  calls `nextCard()`.

### Prayer windows
`prayerCache.times[]` holds minutes-since-midnight for 7 entries
(Fajr..Isha, index 5=Sunrise unused). `updatePrayerWindow()` computes the
active [start,end] window around each prayer; `checkPrayers()` renders the
right message (pre-azan, azan flash, iqama countdown, sleep pixel, azkar).

### Web API & the mobile app ("Basit Remote")
- Device is a WiFi AP `Basit-Masjed-<chipid>` at `192.168.1.177`.
- Endpoints: compact UTF-8 POST commands (`/getData`, `/setDateTime`,
  `/setEqama`, `/setLocation`, `/setSettings`, `/setMessage`, `/setDisplay`,
  `/setFont`),
  `GET /checkUpdate`, and multipart `POST /updateMain` (OTA).
- The AP password is the local access boundary; no `blur()`/`unBlur()` transport
  remains in the commercial path.

### Arabic text encoding
Runtime text is UTF-8 and is shaped by `ArabicShaper` before rendering through
the Basit/Tahoma bitmap fonts. See `docs/FONT_SPEC.md` for the font contract.

## Rules

1. Do NOT modify `lib/*` except `lib/DMDESP/fonts/` (new generated headers are
   allowed there) — DMDESP core changes only if unavoidable and documented in
   `docs/DECISIONS.md`.
2. RAM is scarce (~80KB): prefer PROGMEM for static data, static buffers over
   large stack locals, no `String` in hot paths.
3. EEPROM layout is versioned via `EEPROM_VERSION`; bumping resets user config
   once — always note it in DECISIONS.
4. Keep commits small and descriptive; never commit `.pio/` build output.
5. After code changes: build with `pio run`.
6. Update `docs/ROADMAP.md` status markers whenever you complete a phase.
