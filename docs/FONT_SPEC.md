# FONT_SPEC — Extended Arabic Bitmap Font & Shaper Specification

Single source of truth for the Arabic-support project. If code and this doc
disagree, fix one of them immediately.

---

## 1. Host font format (GLCD variable-width, as consumed by lib/DMDESP)

Verified against `lib/DMDESP/Bitmap.cpp:509-618`:

```
offset  size  meaning
0       2     size (uint16 LE) — MUST NOT be 0x0000 (both-zero means fixed-width!)
2       1     fontWidth  (nominal width; informational for variable fonts)
3       1     fontHeight (pixel height, e.g. 7 or 16)
4       1     firstChar  (we use 0x20)
5       1     charCount  (we use 0xE0 = 224 → covers codes 0x20..0xFF)
6       N     widths[count]      (one byte per char, variable-width mode)
6+N     ...   glyph bitmaps, sequential in char order:
              per glyph: ceil(height/8) * width bytes,
              laid out ROW-major over vertical 8px chunks:
                byte(chunk c, column x) at offset c*width + x
              bit b (LSB first) = pixel row (c*8 + b)
```

Rendering notes:
- Space (0x20) is special-cased: filled blank rect of its widths-entry size.
- Out-of-range codes render nothing and measure 0 width.
- Glyphs carry their own side-bearing inside their bitmap (existing Janat
  artwork already includes ~1px gaps).

## 2. Extended layout

| Range      | Contents |
|------------|----------|
| 0x20–0x7E  | Standard ASCII: digits, Latin, punctuation (unchanged behavior) |
| 0x7F       | blank |
| 0x80–0xFE  | Arabic presentation-form glyphs (127 slots, see §3) |
| 0xFF       | reserved spare |

## 3. Arabic glyph inventory (exactly 127 slots)

Form order per letter: **ISO, FIN** (right-joining letters) or
**ISO, INI, MED, FIN** (dual-joining letters). Letters listed in Unicode order.
Slots assigned sequentially from 0x80 in exactly this order.

### 3.1 Non-joining (1 glyph)
| Letter | Class | Forms | Count |
|--------|-------|-------|-------|
| ء U+0621 hamza            | NONE | ISO | 1 |

### 3.2 Right-joining: ISO + FINAL (11 letters × 2 = 22)
| Letter | Codepoints |
|--------|-----------|
| آ U+0622 alef madda | FE81 ISO, FE82 FIN |
| أ U+0623 alef hamza-above | FE83 ISO, FE84 FIN |
| إ U+0625 alef hamza-below | FE87 ISO, FE88 FIN |
| ا U+0627 alef | FE8D ISO, FE8E FIN |
| د U+062F dal | FEA9 ISO, FEAA FIN |
| ذ U+0630 thal | FEAB ISO, FEAC FIN |
| ر U+0631 reh | FEAD ISO, FEAE FIN |
| ز U+0632 zain | FEAF ISO, FEB0 FIN |
| و U+0648 waw | FEED ISO, FEEE FIN |
| ى U+0649 alef maksura | FEEF ISO, FEF0 FIN |
| ؤ U+0624 waw hamza | FE85 ISO, FE86 FIN |

### 3.3 Dual-joining: ISO + INI + MED + FIN (24 letters × 4 = 96)
| Letter | Base CP | Forms (ISO,INI,MED,FIN) start at |
|--------|---------|----------------------------------|
| ب U+0628 beh | FE8F, FE91, FE92, FE90 |
| ة U+0629 teh marbuta | FE93, reserved, reserved, FE94 *(RIGHT-joining; four storage slots retained for compatibility)* |
| ت U+062A teh | FE95, FE97, FE98, FE96 |
| ث U+062B theh | FE99, FE9B, FE9C, FE9A |
| ج U+062C jeem | FE9D, FE9F, FEA0, FE9E |
| ح U+062D hah | FEA1, FEA3, FEA4, FEA2 |
| خ U+062E khah | FEA5, FEA7, FEA8, FEA6 |
| س U+0633 seen | FEB1, FEB3, FEB4, FEB2 |
| ش U+0634 sheen | FEB5, FEB7, FEB8, FEB6 |
| ص U+0635 sad | FEB9, FEBB, FEBC, FEBA |
| ض U+0636 dad | FEBD, FEBF, FEC0, FEBE |
| ط U+0637 tah | FEC1, FEC3, FEC4, FEC2 |
| ظ U+0638 zah | FEC5, FEC7, FEC8, FEC6 |
| ع U+0639 ain | FEC9, FECB, FECC, FECA |
| غ U+063A ghain | FECD, FECF, FED0, FECE |
| ف U+0641 feh | FED1, FED3, FED4, FED2 |
| ق U+0642 qaf | FED5, FED7, FED8, FED6 |
| ك U+0643 kaf | FED9, FEDB, FEDC, FEDA |
| ل U+0644 lam | FEDD, FEDF, FEE0, FEDE |
| م U+0645 meem | FEE1, FEE3, FEE4, FEE2 |
| ن U+0646 noon | FEE5, FEE7, FEE8, FEE6 |
| ه U+0647 heh | FEE9, FEEB, FEEC, FEEA |
| ي U+064A yeh | FEF1, FEF3, FEF4, FEF2 |
| ئ U+0626 yeh hamza | FE89, FE8B, FE8C, FE8A |

> NOTE on teh marbuta: Unicode has no initial/medial presentation forms for it
> (it only connects to the preceding letter). Its existing four storage slots
> remain fixed: ISO=155, reserved=156/157, FIN=158. Reserved artwork is preserved
> but never selected. FE94 maps to158, including pre-shaped input; no bitmap
> regeneration or renumbering is needed.

### 3.4 Lam-alef ligatures (8)
| Sequence | ISO | FIN |
|----------|-----|-----|
| لا (lam + alef)        | FEFB | FEFC |
| لأ (lam + alef-hamza-above) | FEF7 | FEF8 |
| لإ (lam + alef-hamza-below) | FEF9 | FEFA |
| لآ (lam + alef-madda)  | FEF5 | FEF6 |

**Slot math:** 1 + 22 + 96 + 8 = **127** → fills 0x80–0xFE exactly. No room for
more unless we drop rarely-used forms (documented future options: drop
hamza-standalone ISO, drop teh-marbuta INI/MED duplicates → frees 3 slots).

## 4. Joining classes (for shaper)

Each letter has two boolean traits:
- `joins_right`: can connect to the previous letter (i.e., has a FIN/MED form)
- `joins_left`: allows the next letter to connect (has an INI/MED form)

| Class | joins_right | joins_left | Letters |
|-------|-------------|------------|---------|
| DUAL  | 1 | 1 | §3.3 list except ة |
| RIGHT | 1 | 0 | §3.2 list plus ة |
| NONE  | 0 | 0 | ء |

Transparent (skipped entirely, do not affect joining): harakat
U+064B–U+065F, U+0670; tatweel U+0640 (v1 drops it — see DECISIONS D5).

## 5. Shaper algorithm (`ArabicShaper.shape(utf8In, out, outSize)`)

```
1. cps[] = decode_utf8(utf8In)                    # reject malformed → '?'
2. logical pass (left→right):
     prev_link = false                            # previous emitted letter joins_left?
     i = 0
     while i < len(cps):
       cp = cps[i]
       if transparent(cp): skip                   # drop harakat/tatweel
       if cp == lam and next non-transparent cp is an alef variant:
           emit LAM_ALEF ligature form:
             FIN if prev_link else ISO
           prev_link = false                      # ligature never joins left
           i += 2; continue
       if cp is arabic letter:
           link_prev = prev_link and joins_right(cp)
           nxt   = next non-transparent cp
           link_next = nxt is arabic-letter and joins_left(cp) and joins_right(nxt)
           emit form(cp): MED if link_prev&&link_next
                          else FIN if link_prev
                          else INI if link_next
                          else ISO
           prev_link = joins_left(cp)
       else:
           emit ascii_byte(cp) unchanged          # digits/punct/Latin/space
           prev_link = false
       i += 1
3. visual reorder pass (whole line assumed RTL context when language==ar):
     walk logical list right→left;
       - shaped arabic glyph (>=0x80): append as-is
       - ASCII run [j..i]: copy j→i in forward order (keeps numbers/times intact)
4. NUL-terminate; guarantee len(out) < outSize.
```

Properties:
- Output length ≤ input length (UTF-8 Arabic = 2 B/char in, 1 B/glyph out;
  ligatures shrink further; harakat dropped).
- Pure-ASCII input passes through untouched (English mode needs no shaper).
- Mirrored punctuation (parentheses) is OUT of scope v1.

## 6. Generated artifacts & ownership

`tools/gen_font.py` owns the canonical tables (inventory §3, classes §4).
It emits:
- `lib/DMDESP/fonts/<Name>.h` — extended font (either extracted from legacy
  Janat artwork or rendered from TTF)
- `src/arabic_glyph_map.h` — PROGMEM lookup: presentation-form CP (uint16) →
  slot byte, sorted ascending for binary search; plus joining-class table
- `docs/FONT_SPEC.md` §3 table can be regenerated via `--emit-spec` (keep doc
  and code in sync!)

Legacy-slot bootstrap: existing Janat ASCII slots correspond to specific
forms (e.g., from `"_wWA"`="الله": A→alef ISO, W→lam MED?, w→lam FIN?,
_→heh FIN?). Phase 1 builds a draft mapping from such sample strings, renders
previews, and the OWNER visually confirms/corrects before anything ships.

## 7. Firmware integration contract

- `SmartText` stores SHAPED bytes internally; `textWidth()` operates on shaped
  buffer (plain byte sums — no changes needed in DMDESP).
- English mode (`config.language==1`): skip shaper, use ASCII directly.
- Scrolling direction is independent of glyph order: English enters from the
  right and moves left; Arabic keeps the existing shaped rendering path.
- Buffer sizing: shaped buffer ≥ `strlen(utf8 input)`; keep BUF_TEXT aligned
  with config.message size.

## 8. Selectable font families

Current Tahoma artwork: large uses `TahomaBold16`/Indic; small uses the
optically hinted `Tahoma7`/Indic. Editable sources live in
`GLCDFontCreator2/Fonts`, with matching generated exports in `Exports`.
Small Arabic joins share row4; detached marks and open counters are verified
by `test/tahoma_small_font_test.py`. ASCII/digits remain owner-approved.
The editor's single-byte export now packs row0 into bit0 (including7px);
multi-page packing is unchanged. `test/fontcreator_roundtrip_test.py` checks
editable source -> real export -> runtime parity; reload the editor after a
tool-class rebuild. Optical fit backups/scripts are recorded in ROADMAP.

- Current firmware contains two font families (`fontFamily` = 0..1):
  BasitFont/Janat and Tahoma. Arial Bold was removed to keep the image and
  selector small; old EEPROM value 2 is migrated to BasitFont on boot.
- Each family provides a 7px and a 16px GLCD header with the exact same
  `0x20..0xFF` slot contract. The existing small/large setting remains
  independent from the family choice.
- All font arrays stay in `PROGMEM`; selecting a family changes only the font
  pointer and does not copy glyph data into RAM.
- Basit's ASCII letters/punctuation use the same Tahoma-derived 7px/16px
  artwork as the Tahoma family; Arabic slots (`0x80..0xFF`) remain
  Janat-derived. Digit slots are generated separately and are never replaced
  by a generic Latin overlay.
- Adjacent ASCII letter pairs and adjacent digit pairs receive one runtime
  pixel of spacing, independent of language or scroll direction. The shared
  `TextSpacing.h` rule is used by drawing, width measurement and scroll columns.
  Arabic joining, letter/digit boundaries, punctuation and explicit spaces
  receive no extra gap. Indic fonts reuse the same ASCII digit slots.
- API: `/setFont?font=0..1&lang=0..1&digits=0..1`; font, language, and digit
  choices are stored in EEPROM and applied without reboot. Arabic remains the
  default; English is available for the translated fixed strings and cards.
  family 0 after reboot. Final firmware moves the validated field into the
  coordinated settings/app contract.
- `/getData` appends both `language` and `fontFamily` to the response contract.
- Adding `fontFamily` to `Configuration` requires a coordinated EEPROM version
  bump and matching Android release; do not upload that firmware while the old
  app contract is still in use.
- v1 families are fixed at build time. Runtime TTF parsing or LittleFS font
  uploads are intentionally out of scope.
