# DECISIONS — Decision Log

## D25 — Display-only 12/24 choice
hour24=0 selects 12h, 1 selects 24h. Default 12h. EEPROM v6 reuses v5 padding;
CRC-verified v4/v5 images migrate without resetting settings. Configuration
and arithmetic continue to hold 24h time. /getTimeFormat advertises support;
signed setSettings and v2/getData carry hour24. Seven-segment changes apply
only to its main clock: several prayer hour fields physically have one digit.

## D24 — Complete numerals and independent digit style (2026-09-05)
Replace clipped generated Western digits with complete 5x7 artwork, doubled
to 10x14 for the large font. Indic digits reuse original Janat digit artwork.
Compile selectable font variants in PROGMEM: no runtime bitmap copying or
driver changes. Arabic glyphs remain unchanged. RAM +16 bytes; Flash +16592.
EEPROM v5 appends digitStyle; migrate the verified 448-byte v4 prefix after
checking its old CRC, preserving existing settings. Compile-time offset check
guards the migration assumption. Older/corrupt layouts keep default fallback.
API: setSettings digits=0 (0123) / 1 (Indic); v2/getData reports digits.
Android selector UI is pending; legacy CSV shape remains unchanged.
Regeneration order: generate base fonts, then python tools/build_digit_fonts.py.

Append-only log. Newest at bottom. Each entry: decision, context, consequences.

---

## D1 — Replace legacy ASCII-slot Arabic encoding with UTF-8 + runtime shaper
**Date:** 2026-08-26 (project start)
**Decision:** Store all Arabic text (built-in strings and user messages from the
app) as real UTF-8. A small runtime shaper (`ArabicShaper`) converts logical-order
UTF-8 into shaped glyph bytes rendered through an extended bitmap font.
**Context:** Legacy scheme mapped each ASCII slot (0x20–0x7F) of the font to a
hand-drawn Arabic glyph form and required strings pre-reversed into visual
order. Only ~95 slots existed, so letter forms were sacrificed; source code and
the mobile app had to speak the same unreadable encoding.
**Alternatives rejected:**
- *Extend legacy slots only* (0x80+ without shaper): still requires manual
  encoding everywhere; app keeps doing the hard work.
- *Build-time shaping only*: cannot handle user-entered mosque name / message
  arriving over the API at runtime.
**Consequences:** +~10KB flash for fonts; small RAM growth for buffers;
EEPROM_VERSION bump resets stored config once (accepted — app is updated in the
same release window).

## D2 — No bidirectional-text engine; single display language instead
**Date:** 2026-08-26
**Decision:** The display renders one language at a time (`config.language`:
0=Arabic, 1=English). Mixed-direction lines are out of scope. Inside an Arabic
line, digit runs and punctuation keep LTR order.
**Context:** Owner confirmed texts are either fully Arabic or fully English;
a full bidi implementation is disproportionate for a 128×16 matrix.
**Consequences:** Shaper is much simpler; no UAX#9. If mixing is ever needed,
this decision must be revisited first.

## D3 — Extended GLCD font layout (0x20–0xFF) instead of Unicode-indexed rendering
**Date:** 2026-08-26
**Decision:** Keep DMDESP's GLCD font format but extend char range so Arabic
glyphs live in slots 0x80–0xFF. The shaper maps Unicode presentation forms →
slot codes via a PROGMEM table generated together with the font.
**Context:** `Bitmap::drawChar()` indexes with `(uint8_t)ch`; extending
`first/count` needs zero changes inside `lib/DMDESP` core (respecting rule:
don't modify vendored libs).
**Consequences:** Font headers carry some unused Latin-range entries when
count spans 0x20..0xFF; acceptable (~2KB). Rendering path untouched → no
performance risk on the flicker-sensitive P10 refresh.

## D4 — Transport protocol (blur/unBlur) stays unchanged
**Date:** 2026-08-26
**Decision:** The byte-wise blur obfuscation remains as-is for all API values;
UTF-8 passes through transparently.
**Context:** blur operates per byte ((byte+37) decimal, dash-separated), it has
no opinion about character sets.
**Consequences:** App update only changes content encoding (plain Arabic),
not transport. Response buffers in firmware grow because UTF-8 Arabic blurs to
longer strings — handled in Phase 4.

## D5 — No tashkeel (harakat) support in v1
**Date:** 2026-08-26
**Decision:** Harakat are not rendered; if present in input they should be
skipped safely by the shaper.
**Context:** 16px/7px pixel heights make vowel marks illegible; owner opted out.
**Consequences:** Smaller font tables. Revisit only if a larger panel arrives.

## D6 — Two font sources tried: extracted Janat artwork and fresh TTF render
**Date:** 2026-08-26
**Decision:** gen_font.py supports both extracting existing Janat7/16 glyphs
into the new layout and generating from a TTF (e.g., Cairo/Noto Naskh). Owner
compares PNG previews and picks.
**Consequences:** Mapping table in FONT_SPEC.md is source-independent; both
paths emit identical header format.

## D7 — Remove system_timer_reinit() from DMDESP (loop + dispinit)
**Date:** 2026-09-01
**Decision:** Removed all `system_timer_reinit()` calls from `lib/DMDESP/DMDESP.cpp`
— both from `DMDESP::loop()` (was line 448) and from `dispinit()` (was line 439).
**Context:** `system_timer_reinit()` reinitializes the ESP8266 system timer that
underpins the WiFi TCP/IP stack, `millis()`, and the software watchdog. In the
Arduino core this timer is already initialized at boot; calling it again corrupts
timer state for the already-running WiFi stack.

The call in `DMDESP::loop()` ran on every 100µs refresh tick (thousands/sec),
corrupting watchdog timing → `rst cause:2` (software watchdog reset).

The call in `dispinit()` ran once during `dmd.start()` (called after `initAP()`
in `setup()`), corrupting the live WiFi timer → `LoadStoreError` in
`system_get_sdk_version` during `splashScreen()` (exception 3, `rst cause:2`).

**Consequences:** This is a vendored-library core change (violates AGENTS.md
rule 1), but is unavoidable and documented here. The display refresh timer
(`os_timer_arm_us`) works without `system_timer_reinit()` because the ESP8266
SDK initializes timer hardware at boot. No changes needed to the refresh logic.

## D8 — Guard effectiveBrightness() with RTC check
**Date:** 2026-09-01
**Decision:** `effectiveBrightness()` now returns `config.brightness` directly
until both RTC and the prayer cache are valid. It uses the already-synchronised
`timeData` instead of issuing a fresh `rtc.now()` I2C transaction.
**Context:** `effectiveBrightness()` calls `rtc.now()` (blocking I2C) and was
invoked from `splashScreen()` (via `applySettings()`) before `rtc.begin()`
was called, and from `loop()` every 60s. If the RTC was absent or the I2C bus
was stuck, the call could block long enough to trip the 8s watchdog.
**Consequences:** During setup before RTC init, default brightness is used
instead of a potentially-blocking I2C read. After a successful `rtc.begin()`,
normal brightness logic applies.

## D9 — Correct the restart diagnosis: invalid PROGMEM access was the boot-loop root
**Date:** 2026-09-01
**Decision:** Every fixed string stored with `PROGMEM` must be copied into a
bounded RAM buffer with `safeCopyP()` before it is passed to DMDESP, SmartText,
or a normal `%s` formatter. Flash-resident format strings use `snprintf_P()`.
**Context:** The hardware boot log reported `Exception (3)` with
`excvaddr=0x4024d023`; the ELF symbol table mapped that exact address to
`SPLASH_TITLE`, and the decoded stack mapped to `splashScreen()` → `setup()`.
`splashScreen()` passed the flash pointer directly to `dmd.textWidth()` and
`dmd.drawText()`, which read it as normal RAM. The same unsafe pattern existed
in several prayer/event paths and was corrected at the same time.

This evidence supersedes D7's claim that the observed boot loop was caused by
`system_timer_reinit()`. Removal of those DMDESP calls is an independent,
provisional change and requires P10 refresh plus long-soak verification before
it can be accepted or reverted.
**Consequences:** The firmware now reaches `RTC OK` and `Setup done` on the
physical ESP8266 and remains free of exceptions during the initial serial
observation. The vendored DMDESP timer change is no longer treated as the
proven restart fix.

## D10 — Validate and persist configuration atomically
**Date:** 2026-09-01
**Decision:** HTTP handlers now parse blurred values strictly into a local
`Configuration next`, validate all known ranges, persist that candidate, and
only then replace the live configuration. Malformed, truncated, out-of-range,
or invalid UTF-8 input rejects the whole request. Identical EEPROM images are
not committed again.
**Context:** The previous handlers modified `config` field-by-field with
`atoi()`/`atof()`. Invalid input silently became zero, unsigned fields wrapped,
and an EEPROM failure could leave live state only partly changed. Repeated
identical app requests also consumed flash erase cycles.
**Consequences:** Existing endpoint names and blur transport remain compatible.
Authentication and deferred/coalesced commits remain separate release decisions.

## D11 — RTC reads are validated and retried without blocking the clock loop
**Date:** 2026-09-01
**Decision:** Accept RTC data only when `DateTime::isValid()` succeeds and the
year is within 2025..2099. A missing or invalid RTC is retried every 30 seconds
without a wait loop; meanwhile the main loop, AP, and display continue running.
Power-save uses cached `timeData` and never performs its own I2C transaction.
**Context:** A diagnostic `rtc.adjust()` line had left the physical DS3231 at a
2022 test date. The new firmware detected and rejected it, then performed two
timed retry attempts without reset or WDT.
**Consequences:** The owner must set the correct time once from the mobile app.
After that, RTC recovery and prayer-window hardware tests can continue.

## D12 — Restore the original DMDESP refresh-timer calls
**Date:** 2026-09-01
**Decision:** Restore both original `system_timer_reinit()` calls in
`DMDESP::dispinit()` and `DMDESP::loop()`, returning the vendored library to its
repository version.
**Context:** With the calls removed, the physical P10 visibly painted from top
to bottom as if its refresh rate were too low. Serial/ELF evidence had already
proved the boot-loop root was the invalid `SPLASH_TITLE` PROGMEM access, not
the display timer. Restoring the two lines fixed the actual layer responsible
for row scanning while the corrected firmware continued to boot without an
exception during serial observation.
**Consequences:** D7 is fully superseded. `lib/DMDESP/DMDESP.cpp` has no
functional code difference from the repository version, and display smoothness
is again evaluated using the upstream behavior.

## D13 — Add backward-compatible EEPROM CRC metadata without a version bump
**Date:** 2026-09-01
**Decision:** Store a magic value and CRC32 immediately after the encrypted
`Configuration` inside the existing 512-byte EEPROM allocation. A valid legacy
configuration without metadata is accepted once and receives CRC metadata;
`EEPROM_VERSION` remains 3.
**Context:** Bumping the version would erase the owner's live settings even
though enough unused EEPROM space exists. CRC is needed to detect a torn write
or random corruption before copied values reach display/prayer code.
**Consequences:** Existing installations migrate with one commit and no reset to
defaults. Future CRC mismatch safely selects defaults. Identical encrypted
configuration plus metadata skips `EEPROM.commit()` entirely.

## D14 — Keep HTTP compatibility; defer authentication to the app contract
**Date:** 2026-09-01
**Decision:** Do not add a new query token or HTTP Basic challenge in this
firmware batch. Keep the existing WPA2-protected AP transport contract while
strictly validating all inputs and OTA states.
**Context:** The mobile app's authentication behavior is not present in this
repository, and an uncoordinated firmware-only change would lock the owner out
of configuration and OTA. The AP is local-only, but its shared password is not
a substitute for endpoint authorization.
**Consequences:** Input-safety and OTA-correctness work is complete; endpoint
authentication remains an explicit release risk that must be implemented with
the matching mobile-app update.

## D15 — Keep diagnostic telemetry enabled through the soak window
**Date:** 2026-09-01
**Decision:** Keep `DEBUG=1` during the 7-day soak and emit free heap, maximum
free block, fragmentation, and maximum loop duration once per minute.
**Context:** One line per minute is negligible compared with P10 refresh and
provides objective evidence for leaks, fragmentation, blocking, WDT, and reset
trends. The dangerous test `rtc.adjust()` call is now commented out.
**Consequences:** Serial must remain connected for the soak logger. After the
soak passes, release can set `DEBUG=0` and rebuild without changing behavior.

## D16 — Never reduce a non-zero brightness to zero in power-save mode
**Date:** 2026-09-01
**Decision:** Power-save halves brightness only when the configured value is
greater than one; values zero and one remain unchanged.
**Context:** The live device had `brightness=1`. Integer division produced zero
after the calculated Isha window, blanking every pixel while Test Mode still
worked at brightness one.
**Consequences:** The fix is now included in the experimental font firmware on
the device. Stored brightness two remains a useful visible test value.

## D17 — Ship three selectable font families as paired 7px/16px assets
**Date:** 2026-09-02
**Decision:** Store up to three complete font families in firmware. The user
selects `fontFamily` independently from the existing small/large option.
**Context:** The owner wants a font selector in the Android app. Fixed generated
GLCD assets are safer and much smaller at runtime than parsing TTF or loading
mutable font files from flash storage.
**Consequences:** Font data consumes Flash only; runtime RAM impact is a config
byte and a selected pointer. The new Configuration field, API response, and
Android selector must ship together with an EEPROM version bump.

## D18 — Expand the experimental selector to four font families
**Date:** 2026-09-02
**Decision:** Supersede D17's three-family limit during experiments with four
paired families: BasitFont (legacy Janat artwork), corrected Traditional,
Tahoma, and Arial Bold. The selector remains RAM-only until the Android app and
EEPROM migration are released together.
**Context:** Physical P10 comparison is more reliable than desktop previews,
and the owner requested a visibly heavier fourth option. Decorated alefs in
Traditional are rendered as plain alef so a larger unclipped pixel size fits
the 16-row panel.
**Consequences:** ESP8266 RAM remains unchanged at 35,452 bytes; Flash rises to
377,063 bytes (36.1%), leaving ample OTA headroom. BasitFont fills absent legacy
forms by reusing the closest contextual Janat bitmap of the same letter.

## D19 — Return to three families and accept the Android numeric API contract
**Date:** 2026-09-02
**Decision:** Remove Traditional, keeping BasitFont, Tahoma, and Arial Bold.
Numeric API arguments accept the Android app's plain values first and retain
blur-decoding only as a compatibility fallback.
**Context:** Physical review rejected Traditional. Android logs proved the app
sends `y=2026`, `dst=2`, etc. as plain query values; treating `2026` as one
blur byte caused the otherwise successful HTTP request to return `Error`.
Generated TTF glyphs also retained blank side-bearing columns on joining edges.
**Consequences:** The generator trims only contextual joining edges, preserving
separation for isolated/right-joining letters. `/setFont` returns to `0..2`.

## D20 — Commercial v2 uses per-device HMAC without JSON
**Date:** 2026-09-02
**Decision:** Use ordered UTF-8 `key=value` lines over HTTP POST with
HMAC-SHA256, one-time RAM nonces, and a unique 32-byte key per device. Android
imports the key from a physical QR into Android Keystore. Wi-Fi password and
HMAC key are separate. Support compile-time P10 profiles 1x1 through 4x1.
**Context:** The owner rejected JSON/ArduinoJson overhead and 4x2 support while
requesting a simple commercial security model and project continuity.
**Consequences:** v1/blur remains only during coordinated Android migration.
Diagnostics are protected service data, and automatic ESP8266 OTA rollback is
a separately gated bootloader task. Full handoff: `COMMERCIAL_V2_PLAN.md`.

## D21 — Replace physical QR with on-screen pairing and an admin password
**Date:** 2026-09-02
**Decision:** Do not print the HMAC key or depend on a permanent QR. On first
setup/reset, P10 shows a short-lived six-digit pairing code. The user chooses
an admin password; Android derives the per-device key with PBKDF2-SHA256 and a
random public salt, then stores it in Android Keystore. A new phone derives the
same key by entering the admin password.
**Context:** A mosque clock may be mounted high and labels can be erased or
lost. The display is visible, so a temporary code is more practical.
**Consequences:** Pairing is limited to five attempts and ten minutes. Forgotten
credentials require physical factory reset; an unauthenticated remote reset
would defeat the security model. D21 supersedes D20's physical QR detail.

## D22 — One customer password plus a physical factory-reset button
**Date:** 2026-09-02
**Decision:** Remove QR, pairing codes, and the separate admin password. The
customer manages one Wi-Fi password. API v2 derives its per-device HMAC key
from that password and deviceId using PBKDF2-SHA256. The app forces changing
the shared default on first connection. A recessed button held for ten seconds
performs a full factory reset if the password is forgotten.
**Context:** Mosque installers need a workflow explainable in one sentence and
devices may be mounted where labels are inaccessible.
**Consequences:** Anyone who knows the Wi-Fi password can administer the clock;
the AP must not be shared publicly. Default-password security is intentionally
limited until the mandatory first change. D22 supersedes D20/D21 pairing UX.

## D23 — Persist commercial settings in EEPROM v4
**Date:** 2026-09-02
**Decision:** Bump `EEPROM_VERSION` to 4 and persist the AP password,
`mustChangePassword`, and selected font. Use GPIO2 with `INPUT_PULLUP` for the
ten-second factory-reset button.
**Consequences:** The first v4 boot resets the old experimental configuration
once. GPIO2 must be released while powering/resetting the ESP8266 because it is
a boot-strap pin; factory reset is pressed only after normal boot.

## D24 - Commercial device allowlist and signed firmware gate
**Date:** 2026-09-05
**Decision:** Before commercial operations, Android must verify that the
device serial and reported model match the local customer database. Firmware
updates must also pass a signature/authenticity check before installation.
**Context:** Prevent sending an image to the wrong clock and prevent modified
or unintended firmware from being accepted in the field.
**Consequences:** A missing/unknown serial or model mismatch fails closed.
Signature verification is a release-gate feature; it does not change the
current experimental firmware behavior.
## D23 — AP password is the only customer access control

**Date:** 2026-09-05

The customer-facing flow does not ask for a second application password or
HMAC key. The local AP password is the access boundary; API-v2 keeps its small
`key=value` POST format without requiring HMAC headers. Factory reset restores
the default AP password.

## D25 - Remove unused HMAC/challenge runtime code

**Date:** 2026-09-05

The firmware and Android client no longer carry the unused HMAC derivation,
nonce challenge, or authenticated-response path. This removes boot-time PBKDF2
work and dead header parsing while keeping signed OTA verification. The compact
POST format and AP password boundary remain unchanged.

## D26 - Keep legacy OTA as the stable commercial path

**Date:** 2026-09-28

**Decision:** Keep `/updateMain` as the single customer-facing OTA endpoint and
continue using the existing raw firmware assets. Do not require device
capability detection, signed-image selection, or a second update workflow.

**Context:** Existing market devices use the legacy updater and the owner
prioritizes uninterrupted support and a simple customer experience over the
optional signed OTA experiment.

**Consequences:** `/v2/update` remains engineering-only and is not advertised
to customers. A future migration requires a separate approval and must preserve
the raw `/updateMain` path until all compatibility risks are resolved.

## D27 - Present completed P10 frames with double buffering

**Date:** 2026-09-28

**Decision:** Enable DMDESP double buffering and call `swapBuffersAndCopy()` at
the end of each loop/frame path, including splash, test, and OTA states.

**Context:** Text drawing clears and repaints the live framebuffer. On a 4x1
display this can expose the intermediate clear/partial glyph state as a light
refresh-like twitch. DMDESP explicitly supports double buffering for this
tearing case.

**Consequences:** The extra 4x1 framebuffer is about 256 bytes of heap. The
release build remains within the existing RAM/Flash limits; hardware upload is
required to confirm the visual result.

## D28 - Present only dirty display frames

**Date:** 2026-09-28

**Decision:** Track framebuffer changes in the text engine and direct drawing
paths. `swapBuffersAndCopy()` runs only when the back buffer changed; the
DMDESP timer flag is declared `volatile` because it is written by the timer
callback and read by the main loop.

**Consequences:** Unchanged frames no longer incur a full framebuffer copy,
while completed-frame presentation and timer visibility remain explicit.

## D29 - Persist card schedule as a bounded EEPROM sidecar

**Date:** 2026-09-28

**Decision:** Store a 12-bit enabled mask and a validated 12-byte card order in
a small CRC-protected EEPROM sidecar after the existing configuration metadata.
Expose compact legacy GET endpoints plus matching v2 POST wrappers for the
Android P10 settings dialog.

**Consequences:** Existing configuration offsets, `EEPROM_VERSION`, `/getData`,
and `/updateMain` remain compatible. Invalid sidecar data falls back to all
cards in the default order without writing on boot.

## D30 - Narrow-panel small-font information cards (2026-09-28)

**Decision:** On panels up to 64 pixels wide, keep the clock in the fixed top
row and render day name, Gregorian date, and Hijri date as normal rotating
cards in the lower row. On wider panels, keep the existing top-right rotation.

**Context:** Narrow panels have no independent top-right information area; the
old small-font guard skipped these cards completely.

**Consequences:** No EEPROM/API change and no extra RAM. The owner must verify
the 1x1 and 2x1 physical layouts; 4x1 behavior is unchanged.

## D31 - Enable sunrise iqama configuration (2026-09-28)

**Decision:** Preserve and expose `eqamaShrouq`, defaulting a zero legacy value
to 10 minutes. Include it in the v2 data response and use it in the sunrise
window instead of discarding the setting.

**Consequences:** Existing devices migrate the zero value once; the Android
app can display and save the value. Sunrise remains a separate event name, but
its configurable iqama/sleep window is now consistent with the other entries.

## D32 - On-demand technician health diagnostics (2026-09-28)

**Decision:** Reuse the existing authenticated `/v2/health` endpoint and extend
its compact text response with firmware/config version, reset reason, uptime,
heap metrics, RSSI, AP clients, RTC state, and age of the last RTC sync. The
Android app exposes it only after the existing seven-tap technician unlock.

**Consequences:** No EEPROM writes, periodic logging, JSON dependency, or
customer-facing control is added. Release builds can override
`FIRMWARE_VERSION` at compile time; the default remains `P104-dev`.

## D33 - Non-blocking RTC/I2C recovery (2026-09-28)

**Decision:** Run the DS3231 bus at 100 kHz, recover a stuck SDA/SCL bus with
at most nine short SCL pulses, and retry RTC initialization on a 30-second
schedule without restarting the ESP8266. All RTC reads and writes remain
guarded by a successful initialization and a valid date.

**Consequences:** A missing or electrically unstable RTC no longer causes a
per-second blocking access or a restart loop; the cached/software clock and
display continue while recovery is retried.
The date-setting command may still initialize a physically responding RTC even
when its stored date is invalid, so a lost battery can be corrected from the app.

## D34 - Local RTC fault indicator (2026-09-28)

**Decision:** During the startup splash only, blink one P10 pixel or one unused
7-segment position when RTC initialization is unavailable. Clear it when RTC
recovers or the splash ends.

**Consequences:** A technician can notice an RTC fault without Wi-Fi or the
Android app, while the normal clock layout remains untouched after startup.

## D35 - Central display geometry profile (2026-09-28)

**Decision:** Keep physical display geometry and derived layout zones in one
compile-time `DisplayProfile`. Card placement, small-font areas, and top-line
availability use that profile instead of repeating width-specific conditions.

**Consequences:** Adding another one-row panel width requires one profile build
value and avoids inconsistent card placement. No EEPROM/API change or runtime
RAM allocation is introduced.

## D36 - Technician production self-test (2026-09-28)

**Decision:** Add an authenticated, no-flash-write `/v2/selfTest` endpoint that
checks configuration validity, RTC state, display initialization, AP mode, and
minimum free heap. It is a factory/technician check, not a customer feature.

**Consequences:** A production script can reject a bad unit before delivery
without adding JSON or persistent state. Display health means successful driver
initialization; LED pixel visibility remains a physical check.

## D37 - Reset diagnostics detail (2026-09-28)

**Decision:** Keep the reset reason and add the ESP8266 reset-info string in RAM;
expose both through the existing health response without writing flash.

**Consequences:** WDT, exception, and brownout investigations get more context
while preserving the existing wear-free diagnostics model.
## D38 - Compatible on-panel OTA progress (2026-09-30)

**Decision:** Keep commercial `/updateMain` raw OTA unchanged except for an
optional validated `size` query parameter. Draw progress from the shared upload
callback, not normal `loop()`, because multipart parsing runs inside
`server.handleClient()`. Reuse BasitFont7 and existing double buffering; redraw
changed percentages no faster than every 150 ms. Show at most 99% before
verification and 100% only after `Update.end(true)` succeeds.

**Consequences:** Old apps without `size` show an indeterminate indicator and
remain supported. Android's size parameter is ignored by old firmware. Success
holds a static localized message until the existing five-second restart; narrow
panels split it across two lines. Failed/aborted uploads restore the user font
and cards. No new library, heap allocation for drawing, or EEPROM layout change.
The existing engineering-only signed endpoint remains separate. Owner must
verify physical panel behavior after installing this firmware once.

## D39 - Pause DMD during blocking OTA (2026-09-30)

**Decision:** Pause DMD output for the complete multipart upload. The network
parser and flash writer are blocking, so attempting to animate a progress bar
would compete with the display refresh path. Resume DMD only after upload
completion to show the final result and restart.

**Consequences:** The P10 is intentionally blank during transfer; the Android
screen remains the progress indicator. This removes display/network contention,
avoids timer-driven WDT risk, and preserves normal DMD operation outside OTA.
## D40 — Keep two production font families and expose language selection

**Decision:** Remove Arial Bold from the production firmware and Android selector.
Keep Basit/Janat and Tahoma only. The firmware normalizes legacy EEPROM value
`fontFamily=2` to Basit at boot, so existing devices retain their settings.
`/setFont` accepts `font=0..1`, `lang=0..1`, and `digits=0..1`; Arabic remains
the default while the existing translated display strings can be tested in
English.

**Reason:** Arial increased flash usage and duplicated the role of the existing
bold Tahoma option. English support is intentionally restored for field testing;
any card still lacking a reviewed English translation remains Arabic until its
text is audited.

## D41 — Hybrid Latin glyphs and font tree cleanup (2026-09-30)

**Decision:** Keep the Janat-derived Arabic slots in BasitFont, but use the
same Tahoma-derived 7px/16px artwork for its ASCII letters and punctuation.
Western and Indic digits are generated by `build_digit_fonts.py` and are never
overwritten by the Latin overlay. Regenerate with the digit step followed by
`tools/build_english_hybrid.py`.

Remove 13 legacy font headers that are not used by the firmware; retain only
runtime families plus the source/example headers required by the generator and
vendored DMDESP examples.

**Consequences:** English mode is consistent across both families and the
dedicated numeral artwork is preserved. The four release profiles build
successfully and the native suite remains 19/19; future font changes must rerun
both generators.

## D42 — English inter-letter spacing (2026-09-30)

**Decision:** Add one pixel between adjacent ASCII letters in English and
between adjacent digits in Arabic while drawing and calculating text width.
Other punctuation and spaces keep their native widths; Arabic letters remain
zero-spacing and connected.

**Consequences:** Tahoma-derived English words no longer touch, while Arabic
numeric strings become readable without separating joining letters.

## D43 — Configure all PrayerTimes twilight rules explicitly (2026-10-01)

`calcPrayers()` now uses a small source-owned method table and explicitly sets
Fajr, Maghrib, and Isha on the library's `Custom` slot. The vendored header
does not declare its existing minute setters, so the firmware provides narrow
forward declarations without modifying `lib/PrayerTimes`. Standard mappings are
Egypt 19.5°/17.5°, MWL 18°/17°, Karachi 18°/18°, Custom 18°/17.5°, and Makkah
18.5° Fajr with a compatibility baseline of 17° Isha. Maghrib is sunset plus
zero minutes for these mappings. Makkah's fixed-90-minute option is documented
in D45.

The previous code selected a method and then called Fajr/Isha setters, which
forced `Custom`; it also left Maghrib mode/value implicit in zero-initialized
library state. Native tests cover the method table. Numeric reference cases and
the product meaning of Makkah/eqama settings remain in
`docs/PRAYER_TIME_SPEC.md` pending independent/local timetable verification.

## D45 — Make Makkah Isha rule selectable without changing EEPROM layout (2026-10-01)

Keep the existing 452-byte `Configuration` and `EEPROM_VERSION=6`. Store one
validated mode byte plus a compact CRC in the four-byte gap between `ConfigMeta`
and card preferences at offset 460. Mode `0` means the selected calculation
method (default for compatibility); mode `1` means 90 minutes after Maghrib when
`cm=1`. Expose it only as optional `makkahIsha` in v2 get/set responses so old
CSV clients and seven-segment firmware remain unchanged.

## D46 — Key the prayer cache by the complete Gregorian date (2026-10-01)

`PrayerCache` now stores year, month, and day instead of only the day number.
This prevents a stale prayer schedule when the RTC crosses a month/year boundary
with the same numeric day. The fields are runtime RAM only and do not affect
EEPROM layout or the public API.

## D47 — Reject malformed OTA numeric metadata (2026-10-01)

The v2 OTA manifest now parses `version` and `size` with the existing strict
decimal parser instead of `strtoul`, which previously accepted trailing junk
such as `123abc`. This is a validation-only change; the signed/hash and profile
checks remain unchanged.

## D48 — Key Hijri cache by the complete Gregorian date (2026-10-01)

The Hijri cache now uses the same year/month/day key as the prayer cache. This
avoids retaining the previous converted date when only the Gregorian month or
year changes while the day number stays equal. It is runtime-only and leaves
EEPROM and API compatibility unchanged.

## D49 — Catch up bounded software-second ticks after short loop stalls (2026-10-01)

The loop now consumes elapsed whole seconds and calls the existing `tickSecond()`
up to 60 times per pass. The deadline advances from its previous value instead
of being reset to `now`, so a 2–5 second blocking request does not silently lose
clock seconds. The bound prevents an unusually long stall from monopolizing the
loop; RTC synchronization remains the authoritative correction.

## D50 — Centralize prayer-window phase boundaries (2026-10-01)

Keep the existing display behavior, but classify `since/eqama/sleep` once with
`prayerPhase()`. The renderer consumes the same phases for pre-azan, azan,
countdown, final seconds, eqama, sleep, and azkar. Friday's special path still
skips eqama before classification. This removes duplicated boundary predicates
without introducing a general state-machine framework.

## D51 — Keep 7-segment comparison isolated (2026-10-01)

The separate 7-segment repository uses a different web stack, configuration
layout, and prayer-index loop. Its current loop skips `times[5]` (Maghrib) and
uses `times[4]` for the Maghrib field, while P10 follows the verified mapping
where index 4 is Sunset and index 5 is Maghrib. Record this as a P1 finding and
do not silently patch either repository until a fixed-date numeric and hardware
comparison is approved. Legacy P10 API and sold-device compatibility remain
unchanged.

## D52 — Bound malformed UTF-8 decoding (2026-10-01)

`ArabicShaper` now checks continuation bytes before looking ahead and consumes
the remaining continuation bytes of one malformed sequence. A truncated UTF-8
sequence therefore renders as one `?` without reading beyond the NUL-terminated
input. Valid Arabic, presentation forms, digits, and existing golden vectors
are unchanged.

## D53 — Align the separate 7-segment prayer mapping (2026-10-01)

The 7-segment repository now uses the same explicit angle table as P10, sets
Maghrib to zero minutes after Sunset, reads Maghrib from `times[5]`, and skips
Sunset (`times[4]`) for prayer events. The change is isolated in its own
repository commit `8996bab`; P10 legacy/v2 APIs and EEPROM layout are unchanged.

## D54 — Use one prayer contract in both display firmware paths (2026-10-01)

The verified library indices remain `Sunset=4`, `Maghrib=5`, and `Isha=6`; the
event list is `{Fajr, Sunrise, Dhuhr, Asr, Maghrib, Isha}`. P10 now uses the
same explicit index contract as 7-segment. Both paths also use the same method
table and Makkah Isha modes: `0` follows the selected method rule and `1` is a
fixed 90 minutes after Maghrib. Legacy CSV responses and existing EEPROM/API
layouts remain compatible; new settings are optional and presentation stays
profile-specific.

## D55 — Share the seasonal date rule without renaming legacy fields (2026-10-01)

P10 and 7-segment now evaluate the same date-only rule: seasonal adjustment is
active from the last Friday of April through the last Thursday of October,
inclusive, when enabled. P10 keeps `summerTime`; 7-segment keeps its existing
`winterTime` field because it is part of the sold-device/API contract. The RTC
stores the unshifted value and display/prayer calculation applies the same
one-hour presentation offset, so only the hardware presentation differs.

## D44 — Normalize Iqama countdown distance across midnight (2026-10-01)

Use `forwardMinuteDistance()` for the displayed minutes remaining until Iqama.
The active-window helper already accepts end minutes beyond 1440, but the old
format argument subtracted raw clock minutes and could display values above one
day after midnight. The helper keeps the existing window boundaries and only
normalizes the countdown to 0..1439.

## D56 — Verify raw OTA EC signatures with the parsed BearSSL point (2026-10-02)

The v2 OTA signature remains fixed-width P-256 `r||s` (64 bytes). BearSSL's
PEM parser exposes the public EC point without the SEC1 `0x04` prefix, so the
firmware verifier adds that byte in a bounded local copy before calling the raw
verifier. The signing tool now emits the same raw format. The restart delay is
10 seconds so the success response can leave the socket before the AP reboots;
legacy `/updateMain` compatibility is unchanged.

## D57 — Treat signed OTA socket closure as a post-reboot confirmation case (2026-10-02)

Live P10 tests accepted the signed image, rebooted, and returned healthy
`/v2/health` with `rtc=1`, but the multipart HTTP client did not receive the
success body before the ESP8266 closed/restarted the socket. The firmware keeps
the official `NoDelay/flush/delay/stop` response sequence; the Android client
should confirm a closed OTA request with a bounded post-reboot health check
instead of declaring the update failed. No change is made to legacy update
compatibility until that client behavior is verified.

## D58 — Keep one commercial API and one OTA path (2026-10-02)

The product contract is the compact root-path POST API, `GET /checkUpdate`, and
`POST /updateMain`. Old blurred GET handlers and the unused signed `/v2/update`
verifier were removed from both firmware paths. Sold devices are migrated by
the Android update check before normal control requests, so retaining dead
handlers only increased flash size and duplicated behavior. The AP password
and the existing request validation remain the access and safety boundaries.

## D59 — Suspend DMD's SDK timer during OTA (experiment, 2026-10-05)

Owner approved one isolated test of the intermittent early-OTA hang.
Previously DMD pause blanked output and skipped scanning but left the
100-microsecond SDK timer armed. Pause now disarms that timer; resume
rearms the same callback at the same interval only when paused. Existing
OTA success/failure/abort paths already call resume. No refresh rate,
transport, update version, or EEPROM change is included.

This small DMDESP core edit is necessary to test complete suspension using
its private timer without introducing application-level access to library
internals. It is NOT a proven diagnosis or fix; if hangs recur, preserve
the failure evidence before manual power recovery and reassess.

Outcome recorded 2026-10-05: the owner's 06:18 serial log reproduced
the early reboot using the new 397888-byte image. Timer suspension alone
does not fix the fault; this decision remains an experimental lifecycle
change, not a verified cause/remedy. No further diagnostic DMD changes were
approved in that turn.

### D60 — Unchanged frames and one night window (2026-10-05)

Owner approved the simple completion batch; dual-copy storage was declined.
Preserve existing EEPROM/LittleFS layouts and unchanged-write guards.
`swapBuffersAndCopy()` compares the existing buffers before swapping/copying;
no extra P10 RAM buffer or probabilistic hash. Necessary DMD core exception:
front/back buffers are private; pixel iteration or another full-frame copy
would cost more. Panel multiplex scanning continues for static frames.

7-segment TA6932 tracks 16 published bytes plus control state per chip;
unchanged images are not resent, brightness/on-off-only changes issue one
control command. `reset()` force-sends and resynchronizes the cache. This is
an explicitly documented local driver optimization, not an OTA fault remedy.

Both firmware contracts now use identical normalized `[start,end)` night
windows. Positive after-Isha/before-Fajr offsets enable the window; zero
retains the established disabled convention. Equal boundaries disable it
instead of hiding the clock all day. Removed dead divergent night arithmetic
from 7-segment Checking; dimming and ultra mode use the same helper.

OTA 98 marks end-of-upload/finalization; 99 remains after successful
Update.end. Android 100 requires HTTP UpdateOk or that peer acknowledgement,
then shows the existing five-second restart grace period, NOT a post-boot
verification. Early reset/power/ground investigation remains unresolved.

### D61 — Always-available protected AP (2026-10-05)

Owner approved removing the 7-segment ten-minute AP shutdown, without a new
setting or changing credentials. Removed its timer, flag and loop branch in
Masjed_TA_All_v2/src/Masjed_TA.cpp. Both M73/M73s follow the same policy as
P10, including night dimming and ultra display mode. P10 already has no
scheduled shutdown, so no P10 source change is needed.
`closeAllWifiServices()` is still called in setup before AP/server startup;
keep that initialization cleanup, not a runtime AP-expiry mechanism.
No EEPROM/LittleFS/version/Android UI changes. Hardware/reset fault remains
separate; always-on AP does not prove that issue fixed.

### D62 — Exact OTA size and actionable failures (2026-10-05)

Owner approved proposals 1 and 3 only, across P10, M73/M73s and Android.
No MD5/signatures, new transport, polling, auto-retry or storage layout change.
`size` is mandatory positive decimal uint32 (checked for overflow), bounded
by the available sketch staging space. Update.begin receives this exact size.
Each accepted write increments a received-byte counter; excess chunks are
rejected before writing, missing bytes are rejected at file end. Only the
completed multipart request may call Update.end(false), then send p=99 and
UpdateOk. A completed file part alone is NOT installation acceptance.

The installed ESP8266 core 3.1.2 has no public abort method. Cleanup forces
its write bounds error via remaining()+1 before end(); the core checks the
length before reading the one-byte argument. This avoids accidentally
committing an otherwise full image when its request is aborted/malformed.
Re-check this contract before changing the ESP8266 core. No vendored library
edits were needed. Async 7-segment tracks the owning request, rejects another
upload without replacing its state, and releases staging memory on disconnect.

Both clocks return UpdateError:<code>. Android translates size, incomplete,
oversize, space, memory, write, image, token, busy, aborted and begin/end into
Arabic/English messages. Partial-send socket failures identify interrupted
upload; full-send failures without firmware ACK remain unconfirmed and direct
the owner to the normal version check. Sent bytes alone never imply success.
Existing five-second restart grace, actual versions and owner's test target
remain unchanged. This work is not a fix for the intermittent early reset.

### D63 — Authoritative RTC and prayer event correctness (2026-10-07)

Owner authorized comprehensive-review repairs; batch 1 covers P10 R01-R10/R19.
Keep the approved PrayerTimes formula, base-date seasonal policy, EEPROM v6
layout and commercial API. Software ticks are pure; one authoritative RTC
sample rebases the deadline after catch-up. Compare full dates/minutes for
cache/window changes. Prayer offset follows the RTC-applied seasonal state;
reject spring-gap local inputs that cannot round-trip under the existing rule.

Read bounded DS3231 registers/status directly through Wire to verify transport,
BCD, calendar ranges and OSF before trusting time. Validate writes with a
two-second readback tolerance; restore previous time on transaction failure.
If restoration is uncertain, one runtime-only flag requires a successful
manual time/config transaction. Transient invalid reads remain retryable;
night display cannot override invalid-time feedback. This is not atomic
RTC/Flash persistence: EEPROM staging/commit coherence remains R18.

Use existing EventIndices/phase helpers for Friday/Maghrib and long iqama.
Nonfinite library outputs become unavailable sentinels, never clock values.
Priority text is serviced each frame; last-minute countdown fits the selected
geometry without new fonts or render buffers. Preserve explicit sunrise zero
in v6; default ten and historical migrations remain unchanged.

No version bump, hardware upload, Android asset publication or 7-segment
source change in this batch. No claim of fixing intermittent OTA resets.

### D64 — Local parser hardening and bounded persistence (2026-10-07)

Keep the compact root POST API and the installed ESP8266 core. The project uses
a build-local overlay for the pinned Core 3.1.2 parser; it never edits `lib/`
or the shared SDK. Multipart reads have idle/total deadlines, strict trailers,
32-part and retained-data limits, stale-plain reset, nothrow allocations and
abort cleanup. EEPROM keeps layout/version 6: stage into the existing mirror,
commit once, restore the prior mirror on failure, and publish runtime values
only after success. No A/B image or new filesystem is justified by the current
product scope.

### D65 — Display invalidation and release identity (2026-10-07)

Use one display revision invalidation path after a full clear; do not add a
second framebuffer or an async display layer. Release tooling must receive an
explicit version/tag, reject an existing manifest, and require the actual
Android project path. These are build/release guardrails only; Android assets,
7-segment code, hardware upload and commercial version publication remain
separate owner decisions.

### D66 — Cooperative OTA writes without a progress peer (2026-10-07)

After each accepted HTTP upload chunk, call yield() rather than watchdog-only
feeding. Sending UDP progress already invokes optimistic_yield in the installed
core; upload service must not depend on the existence of that peer or DEBUG
serial output. Keep native Updater buffering, exact-size/finalization checks,
and the five-second restart grace. No new library, delay or polling.
Three short release OTA attempts without a progress peer passed after this
change; RTC stayed valid with the owner's battery fitted. This observed
improvement does not establish the physical cause of every previous reset.

### D67 — Full location identifiers and remaining parity (2026-10-09)

Use signed 32-bit storage and accept nonnegative city IDs through INT32_MAX,
matching existing Android catalog IDs without truncation or database renumbering.
P10 configuration version becomes 7 (not firmware release version). Unlike old
reset-on-version changes, valid v6 LittleFS records migrate in RAM only after
the original CRC passes. Moving the tail right two bytes consumes existing
padding, so record length and all existing fields remain intact. The next normal
save persists v7; boot itself does not write. Native real-struct fixtures verify
passwords, texts, cards, font/digit/hour choices and corruption rejection.
Downgrading to a v6-only firmware after a v7 save is not supported; its old reader
would reject the new record. Do not publish a rollback without a matching migration.

Shared factory iqama values are Android's existing `{15,10,10,10,8,10}`. Both use
timezone 2, night offsets 30/30 and unconfigured placeholder coordinates
10.05/10.05; users must select their actual location. These affect factory reset
or first boot only. M73 previously used Fajr 13, 55.05/55.25 and zero offsets;
valid saved values are retained. Rendering, audio, fonts and device-specific
brightness remain separate; no new dependency or generic runtime layer.

Keep a shared 512-byte POST buffer, consistent error statuses and literal v=2.
Six date/time fields and both coordinates are required. Full committed config
validation is reused for M73 selfTest. Five-second restart grace is shared and
M73 handles millis rollover. Its current synchronous OTA callback yields after
accepted writes, not merely feeding WDT; this is not proof of a hardware reset fix.
M71 is explicitly deferred to its own plan and must never receive an M73 image.

### 2026-10-11 — Safe storage boot and shared iqama limits

Owner approved the seven-segment audit fixes and explicitly limited iqama to
0–99 minutes across both clock families. Zero keeps its disabled-iqama meaning.
Old valid100–120-minute records normalize to99 only on load; all other fields
and storage layouts/versions remain unchanged. P10 checks the original CRC
before normalization, and neither clock performs a migration write on boot.
The shared prayer phase gives 1/2-minute iqamas precedence over the two-minute
azan window, without changing the approved prayer-time calculation formula.

LittleFS auto-format is disabled on both families. A failed mount formats only
a partition verified fully erased (FF); a failed read of an existing config
does not replace it with defaults. RAM defaults keep the AP usable but are not
treated as committed settings: E3/getData storage error and no prayer display.
P10 validates the whole staged record, including sidecars/cards, before writing.
This favors retaining recoverable customer data over silently hiding corruption;
a damaged, non-erased filesystem requires deliberate recovery, not boot erasure.

M73s retains TX-only JQ6500 wiring. Its existing packet protocol is sent without
an impossible RX response wait, preserving100ms command spacing and500ms reset
settling. No vendor library was edited. Physical audio/RTC/OTA/soak verification
is pending; all builds were local with asset publication disabled.

### 2026-10-11 — Deep-review OTA and M73 storage corrections

Freeze OTA callbacks after an accepted image schedules restart; no second upload
may alter the pending eboot image. Both pinned build-local parser overlays check
plain argument allocations at their callers and preserve the previous upload
object on second-file OOM so abort cleanup can release application request state.

M73 storage format2 adds magic/version/payload-size/CRC32 while retaining the raw
Configuration layout and existing XOR payload encoding. Check CRC before
decryption/migration. Read supported legacy layouts without boot writes/reset;
upgrade on the next explicit save, verifying the temporary file before rename
and RAM commit. Older firmware cannot read the new format: downgrades require
deliberate conversion/a compatible reader. This was approved by the owner.
CRC is accidental-corruption detection, not authentication or copy protection.

Treat failed/short AP password reads as unavailable, never a truncated/default
password. Numeric fallback and first-boot defaults require confirmed NOENT, not
FS::exists false (which also hides I/O errors). A local LittleFS lookup accessor
is specific to the pinned ESP8266 Core3.1.2 implementation; revisit it on SDK
changes. Failed reset compensation stops active config and keeps ECO coherent;
successful recovery refreshes time/prayer caches before acknowledgement.

No new journal, dependency, release version, Android asset publication or upload.
Power-loss cross-file atomicity and physical RTC/OTA/soak remain unverified.

### 2026-10-12 — Owner-authorized DP5125D latch timing experiment

The connected P10 panels use DP5125D. Chip identity alone does not establish
panel scan ratio; keep the working four-phase row mapping, 10MHz SPI, 1ms scan
schedule and configured brightness. The experimental `p10_4x1_dp5125d` build
set `DMDESP_DP5125D_TIMING=1`; all existing profiles retained original timing.
The final trial's vendored-driver change was isolated to the scan handoff: force
OE high instead of floating it, select the row, pulse LAT after all SPI data
with 1us settling intervals, then restore PWM. No current/gain registers or
FM6126A initialization are sent; SPI data transfer stays unchanged.

Reference: https://github.com/ironsheep/P2-HUB75-LED-Matrix-Driver/blob/main/HUB75-Driver-SWver1.md
documents DP5125D LAT_POSN_OVERLAP/LAT_STYLE_OFFSET without INIT_PANEL_REQUIRED.
This is a reference-derived experiment, not a manufacturer-datasheet guarantee
or a proved flicker fix. A real-function host check compares all framebuffer
bits against the original path for 1-4 panels, both orientations and all phases;
it checks no clocks under LAT, blanking and retained SPI modes. Electrical timing
and shimmer need owner observation. Baseline BIN SHA256:
`9F46E6DF2F4BCA0071BB7E87754DABF16E8F5623C05CD6A3C2F0D64D75FE9230`.
Rollback uses the saved original BIN or builds/uploads `p10_4x1`.

The first OFFSET candidate was uploaded with flash hash verified. Owner observed
no shimmer but dim random pixels around letters. Serial captured a Hardware
Watchdog reset after `loopMax=9760us`; causation is undetermined. The second
candidate keeps the same LAT overlap but extends blanking past LAT falling to
test whether transitional exposure causes ghosting. Treat both visual quality
and restart stability as pending; never infer either from a successful build.

Owner confirmed second candidate still has dim stray pixels but steady glyphs.
No reset was observed in its 65s serial capture (`loopMax=9798us`), which is
not a soak test. Third candidate removes LAT overlap and final-byte GPIO
transfer completely, retaining explicit OE blanking to isolate these effects.
The RGB-panel reference above does not guarantee overlap suits this monochrome
panel. The original row order, SPI bytes and speed remain exact.

Closed/rejected: owner reports the third candidate reduced stray pixels greatly
but did not eliminate them, and original glyphs shimmer again. Restore exactly
the saved pre-experiment driver and BIN; retain the already-approved splash,
RTC startup and DEBUG Wi-Fi changes. Remove temporary build flag/profile/test
from the repo (test archived with the baseline outside the repo). Do not add a
fourth guessed timing patch: loopMax is not a scan/OE waveform measurement.
The next investigation needs actual scan/OE timing and reset evidence.

### 2026-10-12 — Scan scheduler measurements after rejected timing trials

Owner requested continuation. `p10_4x1_scan_diag` enables DEBUG and a build-only
`DMDESP_SCAN_DIAGNOSTICS` flag. Necessary vendor hooks timestamp refresh start
and the software point immediately before LAT, then report durations outside
the refresh path. Original SPI, GPIO, OE, PWM and scan schedule remain intact;
normal/release builds omit all hooks and counters. Reset the previous LAT sample
on pause/resume to exclude intentional OTA blanking. Ten-second summaries show
scan gap minimum/mean/maximum, gaps over5ms, max scan duration and separate maxima
for services (including RTC), yield, HTTP and rendering. Serial summary time is
excluded from the next measured gap; logging itself may cause a brief twitch.

LoopMax alone cannot prove scan starvation because SmartText refreshes from
inside rendering. These software timestamps quantify scheduling and transfer
duration, not electrical OE/PWM waveforms or signal-level integrity. Host check
executes real stats/refresh source and confirms identical GPIO/SPI ordering
with instrumentation on/off, all orientations/phases for1-4 panels, rollover
and intentional-pause exclusion. No speculative fourth timing patch.

Correct DEBUG command A to call server.begin after initAP: W had stopped the
HTTP server, so restoring only AP left phone API unavailable. Existing normal
DEBUG builds receive this one-line command correction; display timing is still
unchanged. Before that correction the normal BIN was byte-identical to baseline.

The connected-phone comparison measured LAT gaps33.5-43.6ms against HTTP
24.98-39.22ms, with15 intervals over5ms in40s. Stable Wi-Fi OFF gaps3.459-4.211ms
had no over5ms intervals. Idle AP without peers was comparable to OFF; Wi-Fi
radio enabled by itself was not proved the cause. SDK WiFiClient::flush calls
wait_until_acked, blocking the foreground until TCP acknowledgement. Remove
the explicit flush after server.send in sendV2: the normal server owns response
finalization/connection lifecycle and queues copied bytes; five-second restart
grace remains. Preserve body/status/content type and commit-before-success.
This is a measured-network-path candidate, not an additional LED timing guess.
Real-source reply test first fails on a40ms remote-ACK stand-in, then passes
for success/error responses. Physical before/after and residual jitter pending.

Reply fix preserved successful phone changes but did not improve shimmer by
owner observation. Measured HTTP/scan gaps remain long. Old P170 code's
dmdInit is actually called from setup and attaches a timer0 refresh ISR;
current DMDESP relies on foreground loop. Returning HTTP earlier cannot
provide independent scanning during all remaining blocking operations.

An isolated SDK timer experiment is warranted by this evidence. Use an embedded
SDK os_timer per DMDESP instance at1ms, not timer0. Remove per-scan analogWrite
only in this profile: Core3.1.2 startWaveform waits with esp_yield, which must not
be called from SDK timer context. Apply PWM in foreground startup/setBrightness/
resume; keep cached brightness during pause and disarm on pause/destruction.
Keep original SPI and row/LAT/OE pinMode sequence. Background callback only
transfers front-buffer data and records counters; no network/Serial/heap work.
This changes PWM maintenance and scheduling, not gain registers or scan ratio.
OS software timer latency and visual quality require hardware tests; retained
baseline profile is the rollback. No timer0, Timer1 takeover or default change.

Closed/rejected after physical feedback: SDK timer did not reduce visible
shimmer; no stray pixels and phone connection/settings remained functional.
Software gaps still reached14.926ms during setDateTime and6.176ms during
getData in a100s capture. No restart observed in that interval is not soak
proof. Stop timing trials following the owner's time concern. Restore the
quiet foreground-scanning release, keeping splash/RTC and the reply-flush fix.
COM5 upload at115200 completed with flash hash verified;427424B BIN SHA256
`2748ADACB71162D759DBEFF7E344ADCB403D71DAA37A2F886746ED8EAB3B47D8`.

The SDK profile is retained solely as isolated rejected-experiment evidence,
not enabled in release builds. No default scheduler/PWM change is approved.
Visible shimmer is unresolved; electrical signal timing and long-run stability
have not been verified. Do not infer a specific hardware fault from this trial.

### 2026-10-12 — Inspect regional clearing before another display change

Owner proposes full-screen clear/repaint as the cause of shimmer on stationary
text alongside scrolling text. Source already isolates top clock/info zones and
the lower SmartText region. Unchanged static text is not redrawn. Bitmap clear
and drawing target fb; refresh reads displayfb, with double buffering requested
in setup and presentation after drawing. A real-source host check with stand-in
glyph pixels preserves top rows during scroll/change/empty/flash in both
directions across1-4 panels; buffer handoff does not reveal incomplete drawing.
Do not add redundant dirty-region infrastructure or flash a purported fix for
a behavior already present. Actual allocation failure can fall back to single
buffering and has not been checked on the connected board in this review.
Rendering work may still alter scan/PWM timing; this is distinct from pixel
clearing, and remains unproved. No production changes or additional hardware
experiment in this review.

### 2026-10-12 — Isolated packed-pixel scrolling probe

Owner approved a separate probe to reduce repeated glyph rasterization, not
another scanner/PWM change. `p10_4x1_pixel_scroll` enables
`SMARTTEXT_PIXEL_SCROLL=1` with DEBUG0, normal foreground scanning and no scan
logging. SmartText shifts inverted packed bytes only in its existing full-width
text area and rasterizes the entering column from the unchanged GLCD font.
No vendor library edits, extra framebuffer, storage change or API change.
Non-byte-aligned/sub-width/out-of-bounds areas fall back to the original path;
normal profiles omit the probe completely. Startup/static/flash rendering and
scroll cadence/direction/done semantics remain original.

Real-source host comparison first failed on repeated drawChar calls, then
passed103168 byte-identical frame/done samples for all eight Basit/Tahoma fonts,
1-4 panels and Arabic/English. Full-glyph calls in that fixture drop from4303315
to2304 (startup/static/flash); this is not a measured board speed improvement.
Other existing host checks pass. Physical image/API/shimmer feedback remains
the acceptance gate. Saved original427424B release BIN and SmartText files in
the task's external `p10-pixel-scroll-baseline-20261012` directory, SHA256
`2748ADACB71162D759DBEFF7E344ADCB403D71DAA37A2F886746ED8EAB3B47D8`.

Probe and normal release builds succeeded; normal release still reproduces
the saved original SHA256 above. Probe428320B SHA256
`B553867E78EC43AEB33EDA9B341D7A850348E04C961618DAE4815306B7D3D7D6`
uploaded at115200 on COM5 to MAC24:4c:ab:4f:ed:96 with flash hash verified.
RTS reset requested afterward. Owner image/shimmer/API feedback is pending;
no Android assets/version publication or settings writes. A read-only health
POST after upload timed out on the PC in3s; neither API success nor a firmware
API fault is proved by that unreachable request. Upload success is not proof
that shimmer is cured.

Owner subsequently reports very substantial shimmer reduction, intact letters,
and successful phone connection/settings changes. Accept packed-pixel scrolling
as the P10 default via guarded `SMARTTEXT_PIXEL_SCROLL=1` in constants. Explicit
override0 preserves the original renderer and the original saved BIN remains
available. Remove the redundant temporary build profile, keeping real-font
tests for default/explicit-on/explicit-off behavior. No additional hardware
upload, Android asset publication, version bump or Git publication is needed
for this default promotion. Physical evidence is limited to this4x1 board and
short owner tests; no claim that residual shimmer/resets/long-run faults are
eliminated or that electrical signal integrity was verified.

Default/on/off comparison, legacy region, prayer wording and startup checks
pass after promotion. Standard4x1 release builds successfully and its BIN SHA256
is byte-identical to the uploaded/owner-tested probe (`B553867E…` above), so no
additional hardware flash was performed.

### 2026-10-12 — Cache the active incoming-column glyph

Owner approved this further rendering optimization and will assess it on the
real display. Retain only one cached text span (including its spacing) and
glyph address/width/height/pages, rather than a whole glyph table or extra
framebuffer. Reuse it while incoming columns belong to that span; resolve from
text/font only on a miss. Font-pointer changes invalidate the lookup; reset
and text changes invalidate the cached text pointer. Existing direction and
shaping setters already reset. The rasterization, scan cadence and scroll
deadline/done behavior remain unchanged. No library/storage/API edits.

The adjacent-column regression first failed for repeated charWidth calls,
then passed.123648 real-font frame/done samples agree across default/on/off
paths, including text/font/direction/reset/shaping changes; all existing host
scripts pass.4x1 release build succeeds, adding24B static RAM and128B flash.
BIN428448B SHA256 `C9B25E2A0D357BB1C6BD127CA3CD9EF87400C9BCBA56F48DBD9B58861DD193C2`.
Saved prior owner-tested source/BIN externally for rollback. This is reduced
lookup work, not a measured board-speed gain or a guarantee of zero shimmer.
Physical feedback and long-run/reset validation remain pending.

Candidate uploaded on COM5 at115200 to the same ESP8266 MAC24:4c:ab:4f:ed:96,
with flash hash verified and RTS restart requested. No Android assets, firmware
version, Git publication or time/settings writes. Await owner confirmation of
remaining shimmer, intact text and successful phone changes on this candidate.

Owner reports further strong reduction, intact letters and working phone
settings; residual slight twitch is concentrated at scrolling/static text entry.

### 2026-10-19 — Isolated OE/PWM maintenance comparison; no production change

Owner suspects OE and connected P104 on COM5. Current P104-13/4x1 and valid
RTC/time were confirmed over HTTP before a complete4MiB backup and independent
flash digest check. Experiment only modifies a task-local copy of DMDESP and the
application, preserving original production library files and packaged images.

In one binary, A1/A2 retain analogWrite after every phase; B caches the applied
duty and leaves PWM running, reapplying on duty change or pause/start invalidation.
The original pinMode INPUT/OUTPUT blanking, SPI bytes, LAT and row order,16,384Hz
PWM,1ms scan gate and renderer remain. This isolates PWM setup maintenance; it
does not test removal of PWM or force OE high during handoff. On the pinned
Core3.1.2, pinMode does not cancel PWM, whereas digitalWrite does. Preserve this
distinction when evaluating any eventual fix or SDK upgrade.

The first timed attempt is superseded: owner was not watching. Repetition
advances only by host command after a human response. Real-scanner check fails
on original code (12 setups versus1), then passes on the copy with byte-for-byte
SPI/GPIO traces, brightness changes, clamp/endpoints and pause/resume. Scene
checks cover manual gating, midnight/cache restoration, millis rollover and
invalid-time abort; runner checks recovery on success/upload/capture failure.
Temporary build passes (RAM39440B,Flash431935B,BIN436080B); USB115200 upload hash
verified. BIN SHA256 d727e4e40c5604a525ec401227cf942f98157fbdf9365c6c8773d4791dfe4a7e.

Owner describes B as reduced except a large twitch at second5, but also submits
an approximately-unchanged B selection. Preserve both. After explicit return to
A2, owner reports a clear increase. This supports contribution from repeated PWM
setup, not a proven sole cause or final repair. B setup delta0 confirms the
intended variable changed. Stable software windows (unequal durations) have
peers0 and no measured API/send/store work. No scene reset was detected.

The second5 twitch occurs with the clock still visible, so five-second label
alternation is not established as its cause. Diagnostic SCAN printing every10s
and HEAP every60s can still interrupt foreground scanning; SCAN's omitted print
interval must not be mistaken for an electrical timing bound. Next observation
should be quiet, with counters in RAM and no periodic printing, before another
OE/LAT timing guess. Electrical OE/power timing and long-run stability are pending.

Exact original executable restored; executable and original filesystem/EEPROM
digests independently match. Saved Windows profile reconnected; original P104-13,
valid RTC/time and4x1 API confirmed. COM5 closed. No production code, settings,
Android assets or release versions changed. Detailed report and raw evidence:
`C:/Users/m_nag/.codex/visualizations/2026/10/17/01a14900-8f6f-7a02-ac4f-427e59861ad9/oe-pwm-manual-2026-10-19/report.md`.

### 2026-10-19 — Quiet A/B/A supersedes tentative PWM-caching recommendation

Owner approved continued investigation. A new isolated copy runs the SAME
per-phase analogWrite A versus cached-duty B, but disables periodic SCAN/HEAP
printing for both. It keeps counters in RAM and prints only after the human
observation at stage boundaries. Host telemetry check demonstrates14 old UART
writes/120 simulated seconds versus0 in the quiet version, with stage sampling
retained. Existing real-scanner/scene/recovery checks pass; quiet4x1 build passes
(RAM39376B,Flash431831B,BIN435984B) and USB115200 upload/hash verifies.

Owner reports shimmer in quiet A1 with no large second5 twitch, increased shimmer
in B, then recovery to the original level in A2. Therefore REJECT PWM reuse as a
production fix. The earlier logged trial and tentative recommendation do not
override this quiet reversal. Absence of the second5 twitch supports an observer
effect from diagnostic output, not a proven digit defect or timed electrical cause.

B setup counter remains165702 (delta0). Summary peer counts0; API/send/store
maxima0. Mean latch gaps are1000us throughout; maxima2808/2732/4492us over unequal
observation durations. A lower refresh software cost did not improve the picture.
Boundary UART work can contaminate render-stage maxima; software print exclusions
are not optical/electrical bounds. Do not infer uniform phase light or a sole
scheduling cause from the aggregate scan statistics.

Original executable restored with write hash verification; original executable
and filesystem/EEPROM digest checks pass. COM5 closed after normal reset. Only
documentation changes in the production repository. No driver fix, Android asset
replacement or release publication. Owner has only the computer and clock, no
scope/logic analyzer. Any further software probe must isolate its own variable;
do not revive the rejected SDK timer or guess another LAT/row timing change.
Evidence: sibling `oe-pwm-quiet-2026-10-19/report.md` beside the prior report.

### 2026-10-20 — Quiet frequency comparison supports8192Hz candidate

Owner approved16384/8192/16384Hz trial. Initial attempt did not reach B and failed
to reconnect for recovery. After owner USB reconnection, read-only identity
confirmed the same MAC24:4c:ab:4f:ed:96/ChipID0x004fed96. Original executable and
storage were then independently restored/verified and original API/RTC checked.

Resumption is a separate copied-source trial with a tested15-minute board-side
exit to normal display/default frequency if host sleeps. Stage transitions stay
human-gated. Production per-phase PWM maintenance, requested brightness9,
SPI/LAT/row/OE modes,1ms scan gate, rendering and presentation are retained;
only requested PWM frequency changes. Periodic UART remains disabled. Tests,
quiet4x1 build and USB115200 upload/hash pass. BIN436016B SHA256:
74303506ef810a1658c18430598525f4ab8e0886798a8f0fe03c717bb3bd64e1.

Owner reports usual shimmer at16384Hz, clear reduction at8192Hz, and return to
the original level at16384Hz. Perceived brightness is approximately unchanged.
B retains65709 PWM calls for65709 scans. All mean gaps1000us; maxima2596/2540/
2765us over unequal durations. Peers0, no instrumented API/send/store work, no
scene reset. Aggregate software timing does not identify electrical pulse shape,
phase interaction or interrupt load as the sole cause. Lower shimmer is observed,
not complete elimination or qualification at other duty/font/model/network states.

Recommended bounded next candidate is analogWriteFreq(8192) in application setup
immediately AFTER dmd.start(), which overrides the library's startup default via
the public Arduino API and avoids modifying DMDESP core. Retain per-phase
analogWrite. Candidate source change/ordinary-mode test and publication have NOT
been performed in this trial.

After comparison, exact original executable restored and executable/storage
digests independently match. COM5 closed; saved Windows profile reconnected and
original P104-13/4x1/config7 with valid RTC/time confirmed. No settings/RTC write,
Android assets, release-version change or publication. Detailed report/raw data:
sibling `oe-frequency-resume-2026-10-20/report.md` under the prior artifact root.
Cache is visually accepted. Trace first-frame work separately: shape/measure/
regional clearing still occurs, and initial scrolling repeats clearArea and
visits offscreen glyphs. This supports an initial-render-work hypothesis, not
a measured electrical cause. No additional firmware edits or upload for this
feedback review; initial-frame and long-run/reset validation remain pending.

### 2026-10-12 — Reduce first-frame work without changing rendered pixels

Owner approved packed regional clear and removal of redundant entry work.
Under the adopted pixel-rendering path, clear valid full-width byte-aligned
regions with0xFF packed bytes, retaining Bitmap clipping for partial/invalid
regions. updateText still shapes/measures and clears once; first scrolling draw
does not clear again. drawTextFast still measures each character for placement,
but skips drawChar's bitmap offset/pixel work for wholly hidden left glyphs
(including invalid/zero-width handling). Existing yield/scan cadence calls and
scroll deadlines remain; no library, RTC, storage, prayer or API edits.

Entry test first failed for pixel-rectangle clearing, then passed; fallback
first-frame clear count is one.124368 real-font frame/done samples agree across
default/on/off and geometric edge cases; all other existing host checks pass.
4x1 release builds with unchanged37436B RAM, +80B flash;428528B BIN SHA256
`F3D0A4677CC9AA48AC96FAD400B5C571B7435121B2F04D7A8B71EF074C5C9A83`.
Prior owner-tested source/BIN saved externally for rollback. Owner physical
feedback and long-run/reset verification remain pending; reduced work is not
a quantified hardware latency gain or a guarantee of zero entry twitch.

Uploaded428528B on COM5 at115200 to MAC24:4c:ab:4f:ed:96 with flash hash
verified and RTS reset requested. No time/settings, version, Android or Git
publication changes. Await owner feedback specifically at static/scroll entry.

Owner now confirms the entry twitch disappeared, letters are intact and the
phone app/settings remain functional. Accept the current display implementation
on this4x1 board (BIN SHA256 `F3D0A467…` above). Stop display timing experiments
here; no source/firmware/upload/publication change is needed for this feedback.
Long-run/intermittent-reset proof and physical validation of other panel sizes
remain pending; this visual acceptance is not a whole-firmware release claim.

### 2026-10-12 — Observe the exact owner-tested release for12 hours

User authorized long-run and intermittent-reset testing, connected the PC to
clock Wi-Fi and agreed to leave it/USB powered. Do not substitute DEBUG firmware:
retain release428528B SHA256 `F3D0A467…`. Added host collector only. Passive COM5
74880 with DTR/RTS false before open; it never writes serial commands, time or
settings. `/health` every30s and `/selfTest` every5min provide live-response,
uptime/heap/RTC evidence. Windows thread-local system-sleep inhibition is held
only during the run; the screen may sleep. Record connection/observation gaps
and possible millis rollover rather than treating them as successful coverage.
ROM boot markers and uptime drops may describe one reboot and must not be summed.
Port-open-window evidence is annotated; avoid declaring it spontaneous without
comparison. Baseline External System reason alone is not a new reboot event.

Five local checks pass including two complete simulated collector runs (normal
and ROM boot); build succeeds and firmware hash remains exact. Physical run
starts02:34:20.535294 UTC, ends14:34:20.535294 UTC (05:34–17:34 Cairo), actual
interpreter PID34304 (launcher23256). Logs:
`C:/Users/m_nag/.codex/visualizations/2026/10/10/01a12388-fa61-7732-b647-7a5067e1e950/p10-soak-20261012-022046Z/`.
Read status.json, events.jsonl, serial.raw and runner.err.txt; do not open COM5
again while the collector runs. Heartbeat automation `p10-12` follows this thread
every30min, stays quiet for unchanged healthy state, notifies meaningful new
evidence/failure and reports completion, then disables itself. First123s healthy
with5 successful health samples, no reset/drop/coverage error, heap minimum36392B
and max fragmentation4%; this is not a finished12-hour result. No power cycling,
OTA/write stress, other-size physical validation, assets/version/Git publication.

## 2026-10-15 — Preserve readable alerts on narrow panels

An alert must fit its measured shaped width before it can blink in place.
Longer alerts use SmartText's existing continuous pixel scroll, because
blinking while entering/exiting would hide glyphs. This applies to the actual
font width on every profile, including English alerts with the small font;
no glyph changes, automatic font shrinking or separate firmware project is
needed. Fitting alerts keep the existing blink cadence.

Paired large-font prayer messages now advance after a full scrolling pass,
or five visible seconds for fitting text, rather than an uptime-aligned
five-second boundary. A display revision restarts the pair at message one.
The prayer state machine still chooses the current phase on every loop and
can supersede either message immediately; the rendering engine never delays
an adhan/iqama phase boundary to finish an old message.

## 2026-10-16 — Bound and align the small-font top row

Keep owner font pixels intact. Determine the font's globally blank leading
rows once per font and offset only top-row drawing. Do not apply a blind
negative y offset to a font already beginning at bit0, or move the lower row.

Clock and info draw into a shared once-allocated8-row Bitmap. Copy only the
owning horizontal zone into the existing back buffer, so glyph foreground
and background pixels cannot cross into the adjacent zone. Right-align
actual ink rather than glyph advance plus a fixed4px margin. Use Bitmap.copy
between distinct buffers for non-byte-aligned padding; the vendored in-place
Bitmap.scroll/blit path has incorrect row translation and is not used here.
No library or scan-driver changes are needed.

Prefer readable compact text before clipping: keep clock hours/minutes,
drop period then seconds only as needed, and compact the fixed info formats
when their measured shaped width exceeds their zone. Lower-row SmartText
continues using its existing coordinates, artwork and scrolling.

## 2026-10-16 — Owner's final top-row margins and retained seconds

Use one blank outer pixel to the left of clock ink and to the right of info
ink. Copy the measured ink rectangle into its zone with that fixed margin;
remove the separate packed-copy/padding branches and byte-alignment rule.
Clock is left-aligned rather than centered.

Remove the adaptive period/seconds deletion cases. On3x1/4x1 the clock owns
the first64 pixels, so its full time fits; info owns the remainder. On1x1,
the existing HH:MM format is unchanged because full seconds cannot fit32px.
On3x1 only, compact info may omit the Gregorian year or Arabic weekday's
definite article to fit its32px zone. No new configuration or font edits.

## 2026-10-20 — Full-brightness probe does not resolve P104 shimmer

Quiet owner-paced saved9/full255/saved9 at fixed16384Hz retained per-phase PWM
and scan/render behavior in isolated copies. Full255 uses the pinned core's
GPIOHIGH endpoint; electrical OE/panel polarity was not instrumented. Owner
confirms independent5V supply and visibly brighter B, describes B shimmer as
approximately unchanged, then clear reduction returning to saved brightness.
Full brightness is not a fix; do not claim electrical OE/power qualification.
Scan blanking/frame updates persist without a generated PWM pulse train.

Full stage ended manually at58.65s before60s exit. Original executable/storage
restored and independently verified; normal API/RTC afterward. No production
source/library/settings/version/Android change. Evidence and limits: external
oe-full-brightness-2026-10-20/report.md under the artifact root in ROADMAP.
Continue the authorized ordinary8192Hz trial separately; earlier frequency
reversal supports a candidate, not a complete fix.

## 2026-10-20 — Ordinary P104 observation supports8192Hz

The isolated release-mode candidate adds only analogWriteFreq(8192) immediately
after dmd.start(), retaining production per-phase PWM and ordinary rendering.
Build, same-board identity, write hash, independent candidate/storage digests
and API/RTC checks passed. Owner reports shimmer "اختفت تقريبًا" in ordinary
operation. Retain this candidate on the test P104 for longer observation; no
further write is needed. Current version remains13, so use the recorded BIN
hash9100ced1... to distinguish it from the captured original. Production source
and Android assets remain unchanged. This result supports a frequency-dependent
contribution, not a measured OE/power diagnosis or market-wide qualification.
Evidence: external oe-frequency-normal-2026-10-20/report.md under ROADMAP's
artifact root; other settings/models, connected peers and soak remain pending.

### Small-font result limits the8192Hz improvement

Owner then requested small-font testing. Changed only fS1 to0 using the existing
setDisplay API, retaining8192Hz/brightness9/family/digits/messages. Readback
verified only fS changed. Owner reports original-severity shimmer in both rows.
Thus frequency reduction alone is insufficient for both modes; do not market it
as a general fix or assign the residual to a single row's renderer.

Restored fS1 through the saved minimal request and verified every original
getData field matched. Settings sequence may advance, so raw storage identity
is not asserted after these writes. Computer disconnected; same candidate stays
on P104 with original large font. No production source/library/Android changes.
Small-font frozen-image/presentation isolation and electrical evidence remain
outstanding; changing scan/buffer scheduling without that evidence is premature.

## 2026-10-20 — Quiet small-font frame hold reproduces update contribution

Owner authorized a fixed8192Hz/brightness9 small-font active/held/active trial.
Temporary RAM overrides and real pre-azan rendering only; unchanged vendor
libraries/per-phase PWM/scan gate. No periodic UART prints. Owner reports original
shimmer, near disappearance with held image, then original severity returning
after updates resume. HOLD-entry/ACTIVE2-entry CRC98fdfd90 matches. Software gap
maxima5406/1415/5417us and over5ms counts6/0/5 accompany that reversal. Summaries
have peers0 and API/send/store maxima0; no scene reset detected.

This strongly supports a frame-update contribution and scheduling/load involvement.
HOLD removes drawing, actual swaps and drawing-internal scan opportunities
together; it does not isolate CPU work from frame-boundary mixing. Boundary
prints/unequal durations limit timing maxima, and no light/OE waveform is measured.
The next discriminating experiment retains back-buffer rendering while suppressing
presentation; it was not performed by this trial. Do not jump directly to a
driver/timer rewrite or claim a completed all-mode shimmer fix.

Fresh current4MiB captured from the identified board before upload. After the
trial captured ordinary8192Hz first1MiB restored/hash verified; executable/storage
digests independently match. Normal API/RTC/time and all original getData fields
confirmed, including original large font/brightness9. COM5 closed and computer
Wi-Fi disconnected. No saved setting/RTC write, production source/library or
Android/version change. Evidence: external small-frame-hold-2026-10-20/report.md
under ROADMAP's artifact root, including tests, raw serial and recovery manifest.

## 2026-10-20 — Drawing retained with presentation held narrows shimmer cause

Owner approved the discriminating small-font trial: real renderer in all A/B/A
stages, only presentDisplayFrame suppressed in B.8192Hz/brightness9/family0/
Western digits unchanged; RAM font/duty override, no vendor edits or stored writes.
Instrumentation counts actual swaps by changed draw pointer, then retains the
old pointer as visible-buffer identity. Native actual scene/app-presentation/
production-swap checks cover60000 changing back-buffer renders with stable front
bytes/no swaps, resumption, equal-buffer early exit and recovery edge cases.

Owner reports usual shimmer in A1, unchanged B, unchanged A2. B recorded142836
render-path invocations (not full redraws),81 dirty marks, zero actual swaps,
matching visible CRC95352656 and changed drawing CRC. Scan-gap maxima5443/5665/
5956us, over5ms counts14/6/4; summaries peers0/API-send-store0, no detected reset.
The earlier no-render hold nearly removed shimmer with1415us maximum. Together
these strongly support drawing-work/cooperative-scan scheduling involvement;
removing presentation alone did not help. Do not label tearing the proven primary
cause. A particular drawing function's duration and the final remedy remain
unverified. Boundary UART/unequal stage lengths limit maxima; no light/OE measure.

Next bounded candidate is scan service during application drawing, particularly
small-font top-row pixel loops that currently lack service checkpoints. Avoid
removing SmartText's existing service calls or jumping to a timer/library rewrite.
This trial itself implements no production drawing fix. Captured ordinary8192
executable restored/hash verified, independent executable/storage digests match;
API/RTC/time and every pretrial getData field confirmed. COM5 closed, computer
Wi-Fi disconnected, large font restored. Production source/library/settings/
RTC/Android/version untouched. Evidence: external small-present-hold-2026-10-20/
report.md under ROADMAP's artifact root, with raw logs, tests and backup manifest.

## 2026-10-20 — Service the cooperative scanner during application drawing

Keep foreground scan and existing per-phase PWM. Add dmd.loop between characters,
64-byte chunks of cold font inspection,8-column ink-inspection chunks and single
top-row clear/copy rows. Bitmap.copy remains between distinct buffers with correct
source/destination row offsets. Preserve every font/position/margin/colon pixel;
no added yield, ISR, buffer-swap policy, allocation or vendor edit. Set8192Hz
after dmd.start using the public application call, retaining the validated
large-font improvement. Shared drawDisplayText callers, including OTA/splash,
were inspected; the driver's paused guard remains applicable.

RED original native check fails552/576 cases per profile without bounded scan
opportunities (largest counted access run8876). GREEN actual Bitmap/all8 fonts/
all4 widths:2304 pixel-identical cases, largest counted run409. Counted native
accesses do not establish microsecond deadlines. Quiet owner-paced service-off/
on/off P104 trial yields usual shimmer / almost disappeared / usual shimmer.
Gap maxima5517/1876/5758us, over5ms7/0/11; B checkpoint calls2304, A calls constant.
All summary peers0/API-send-store0; no scene reset. Unequal durations/boundary
prints limit maxima; no light/power waveform claim.

After verified baseline recovery, adopted the exact checked ordinary source.
Four P10 release builds pass. Ordinary P104431024B/SHA25677851678... byte-matches
production output; same-board write/independent executable and storage digests
verify. Normal API/RTC/time/settings confirmed. Owner also reports near
disappearance in ordinary small-font operation. Original fS1 restored and every
pretrial getData field plus valid RTC/time verified. Font saves may advance
storage sequence; no raw storage identity claim afterward. COM5 closed,
computer Wi-Fi disconnected. Fixed ordinary firmware remains on P104.
No Android assets/version/layout change or other-model physical validation.
Evidence: external small-draw-service-2026-10-20/report.md under ROADMAP's root.

## 2026-10-20 — P10 v14 release and M-71 deferral

The four P10 variants share the reviewed drawing/scanner changes and now
advertise version 14: P101-14, P102-14, P103-14 and P104-14. The EEPROM schema
stays at version 7, so this version bump does not reset configuration. Release
builds passed for all four profiles; the corresponding BINs were copied to the
Android assets directory and verified against the build outputs by size and
SHA-256. The owner rebuilt the Android APK and confirmed the P10 long-run and
OTA tests successful.

M-71 is intentionally deferred. Its legacy version-8 firmware and asynchronous
OTA response path require a separate compatibility task; no P10 or M73 image is
valid for M-71. Seven-segment publication remains a separate release line.
