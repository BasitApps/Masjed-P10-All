# DECISIONS — Decision Log

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
