# Configurable Card Scheduler Design

**Date:** 2026-09-28  
**Status:** Approved in chat; awaiting written-spec review

## Goal

Allow the owner/customer to enable, disable, and reorder the P10 information
cards without changing the existing customer workflow or breaking deployed
firmware configurations.

## Constraints

- Keep the existing card order and all cards enabled by default.
- Keep prayer windows higher priority than normal cards.
- Keep legacy GET endpoints and `blur()` transport; no JSON, HMAC, or new
  dependency.
- Do not bump `EEPROM_VERSION` for this feature.
- Old firmware and old Android builds must continue to work unchanged.
- Keep the 1x1, 2x1, 3x1, and 4x1 profiles supported.

## Firmware design

`CardPrefs` is stored in a dedicated EEPROM sidecar area after the existing
configuration/CRC region. It contains a magic value, a 12-bit enabled mask, a
12-byte permutation/order array, and a CRC. Invalid or missing sidecar data
falls back to the factory order and all-enabled mask; the main `Configuration`
layout and version remain unchanged.

The scheduler uses the persisted order and skips disabled cards. It validates
the permutation, bounds every scan to `CARD_COUNT`, and falls back to the
clock/message defaults if no selectable card remains. Existing card functions
continue to provide their own content availability checks, but `nextCard()` no
longer relies on unbounded recursion. Prayer windows continue to set
`stopCards` and bypass the scheduler.

Each card receives a small fixed display policy (default duration and whether
it is valid for small/large layout). Scrolling cards still advance when their
scroll completes; static cards use their policy duration. Empty messages and
unsupported layout cards are skipped without drawing a blank frame.

## API design

Add legacy-compatible GET endpoints:

- `/getCardSettings` returns a compact blurred response containing the enabled
  mask and 12-byte order.
- `/setCardSettings?mask=<blurred integer>&ord=<blurred 12-byte order>` validates
  the mask, exact order permutation, and card range before atomically saving
  the sidecar and applying the scheduler.

Existing endpoints, `/getData` field positions, and OTA behavior are unchanged.

## Android design

Add a P10 card-management dialog in the existing settings screen. It displays
the 12 card names, a checkbox for visibility, and move-up/move-down controls.
Save sends one `/setCardSettings` request using the existing request helper;
cancel leaves the device unchanged. A “restore factory order” action resets
the local draft to all enabled/default order before saving. If the endpoint is
unavailable, the screen reports the failure and does not alter local state.

## Verification

- Native tests cover order permutation validation, disabled-card skipping,
  all-disabled fallback, and sidecar default recovery.
- `pio run` succeeds for all four Release profiles.
- The Android project compiles after the dialog and request wiring.
- Hardware check confirms default behavior is unchanged, then verifies a
  custom order, disabled card, restore-factory action, and reboot persistence.
