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
