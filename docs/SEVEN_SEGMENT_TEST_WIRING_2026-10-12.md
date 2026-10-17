# Seven-segment test wiring — 2026-10-12

Source: the owner's three PDFs in `C:\Users\m_nag\Desktop\diagram`:

- `Schematic_M-TA6932-1inch-2layer-Date_2026-10-11.pdf`: page 1.
- `Schematic_M-TA6932-1inch-2layer-Prayers_2026-10-11.pdf`: pages 1–3.
- `Schematic_M-TA6932-1inch-2layer-Eqama_2026-10-11.pdf`: page 1.

All pages were inspected as rendered schematics, not just extracted text.
STB1/2/3 match the firmware's GPIO13/2/16. Driver caches are zero-indexed:
GRID1 is displayCache[0]. No vendor library or normal `show()` mapping changed.

## Dash traversal

One G segment on one digit across the entire clock, 500ms per digit.
Start with date, then time, prayer rows top-to-bottom, then the Eqama board.
Within each group, follow the left-to-right positions drawn in the schematic.

| Group | Schematic nets, left-to-right | Firmware chip and cache indices |
|---|---|---|
| Date YYYY/MM/DD | Date S14 S13 S12 S11 S10 S9 S8 S7 | tm1: 13 12 11 10 9 8 7 6 |
| Time HH/MM/SS | Date S1 S2 S3 S4 S5 S6 | tm1: 0 1 2 3 4 5 |
| Fajr | Prayers S3 S2 S1 | tm2: 13 14 15 |
| Sunrise | Prayers S4 S5 S6 | tm2: 9 10 11 |
| Dhuhr | Prayers S10 S9 S8 S7 | tm2: 5 6 7 8 |
| Asr | Prayers S11 S12 S13 | tm2: 3 2 4 |
| Maghrib | Prayers S16 S15 S14 | tm3: 2 1 0 |
| Isha | Prayers S17 S18 S19 S20 | tm3: 3 4 5 6 |
| Eqama/temperature board | S24 S23 S22 S21 | tm3: 10 9 8 7 |

All three Asr nets connect to U2 in the supplied schematic; none connects to U3.
The regular firmware assigns iqama to tm3[9,10] and temperature to tm3[7,8].
Its digit values/order are untouched: this task corrects test traversal only.

## Segment and number phases

After the 19-second dash walk: blank 1s, A–G on all 38 connected digits
for 1s each, blank 1s, digits 0–9 for 1s each. Total is 38 seconds.
Unconnected GRID outputs stay blank in every phase.

The Date/Prayers boards offer JA/JH jumpers routing physical segment A from
SEG1 or SEG8. Eqama shares the U3 segment bus. All displayed DP pins are NC.
Test frames therefore mirror logical A (bit 0) onto bit 7, leaving B–G unchanged:
top-segment phase is 0x81, not 0x01 alone. This also removes the inconsistent
old high bit from test numbers 1/4 and adds it for 5/6. No decimal-point phase
is introduced; the normal digits table is not changed.

## Verification and limits

Startup now keeps the existing `01030007330` frame on tm1 unchanged and adds
model 73 on tm3[7,8], firmware release on tm3[9,10], in the real front-panel
order reported by the owner (the initial schematic-derived order showed 37/51).
This matches normal temperature/iqama rendering; the previously accepted dash
walk is unchanged. Both derive from FIRMWARE_VERSION (M73-15 / M73s-14).
These four new digits normalize the A bit for JA/JH, as the test does.
Startup animation is now nonblocking: seven A–G frames (150ms each) followed by
nine single-group dash frames (100ms each), date first. It uses the same shared
38-grid table as the accepted manual test. The animation lasts 1950ms nominally;
late loop iterations keep each frame visible for its full interval rather than skip.
The six-second contact/identity hold starts when the actual frame is displayed.
Setup initializes RTC/AP/server first; loop services startup alongside networking.
RTC is read at most once per second during startup. Config/RTC/time faults hand
off immediately to normal show() error rendering. OTA and restart suppress all
startup writes. The unused GRID16 RTC boot marker and old absolute deadline gate
were removed; the manual 38-second solder test is unchanged.
The two-digit release layout has a compile-time format guard: revise it before
release 100. The regular temperature/iqama display mapping remains untouched.

`test/seven_segment_display_test.py` executes production `testMode()` and its
HTTP entry using a controllable clock. Hand-checked expected buffers cover
schematic order, connected grids only, both A routes, all digits, 500ms/1s
holds, blank transitions, millis rollover and test restart. Firmware fault
checks also passed 27/27.

Schematics define connections and the drawn symbol order, not proof of actual
front-panel assembly orientation or solder quality. Owner observation remains
required. No M71 image, Android publication or normal display/config change.
