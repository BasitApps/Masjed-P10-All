# Configurable Card Scheduler Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add persistent card enable/order settings and a safe scheduler while preserving the legacy firmware/API behavior for deployed devices.

**Architecture:** Keep `Configuration` and `EEPROM_VERSION` unchanged. Store a CRC-protected `CardPrefs` sidecar in unused EEPROM space, validate it on load, and make the existing `nextCard()` consume a bounded validated order. Expose two additive legacy GET endpoints and add one Android settings dialog using the existing HTTP/blur helper.

**Tech Stack:** ESP8266 Arduino C++, EEPROM, existing `ESP8266WebServer`, Java Android UI, existing OkHttp/request helper, native PlatformIO tests.

**Spec:** `docs/superpowers/specs/2026-09-28-card-scheduler-design.md`

## Global Constraints

- Keep `/updateMain`, `/getData`, and all existing endpoint field positions unchanged.
- Keep `EEPROM_VERSION` unchanged and fall back to all-enabled factory order when sidecar data is invalid.
- Use the existing blur transport; add no JSON, HMAC, or dependency.
- Support P10 profiles 1x1, 2x1, 3x1, and 4x1.
- Do not modify `lib/*` except the already-approved DMDESP timer flag fix.

## Review Focus

- Corrupt/partial sidecar EEPROM: defaults load and the clock still starts.
- Duplicate/out-of-range order bytes: request is rejected atomically.
- All cards disabled: scheduler selects a safe fallback and never loops forever.
- Small display with large-only cards: cards are skipped without blanking the display.
- Old app/device: existing endpoints and old card behavior remain unchanged.

---

### Task 1: Firmware sidecar, scheduler, and API

**Files:**
- Modify: `src/globals.h`
- Modify: `src/Masjed-P104.cpp`
- Modify: `src/constants.h` only if a sidecar offset constant is required
- Test: `test/test_card_scheduler/test_main.cpp`

**Interfaces:**
- Produces `loadCardPrefs()`, `saveCardPrefs()`, `validCardOrder()`, and bounded scheduler behavior used by the existing card loop.
- Adds GET `/getCardSettings` and `/setCardSettings` without changing old handlers.

- [ ] Add pure order/mask validation tests first: valid permutation, duplicate, out-of-range, disabled entries, and all-disabled fallback.
- [ ] Add the `CardPrefs` sidecar with magic, enabled mask, order, and CRC; load defaults on invalid data and avoid EEPROM writes when unchanged.
- [ ] Update `nextCard()`/`handleCards()` to scan the validated order with a hard `CARD_COUNT` bound and preserve prayer-window priority.
- [ ] Add fixed card duration/layout policy without changing existing scroll-completion behavior.
- [ ] Implement and validate `/getCardSettings` and `/setCardSettings` using existing blur parsing and atomic persistence.
- [ ] Run `pio test -e native` (or record the existing missing-gcc limitation) and `pio run` for all four Release profiles.

### Task 2: Android card settings dialog and request wiring

**Files:**
- Modify: `C:/Users/m_nag/Desktop/MasjedClockWiFi/app/src/main/java/com/basitapps/masjedclockwifi/MainActivityP10.java`
- Modify: `C:/Users/m_nag/Desktop/MasjedClockWiFi/app/src/main/res/layout/activity_main_p10.xml`
- Modify: `C:/Users/m_nag/Desktop/MasjedClockWiFi/app/src/main/res/values/strings.xml` only for card labels/actions
- Test: Android Studio project compile

**Interfaces:**
- Consumes `/getCardSettings` and sends one blurred `/setCardSettings` request.
- Leaves other display models and existing P10 settings untouched.

- [ ] Add a draft model for 12 cards with enabled state and order.
- [ ] Add the dialog with checkbox, move-up/down, cancel, save, and restore-factory actions using existing styling.
- [ ] Load settings when opening the dialog; keep device state unchanged on cancel or request failure.
- [ ] Send one validated mask/order request on save and show the existing success/error feedback.
- [ ] Compile the Android project and verify the legacy P10 screens still build.

### Task 3: Integration verification and continuity docs

**Files:**
- Modify: `docs/ROADMAP.md`
- Modify: `docs/DECISIONS.md`
- Modify: `docs/RELIABILITY_PROGRESS.md`

- [ ] Document EEPROM sidecar layout and API compatibility.
- [ ] Record firmware/app build results and any environment-blocked tests.
- [ ] Perform the hardware checks: default order, custom order, disabled card, factory restore, and reboot persistence.
