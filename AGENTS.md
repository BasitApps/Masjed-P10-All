# AGENTS.md — Project Guide for AI Assistants

> **Active handoff (2026-10-01):** Start with
> `docs/superpowers/plans/2026-10-01-firmware-comprehensive-review.md` for the
> comprehensive prayer/iqama/correctness/performance review and its progress log.
> RC1 was uploaded and initially confirmed working by the owner; development
> continues. Arabic UTF-8 shaping and the eight Basit/Tahoma font variants are
> already implemented. Legacy-encoding descriptions below are historical.
> PrayerTimes index 1 is Sunrise, 4 is Sunset, 5 is Maghrib; verify the current
> application mapping rather than following the outdated index description.

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
| Native unit tests | `pio test -e native` (added in Phase 3 of Arabic project) |
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
  texts.h           All fixed strings (currently legacy-encoded; being migrated to UTF-8)
  SmartText.{h,cpp} Flicker-free text engine on top of DMDESP (scroll + flash)
  ArabicShaper.*    (planned) UTF-8 -> shaped glyph codes engine
lib/                Vendored libraries (treat as read-only unless noted):
  DMDESP/           LED matrix driver + GLCD-format bitmap fonts (fonts/*.h)
  PrayerTimes/      Prayer time calculation
  RTClib/           DS3231 RTC driver
  Adafruit BusIO/   I2C helper used by RTClib
tools/              Python dev tools (font generator, previews)
test/               Unit tests (native env tests the shaper without hardware)
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
- Endpoints (GET): `/getData`, `/setDateTime`, `/setEqama`, `/setLocation`,
  `/setSettings`, `/setMessage`; POST `/updateMain` (OTA); plus `/testMode`,
  `/checkUpdate`.
- **Transport encoding**: request/response values pass through `blur()` /
  `unBlur()` (each byte → decimal of (byte+37), dash-separated). This operates
  byte-wise and is transparent to UTF-8 — keep it as-is.
- The app is updated separately by the owner to send plain Arabic UTF-8.

### Legacy Arabic text encoding (being replaced)
Current fonts map each ASCII slot (0x20–0x7F) to one hand-drawn Arabic glyph
form; strings are stored in reversed visual order because drawing is LTR.
Examples in old `texts.h`: `"_}nWA JaVIAh"` = "اهلا وسهلا". Only ~95 slots
exist so several letter forms were sacrificed — this is the problem the
Arabic-support project fixes. See `docs/FONT_SPEC.md` for the new scheme.

## Rules

1. Do NOT modify `lib/*` except `lib/DMDESP/fonts/` (new generated headers are
   allowed there) — DMDESP core changes only if unavoidable and documented in
   `docs/DECISIONS.md`.
2. RAM is scarce (~80KB): prefer PROGMEM for static data, static buffers over
   large stack locals, no `String` in hot paths.
3. EEPROM layout is versioned via `EEPROM_VERSION`; bumping resets user config
   once — always note it in DECISIONS.
4. Keep commits small and descriptive; never commit `.pio/` build output.
5. After code changes: build with `pio run` and, when touching the shaper,
   run `pio test -e native`.
6. Update `docs/ROADMAP.md` status markers whenever you complete a phase.
