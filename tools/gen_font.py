#!/usr/bin/env python3
"""gen_font.py - Arabic extended bitmap font generator for Masjed-P104-Esp-v2.

Implements docs/FONT_SPEC.md. Canonical glyph inventory lives HERE; the tool
emits GLCD variable-width font headers consumable by lib/DMDESP plus a PROGMEM
mapping header (src/arabic_glyph_map.h) for the runtime shaper.

Modes:
  emit-spec   Print FONT_SPEC section-3 markdown table (keep doc in sync)
  ttf         Render every glyph from an Arabic-capable TTF
  extract     Reuse legacy Janat ASCII-slot bitmaps via a mapping JSON
              (+ optional TTF fallback for whatever is missing)
  preview     Shape UTF-8 text (reference shaper) and render PNG line(s)
  sheet       Contact-sheet of every slot in any GLCD header (labeled)

Examples:
  python tools/gen_font.py ttf --font C:/Windows/Fonts/arial.ttf --height 16 ^
      --out-font lib/DMDESP/fonts/Tahoma16.h --out-map src/arabic_glyph_map.h
  python tools/gen_font.py preview --font lib/DMDESP/fonts/Tahoma16.h ^
      --scale 8 --out tools/out/preview.png --text "الصلاة على النبي"
"""

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# ---------------------------------------------------------------------------
# Canonical inventory (FONT_SPEC section 3) -- SINGLE SOURCE OF TRUTH
# ---------------------------------------------------------------------------
# (letter_name, base_cp, join_class, [(form_tag, presentation_cp), ...])
# join_class: DUAL, RIGHT, NONE. Form order: ISO,FIN or ISO,INI,MED,FIN.
LETTERS = [
    ("hamza",          0x0621, "NONE",  [("ISO", 0xFE80)]),
    ("alef_madda",     0x0622, "RIGHT", [("ISO", 0xFE81), ("FIN", 0xFE82)]),
    ("alef_hamza",     0x0623, "RIGHT", [("ISO", 0xFE83), ("FIN", 0xFE84)]),
    ("alef_hamza_blw", 0x0625, "RIGHT", [("ISO", 0xFE87), ("FIN", 0xFE88)]),
    ("alef",           0x0627, "RIGHT", [("ISO", 0xFE8D), ("FIN", 0xFE8E)]),
    ("dal",            0x062F, "RIGHT", [("ISO", 0xFEA9), ("FIN", 0xFEAA)]),
    ("thal",           0x0630, "RIGHT", [("ISO", 0xFEAB), ("FIN", 0xFEAC)]),
    ("reh",            0x0631, "RIGHT", [("ISO", 0xFEAD), ("FIN", 0xFEAE)]),
    ("zain",           0x0632, "RIGHT", [("ISO", 0xFEAF), ("FIN", 0xFEB0)]),
    ("waw",            0x0648, "RIGHT", [("ISO", 0xFEED), ("FIN", 0xFEEE)]),
    ("alef_maksura",   0x0649, "RIGHT", [("ISO", 0xFEEF), ("FIN", 0xFEF0)]),
    ("waw_hamza",      0x0624, "RIGHT", [("ISO", 0xFE85), ("FIN", 0xFE86)]),
    ("beh",            0x0628, "DUAL",  [("ISO", 0xFE8F), ("INI", 0xFE91),
                                        ("MED", 0xFE92), ("FIN", 0xFE90)]),
    ("teh_marbuta",    0x0629, "DUAL",  [("ISO", 0xFE93), ("INI", 0xFE94),
                                         ("MED", 0xFE94), ("FIN", 0xFE94)]),
    ("teh",            0x062A, "DUAL",  [("ISO", 0xFE95), ("INI", 0xFE97),
                                        ("MED", 0xFE98), ("FIN", 0xFE96)]),
    ("theh",           0x062B, "DUAL",  [("ISO", 0xFE99), ("INI", 0xFE9B),
                                        ("MED", 0xFE9C), ("FIN", 0xFE9A)]),
    ("jeem",           0x062C, "DUAL",  [("ISO", 0xFE9D), ("INI", 0xFE9F),
                                        ("MED", 0xFEA0), ("FIN", 0xFE9E)]),
    ("hah",            0x062D, "DUAL",  [("ISO", 0xFEA1), ("INI", 0xFEA3),
                                        ("MED", 0xFEA4), ("FIN", 0xFEA2)]),
    ("khah",           0x062E, "DUAL",  [("ISO", 0xFEA5), ("INI", 0xFEA7),
                                        ("MED", 0xFEA8), ("FIN", 0xFEA6)]),
    ("seen",           0x0633, "DUAL",  [("ISO", 0xFEB1), ("INI", 0xFEB3),
                                        ("MED", 0xFEB4), ("FIN", 0xFEB2)]),
    ("sheen",          0x0634, "DUAL",  [("ISO", 0xFEB5), ("INI", 0xFEB7),
                                        ("MED", 0xFEB8), ("FIN", 0xFEB6)]),
    ("sad",            0x0635, "DUAL",  [("ISO", 0xFEB9), ("INI", 0xFEBB),
                                        ("MED", 0xFEBC), ("FIN", 0xFEBA)]),
    ("dad",            0x0636, "DUAL",  [("ISO", 0xFEBD), ("INI", 0xFEBF),
                                        ("MED", 0xFEC0), ("FIN", 0xFEBE)]),
    ("tah",            0x0637, "DUAL",  [("ISO", 0xFEC1), ("INI", 0xFEC3),
                                        ("MED", 0xFEC4), ("FIN", 0xFEC2)]),
    ("zah",            0x0638, "DUAL",  [("ISO", 0xFEC5), ("INI", 0xFEC7),
                                        ("MED", 0xFEC8), ("FIN", 0xFEC6)]),
    ("ain",            0x0639, "DUAL",  [("ISO", 0xFEC9), ("INI", 0xFECB),
                                        ("MED", 0xFECC), ("FIN", 0xFECA)]),
    ("ghain",          0x063A, "DUAL",  [("ISO", 0xFECD), ("INI", 0xFECF),
                                        ("MED", 0xFED0), ("FIN", 0xFECE)]),
    ("feh",            0x0641, "DUAL",  [("ISO", 0xFED1), ("INI", 0xFED3),
                                        ("MED", 0xFED4), ("FIN", 0xFED2)]),
    ("qaf",            0x0642, "DUAL",  [("ISO", 0xFED5), ("INI", 0xFED7),
                                        ("MED", 0xFED8), ("FIN", 0xFED6)]),
    ("kaf",            0x0643, "DUAL",  [("ISO", 0xFED9), ("INI", 0xFEDB),
                                        ("MED", 0xFEDC), ("FIN", 0xFEDA)]),
    ("lam",            0x0644, "DUAL",  [("ISO", 0xFEDD), ("INI", 0xFEDF),
                                        ("MED", 0xFEE0), ("FIN", 0xFEDE)]),
    ("meem",           0x0645, "DUAL",  [("ISO", 0xFEE1), ("INI", 0xFEE3),
                                        ("MED", 0xFEE4), ("FIN", 0xFEE2)]),
    ("noon",           0x0646, "DUAL",  [("ISO", 0xFEE5), ("INI", 0xFEE7),
                                        ("MED", 0xFEE8), ("FIN", 0xFEE6)]),
    ("heh",            0x0647, "DUAL",  [("ISO", 0xFEE9), ("INI", 0xFEEB),
                                        ("MED", 0xFEEC), ("FIN", 0xFEEA)]),
    ("yeh",            0x064A, "DUAL",  [("ISO", 0xFEF1), ("INI", 0xFEF3),
                                        ("MED", 0xFEF4), ("FIN", 0xFEF2)]),
    ("yeh_hamza",      0x0626, "DUAL",  [("ISO", 0xFE89), ("INI", 0xFE8B),
                                        ("MED", 0xFE8C), ("FIN", 0xFE8A)]),
]

# Lam-alef ligatures: ((lam_cp, alef_cp), iso_cp, fin_cp) -- slots AFTER letters.
LAM_ALEFS = [
    ((0x0644, 0x0627), 0xFEFB, 0xFEFC),  # laa
    ((0x0644, 0x0623), 0xFEF7, 0xFEF8),  # laa-hamza-above
    ((0x0644, 0x0625), 0xFEF9, 0xFEFA),  # laa-hamza-below
    ((0x0644, 0x0622), 0xFEF5, 0xFEF6),  # laa-madda
]

FIRST_CHAR = 0x20
COUNT = 0xE0                  # codes 0x20..0xFF
ARABIC_SLOT_BASE = 0x80

TRANSPARENT_CPS = set(range(0x064B, 0x0660)) | {0x0670, 0x0640}

ALEF_VARIANTS = {0x0627, 0x0623, 0x0625, 0x0622}
LAM_CP = 0x0644


class Form:
    __slots__ = ("slot", "cp", "letter", "tag")

    def __init__(self, slot, cp, letter, tag):
        self.slot, self.cp, self.letter, self.tag = slot, cp, letter, tag


# Ordered form list: slots assigned sequentially from 0x80 (spec section 3).
FORMS = []
for _ltr in LETTERS:
    for _tag, _cp in _ltr[3]:
        FORMS.append(Form(ARABIC_SLOT_BASE + len(FORMS), _cp, _ltr[0], _tag))
for (_seq, _iso, _fin) in LAM_ALEFS:
    FORMS.append(Form(ARABIC_SLOT_BASE + len(FORMS), _iso, "lam_alef", "ISO"))
    FORMS.append(Form(ARABIC_SLOT_BASE + len(FORMS), _fin, "lam_alef", "FIN"))
assert len(FORMS) == 127, f"inventory must be 127 forms, got {len(FORMS)}"

CP_TO_SLOT = {}               # presentation cp -> first slot owning it
for _f in FORMS:
    CP_TO_SLOT.setdefault(_f.cp, _f.slot)

JOIN_RIGHT = {}               # base cp -> connects to previous letter?
JOIN_LEFT = {}                # base cp -> allows next letter to attach?
BASE_BY_CP = {}
for _ltr in LETTERS:
    JOIN_RIGHT[_ltr[1]] = _ltr[2] in ("DUAL", "RIGHT")
    JOIN_LEFT[_ltr[1]] = _ltr[2] == "DUAL"
    BASE_BY_CP[_ltr[1]] = _ltr[0]

FORM_SLOT_BY_LETTER_TAG = {}  # (letter, tag) -> slot
for _f in FORMS:
    if _f.letter != "lam_alef":
        FORM_SLOT_BY_LETTER_TAG.setdefault((_f.letter, _f.tag), _f.slot)

LIGATURE_SLOTS = {}           # (lam_cp, alef_cp) -> (iso_slot, fin_slot)
for (_seq, _iso, _fin) in LAM_ALEFS:
    LIGATURE_SLOTS[_seq] = (CP_TO_SLOT[_iso], CP_TO_SLOT[_fin])


def classify_form(_letter, link_prev, link_next):
    if link_prev and link_next:
        return "MED"
    if link_prev:
        return "FIN"
    if link_next:
        return "INI"
    return "ISO"


# ---------------------------------------------------------------------------
# Reference shaper (mirrors firmware ArabicShaper.cpp; keep in parity!)
# ---------------------------------------------------------------------------
def shape(text):
    """UTF-8 logical string -> shaped visual-order byte list."""
    cps = [ord(c) for c in text]
    n = len(cps)
    logical = []                                  # (joins_left_flag, byte)
    i = 0
    while i < n:
        cp = cps[i]
        if cp in TRANSPARENT_CPS:
            i += 1
            continue
        # Lam-alef ligature (harakat between lam and alef tolerated)
        if cp == LAM_CP:
            j, nxt = i + 1, None
            while j < n and cps[j] in TRANSPARENT_CPS:
                j += 1
            if j < n:
                nxt = cps[j]
            if nxt in ALEF_VARIANTS:
                iso_slot, fin_slot = LIGATURE_SLOTS[(LAM_CP, nxt)]
                prev_link = bool(logical and logical[-1][0])
                logical.append((False, fin_slot if prev_link else iso_slot))
                i = j + 1
                continue
        if cp in JOIN_RIGHT:                      # an Arabic letter
            prev_link = bool(logical and logical[-1][0])
            j = i + 1
            while j < n and cps[j] in TRANSPARENT_CPS:
                j += 1
            nxt = cps[j] if j < n else None
            next_link = bool(JOIN_LEFT[cp] and nxt in JOIN_RIGHT
                             and JOIN_RIGHT[nxt])
            letter = BASE_BY_CP[cp]
            tag = classify_form(letter, prev_link, next_link)
            logical.append((JOIN_LEFT[cp],
                            FORM_SLOT_BY_LETTER_TAG[(letter, tag)]))
        else:
            logical.append((False, cp))           # ascii passthrough
        i += 1
    # Visual reorder: walk backwards; arabic glyphs reversed, ASCII runs kept.
    out = []
    i = len(logical) - 1
    while i >= 0:
        v = logical[i][1]
        if v >= ARABIC_SLOT_BASE:
            out.append(v)
            i -= 1
        else:
            j = i
            while j >= 0 and logical[j][1] < ARABIC_SLOT_BASE:
                j -= 1
            out.extend(e[1] for e in logical[j + 1:i + 1])
            i = j
    return out


# ---------------------------------------------------------------------------
# GLCD font container (FONT_SPEC section 1)
# ---------------------------------------------------------------------------
class GlcdFont:
    def __init__(self, height, nominal_width):
        self.height = height
        self.nominal_width = nominal_width
        self.first = FIRST_CHAR
        self.count = COUNT
        self.widths = [0] * COUNT
        self.glyphs = {}                          # code -> bytearray

    @property
    def height_bytes(self):
        return (self.height + 7) >> 3

    def set_glyph_cols(self, code, cols):
        """cols: list of column ints; bit b of column = pixel row b."""
        hb = self.height_bytes
        data = bytearray(hb * len(cols))
        for x, col in enumerate(cols):
            for c in range(hb):
                data[c * len(cols) + x] = (col >> (8 * c)) & 0xFF
        self.widths[code - self.first] = len(cols)
        self.glyphs[code] = data

    def get_glyph(self, code):
        idx = code - self.first
        if idx < 0 or idx >= self.count or code not in self.glyphs:
            return None
        w = self.widths[idx]
        raw = self.glyphs[code]
        hb = self.height_bytes
        if len(raw) < w * hb:
            return None
        cols = []
        for x in range(w):
            col = 0
            for c in range(hb):
                col |= raw[c * w + x] << (8 * c)
            cols.append(col)
        return cols

    def serialize(self):
        body = bytearray()
        body += bytes([self.nominal_width & 0xFF, self.height,
                       self.first, self.count])
        body += bytes(self.widths)
        for code in range(self.first, self.first + self.count):
            g = self.glyphs.get(code)
            if g:
                body += g
        total = len(body) + 2
        assert 0 < total <= 0xFFFF
        return bytes([total & 0xFF, (total >> 8) & 0xFF]) + bytes(body)

    def save_c(self, path, name):
        data = self.serialize()
        lines = [
            "// GENERATED by tools/gen_font.py.",
            "// Each width/glyph row is labeled with its real character or form.",
            "// Glyph bytes may be tuned manually; rerun the generator to reproduce.",
            f"// Extended Arabic layout per docs/FONT_SPEC.md "
            f"(height={self.height}, codes 0x20-0xFF).",
            "#include <inttypes.h>",
            "#include <avr/pgmspace.h>",
            f"#ifndef {name.upper()}_H",
            f"#define {name.upper()}_H",
            f"#define {name.upper()}_WIDTH {self.nominal_width}",
            f"#define {name.upper()}_HEIGHT {self.height}",
            f"const uint8_t {name}[] PROGMEM = {{",
        ]
        lines += [
            f"    0x{data[0]:02X}, 0x{data[1]:02X}, // GLCD byte length",
            f"    0x{data[2]:02X}, // nominal width",
            f"    0x{data[3]:02X}, // height",
            f"    0x{data[4]:02X}, // first slot (0x20 / space)",
            f"    0x{data[5]:02X}, // slot count",
            "    // Width table: one entry per slot below.",
        ]

        def slot_label(code):
            if code == 0x20:
                return "SPACE"
            if 0x21 <= code <= 0x7E:
                ch = chr(code)
                if ch in ("\\", "'"):
                    ch = "\\" + ch
                return "'%s'" % ch
            if code == 0x7F:
                return "DEL"
            for form in FORMS:
                if form.slot == code:
                    base_chars = {
                        "hamza": "ء", "alef_madda": "آ", "alef_hamza": "أ",
                        "alef_hamza_blw": "إ", "alef": "ا", "dal": "د",
                        "thal": "ذ", "reh": "ر", "zain": "ز", "waw": "و",
                        "alef_maksura": "ى", "waw_hamza": "ؤ", "beh": "ب",
                        "teh_marbuta": "ة", "teh": "ت", "theh": "ث",
                        "jeem": "ج", "hah": "ح", "khah": "خ", "seen": "س",
                        "sheen": "ش", "sad": "ص", "dad": "ض", "tah": "ط",
                        "zah": "ظ", "ain": "ع", "ghain": "غ", "feh": "ف",
                        "qaf": "ق", "kaf": "ك", "lam": "ل", "meem": "م",
                        "noon": "ن", "heh": "ه", "yeh": "ي", "yeh_hamza": "ئ",
                        "lam_alef": "لا",
                    }
                    form_names = {"ISO": "منفرد", "INI": "أول",
                                  "MED": "وسط", "FIN": "آخر"}
                    base = base_chars.get(form.letter, form.letter)
                    state = form_names.get(form.tag, form.tag)
                    return "U+%04X %s %s (%s / %s)" % (
                        form.cp, chr(form.cp), base, state, form.letter)
            return "reserved"

        for idx, width in enumerate(self.widths):
            code = self.first + idx
            lines.append(f"    0x{width:02X}, // slot 0x{code:02X} {slot_label(code)}")

        lines.append("    // Glyph columns: each block starts at the labeled slot.")
        for code in range(self.first, self.first + self.count):
            glyph = self.glyphs.get(code)
            if not glyph:
                continue
            for off in range(0, len(glyph), 12):
                chunk = glyph[off:off + 12]
                comment = f" // slot 0x{code:02X} {slot_label(code)}" if off == 0 else ""
                lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + "," + comment)
        lines += ["};", "#endif"]
        Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_glcd_header(path):
    """Parse an existing GLCD .h font (e.g., Janat16.h)."""
    txt = Path(path).read_text(encoding="utf-8", errors="replace")
    m = re.search(r"(?:(?:const\s+)?static|const)\s+uint8_t\s+"
                  r"(\w+)\s*\[\]\s*PROGMEM\s*=\s*\{(.*?)\};",
                  txt, re.S)
    if not m:
        sys.exit(f"cannot find font array in {path}")
    body = re.sub(r"/\*.*?\*/", "", m.group(2), flags=re.S)
    body = re.sub(r"//[^\r\n]*", "", body)
    data = bytes(int(t, 16) for t in re.findall(r"0x[0-9A-Fa-f]{2}", body))
    if data[0] == 0 and data[1] == 0:
        sys.exit(f"{path}: fixed-width fonts unsupported")
    height = data[3]
    first, count = data[4], data[5]
    font = GlcdFont(height, data[2])
    font.first, font.count = first, count
    font.widths = list(data[6:6 + count])
    hb = (height + 7) >> 3
    off = 6 + count
    for idx in range(count):
        w = font.widths[idx]
        if w:
            font.glyphs[first + idx] = bytearray(data[off:off + w * hb])
        off += w * hb
    return font


# ---------------------------------------------------------------------------
# C++ mapping header emission
# ---------------------------------------------------------------------------
def _dense_joins(kv_true_cps, size=0x064B):
    vals = [1 if cp in kv_true_cps else 0 for cp in range(size)]
    lines, line = [], ""
    for i, v in enumerate(vals):
        piece = f"{v}," if i < len(vals) - 1 else f"{v}"
        if len(line) + len(piece) > 72:
            lines.append("    " + line)
            line = ""
        line += piece
    if line:
        lines.append("    " + line)
    return "\n".join(lines)


def emit_map_header(path):
    seen, rows = set(), []
    for f in sorted(FORMS, key=lambda x: x.cp):
        if f.cp in seen:
            continue
        seen.add(f.cp)
        rows.append(f"{{0x{f.cp:04X},0x{f.slot:02X}}}")
    body_rows = ""
    for off in range(0, len(rows), 8):
        body_rows += "    " + ", ".join(rows[off:off + 8]) + ",\n"

    jr = {cp for cp, v in JOIN_RIGHT.items() if v}
    jl = {cp for cp, v in JOIN_LEFT.items() if v}

    # FORMS_BY_BASE[base_cp][ISO,INI,MED,FIN] -> slot (0 = absent)
    fb = {}
    for ltr in LETTERS:
        row = [0, 0, 0, 0]
        for tag, cp in ltr[3]:
            idx = {"ISO": 0, "INI": 1, "MED": 2, "FIN": 3}[tag]
            row[idx] = CP_TO_SLOT[cp]
        # dedup rows for duplicate-cp letters (teh_marbuta): keep first hit
        for i in (1, 2):
            if row[i] == row[3] and i:
                row[i] = row[3]
        fb[ltr[1]] = row
    fb_rows = ",\n".join(
        "    {0x%04X,{%d,%d,%d,%d}}" % (cp, *row) for cp, row in sorted(fb.items()))

    lig_lines = []
    for (seq, iso_cp, fin_cp) in LAM_ALEFS:
        a, b = seq
        ia, ifin = CP_TO_SLOT[iso_cp], CP_TO_SLOT[fin_cp]
        lig_lines.append(
            "    {{0x%04X,0x%04X},0x%02X,0x%02X}," % (a, b, ia, ifin))
    ligs = "\n".join(lig_lines)

    content = f"""// GENERATED by tools/gen_font.py - do not edit by hand.
// Unicode presentation-form -> extended-font slot map (docs/FONT_SPEC.md).
#ifndef ARABIC_GLYPH_MAP_H
#define ARABIC_GLYPH_MAP_H

#include <stdint.h>

#if defined(ARDUINO) || defined(ESP8266) || defined(AVR)
#include <avr/pgmspace.h>
#else
// Host-build shim for native unit tests
#define PROGMEM
inline uint8_t pgm_read_byte(const void *p) {{
    return *(const uint8_t *)p;
}}
inline uint16_t pgm_read_word(const void *p) {{
    return *(const uint16_t *)p;
}}
#endif

namespace ArabicMap {{

struct CpSlot {{ uint16_t cp; uint8_t slot; }};

// sorted ascending by cp ({len(seen)} unique entries) - binary searchable
static const CpSlot MAP[{len(seen)}] PROGMEM = {{
{body_rows}}};

static const uint8_t JOINS_RIGHT[0x064B] PROGMEM = {{
_DENSE_JR_
}};

static const uint8_t JOINS_LEFT[0x064B] PROGMEM = {{
_DENSE_JL_
}};

struct LamAlef {{ uint16_t pair[2]; uint8_t iso, fin; }};
static const LamAlef LIGATURES[{len(LAM_ALEFS)}] PROGMEM = {{
{ligs}
}};

// form slots by base letter: columns [ISO, INI, MED, FIN], 0 = absent
struct FormRow {{ uint8_t f[4]; }};
struct BaseForms {{ uint16_t base; FormRow row; }};
static const BaseForms FORMS_BY_BASE[{len(fb)}] PROGMEM = {{
{fb_rows}
}};

inline const FormRow *formsByBase(uint16_t cp) {{
    int lo = 0, hi = {len(fb)} - 1;
    while (lo <= hi) {{
        int mid = (lo + hi) >> 1;
        uint16_t b = pgm_read_word(&FORMS_BY_BASE[mid].base);
        if (b == cp) return &FORMS_BY_BASE[mid].row;
        if (b < cp) lo = mid + 1; else hi = mid - 1;
    }}
    return nullptr;
}}

inline uint8_t joinsRight(uint16_t cp) {{
    return cp < 0x064B ? pgm_read_byte(&JOINS_RIGHT[cp]) : 0;
}}
inline uint8_t joinsLeft(uint16_t cp) {{
    return cp < 0x064B ? pgm_read_byte(&JOINS_LEFT[cp]) : 0;
}}
inline bool lookupSlot(uint16_t cp, uint8_t &slot) {{
    int lo = 0, hi = {len(seen)} - 1;
    while (lo <= hi) {{
        int mid = (lo + hi) >> 1;
        uint16_t c = pgm_read_word(&MAP[mid].cp);
        if (c == cp) {{ slot = pgm_read_byte(&MAP[mid].slot); return true; }}
        if (c < cp) lo = mid + 1; else hi = mid - 1;
    }}
    return false;
}}

}} // namespace ArabicMap
#endif
"""
    content = content.replace("_DENSE_JR_", _dense_joins(jr))
    content = content.replace("_DENSE_JL_", _dense_joins(jl))
    Path(path).write_text(content, encoding="utf-8")


# ---------------------------------------------------------------------------
# TTF rendering
# ---------------------------------------------------------------------------
def _notdef_bitmap(font, canvas):
    """Bitmap of the font's .notdef glyph (via an unassigned codepoint)."""
    from PIL import Image, ImageDraw
    img = Image.new("1", canvas, 0)
    d = ImageDraw.Draw(img)
    d.text((canvas[0] // 4, canvas[1] // 2), "\u0378", font=font, fill=1,
           anchor="ls")
    b = img.getbbox()
    if b is None:
        return frozenset()
    return frozenset((x, y) for y in range(b[1], b[3]) for x in range(b[0], b[2])
                     if img.getpixel((x, y)))


def _is_notdef(font, cp, canvas, ref):
    """True if cp renders as the .notdef box (missing glyph)."""
    from PIL import Image, ImageDraw
    img = Image.new("1", canvas, 0)
    d = ImageDraw.Draw(img)
    d.text((canvas[0] // 4, canvas[1] // 2), chr(cp), font=font, fill=1,
           anchor="ls")
    b = img.getbbox()
    if b is None:
        return True
    bits = frozenset((x, y) for y in range(b[1], b[3])
                     for x in range(b[0], b[2]) if img.getpixel((x, y)))
    return bits == ref


def _validate_coverage(font, cps, fatal=True):
    """Reject fonts whose 'coverage' is fake (.notdef boxes). Returns missing."""
    canvas = (256, 256)
    ref = _notdef_bitmap(font, canvas)
    missing = [cp for cp in cps if _is_notdef(font, cp, canvas, ref)]
    if missing and fatal:
        sample = " ".join(f"U+{c:04X}" for c in missing[:15])
        sys.exit(f"ERROR: font lacks {len(missing)} glyphs (notdef), "
                 f"e.g.: {sample}\n"
                 "Known-good: tahoma.ttf, arial.ttf, trado.ttf, segoeui.ttf, "
                 "majalla.ttf. (simpbdo lacks 23 final forms!)")
    return missing


def _probe_size(path_ttf, needed, height):
    """Pick the largest close-fitting size without clipping the ink box."""
    from PIL import ImageFont
    best = None
    for size in range(max(4, height // 2), height * 2 + 2):
        f = ImageFont.truetype(str(path_ttf), size)
        tops, bots = [], []
        ok = True
        for cp in needed:
            bb = f.getbbox(chr(cp), anchor="ls")
            if bb is None or bb[2] <= bb[0] or bb[3] <= bb[1]:
                ok = False
                break
            tops.append(bb[1]); bots.append(bb[3])
        if not ok:
            continue
        # PIL bbox bottom is exclusive. Prefer a fitting size even when a
        # slightly closer oversized candidate exists; oversized ink is lost
        # permanently when serialized into the fixed-height GLCD bitmap.
        span = max(bots) - min(tops)
        score = (span > height, abs(span - height), -size)
        if best is None or score < best[0]:
            best = (score, size)
    if best is None:
        sys.exit(f"ERROR: {path_ttf}: no usable pixel size for height "
                 f"{height}")
    return best[1]


PLAIN_ALEF_ALIASES = {
    0xFE81: 0xFE8D,  # alef with madda isolated -> plain alef isolated
    0xFE82: 0xFE8E,  # alef with madda final    -> plain alef final
    0xFE83: 0xFE8D,  # alef with hamza above
    0xFE84: 0xFE8E,
    0xFE87: 0xFE8D,  # alef with hamza below
    0xFE88: 0xFE8E,
}


def ttf_render(path_ttf, height, method="direct", plain_alef=False):
    from PIL import Image, ImageDraw, ImageFont

    HI = 256
    source_cp = (lambda cp: PLAIN_ALEF_ALIASES.get(cp, cp)) \
        if plain_alef else (lambda cp: cp)
    needed = sorted({source_cp(f.cp) for f in FORMS}
                    | set(range(0x21, 0x7F)))

    if method == "direct":
        # Render at a hinted pixel size close to the target height; no
        # rescaling. Produces crisper small sizes than supersampling.
        base_px = _probe_size(path_ttf, needed, height)
        big = None
        for attempt in range(4):
            cand = ImageFont.truetype(str(path_ttf), base_px + attempt)
            miss = _validate_coverage(cand, needed, fatal=False)
            if not miss:
                big = cand
                break
            if not all(0x20 <= c <= 0x7E for c in miss):
                sys.exit(f"ERROR: Arabic forms missing at pixel size "
                         f"{base_px + attempt}: "
                         + " ".join(f"U+{c:04X}" for c in miss[:8]))
        if big is None:
            sys.exit(f"ERROR: no hinted size renders all ASCII "
                     f"(base {base_px})")
        _probe_px = base_px + attempt
        boxes = {}
        for cp in needed:
            bb = big.getbbox(chr(cp), anchor="ls")
            if bb is None or bb[2] <= bb[0] or bb[3] <= bb[1]:
                sys.exit(f"ERROR: {path_ttf} cannot render U+{cp:04X}")
            boxes[cp] = bb
        ink_top = min(b[1] for b in boxes.values())
        ink_bot = max(b[3] for b in boxes.values())
        s = 1.0
        baseline = -ink_top + max(0, (height - (ink_bot - ink_top)) // 2)
        pad = 2
    else:
        big = ImageFont.truetype(str(path_ttf), HI)
        _validate_coverage(big, needed)
        boxes = {}
        for cp in needed:
            bb = big.getbbox(chr(cp), anchor="ls")
            if bb is None or bb[2] <= bb[0] or bb[3] <= bb[1]:
                sys.exit(f"ERROR: {path_ttf} cannot render U+{cp:04X}")
            boxes[cp] = bb
        ink_top = min(b[1] for b in boxes.values())
        ink_bot = max(b[3] for b in boxes.values())
        s = height / (ink_bot - ink_top + 1)
        baseline = round(-ink_top * s)
        pad = 2

    def render_cp(cp, tag=None):
        x0, y0, x1, y1 = boxes[cp]
        w_src, h_src = x1 - x0, y1 - y0
        img = Image.new("1", (w_src + 2 * pad, h_src + 2 * pad), 0)
        d = ImageDraw.Draw(img)
        d.text((pad - x0, pad - y0), chr(cp), font=big, fill=1, anchor="ls")
        px = img.load()
        W, Hc = img.size
        src_rows = [sum(px[x, y] << x for x in range(W)) for y in range(Hc)]

        sx0, sx1 = round(x0 * s), round(x1 * s)
        w_out = max(1, sx1 - sx0)
        cols = []
        for ox in range(w_out):
            if s == 1.0:
                cx = int(x0 - x0) + ox + pad
                if cx >= W:
                    break
            else:
                fx = x0 + (sx0 + ox + 0.5) / s
                cx = min(W - 1, max(0, int(fx - x0) + pad))
            col = 0
            for oy in range(height):
                if s == 1.0:
                    fy = oy - baseline
                    if fy < y0 or fy >= y1:
                        continue
                    cy = int(fy) - y0 + pad
                else:
                    fy = (oy - baseline) / s
                    if fy < y0 or fy > y1:
                        continue
                    cy = min(Hc - 1, max(0, int(round(fy)) - y0 + pad))
                if (src_rows[cy] >> cx) & 1:
                    col |= 1 << oy
            cols.append(col)
        # Presentation-form fonts often retain a one-pixel side bearing at
        # joining edges. Remove only the edge that must connect; preserve the
        # opposite edge so right-joining letters still separate correctly.
        if tag in ("INI", "MED"):
            while len(cols) > 1 and cols[0] == 0:
                cols.pop(0)
        if tag in ("FIN", "MED"):
            while len(cols) > 1 and cols[-1] == 0:
                cols.pop()
        if tag == "ISO":
            cols = [0] + cols + [0]
        return cols

    font = GlcdFont(height, nominal_width=10)
    space_w = max(3, round(4 * s))
    font.set_glyph_cols(0x20, [0] * space_w)
    font.set_glyph_cols(0x7F, [0])
    font.set_glyph_cols(0xFF, [0])
    for f in FORMS:
        font.set_glyph_cols(f.slot,
                            render_cp(source_cp(f.cp), tag=f.tag))
    for cp in range(0x21, 0x7F):
        font.set_glyph_cols(cp, render_cp(cp))
    font.nominal_width = max(font.widths)
    return font


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------
def cmd_emit_spec(_args):
    print("| Slot | Letter | Tag | Presentation CP |")
    print("|------|--------|-----|-----------------|")
    for f in FORMS:
        print(f"| 0x{f.slot:02X} | {f.letter} | {f.tag} | U+{f.cp:04X} |")


def cmd_ttf(args):
    if not (4 <= args.height <= 16):
        sys.exit("--height must be 4..16")
    font = ttf_render(args.font, args.height, method=args.method,
                      plain_alef=args.plain_alef)
    font.save_c(args.out_font, Path(args.out_font).stem)
    emit_map_header(args.out_map)
    print(f"wrote {args.out_font} and {args.out_map}")


def cmd_preview(args):
    from PIL import Image, ImageDraw
    font = parse_glcd_header(args.font)
    S = args.scale
    texts = args.text or ["الصلاة على النبي",
                          "لا إله إلا الله محمد رسول الله",
                          "عيدكم مبارك وكل عام وأنتم بخير",
                          "12:30 25-12-1447"]
    W = max((sum(font.get_glyph(b).__len__() if font.get_glyph(b) else 4
                 for b in shape(t)) for t in texts), default=20) * S + 8 * S
    H = (len(texts) * (font.height + 6) + 2) * S
    img = Image.new("1", (W, H), 0)
    d = ImageDraw.Draw(img)
    for li, t in enumerate(texts):
        y = (li * (font.height + 6) + 2) * S
        x = 4 * S
        for b in shape(t):
            g = font.get_glyph(b) if 0x20 <= b <= 0xFF else None
            if g is None:
                x += 4 * S
                continue
            for cx, col in enumerate(g):
                for bit in range(min(font.height, 16)):
                    if (col >> bit) & 1:
                        d.rectangle([x + cx * S, y + bit * S,
                                     x + cx * S + S - 1, y + bit * S + S - 1],
                                    fill=1)
            x += len(g) * S
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    img.save(args.out)
    print(f"wrote {args.out}")


def cmd_sheet(args):
    from PIL import Image, ImageDraw
    font = parse_glcd_header(args.src)
    S = 6
    cols_n = 16
    rows_n = (font.count + cols_n - 1) // cols_n
    CW = max(font.nominal_width, 10) * S + 4 * S
    CH = (font.height + 10) * S
    img = Image.new("L", (cols_n * CW, rows_n * CH), 0)
    d = ImageDraw.Draw(img)
    for idx in range(font.count):
        code = font.first + idx
        gx, gy = (idx % cols_n) * CW, (idx // cols_n) * CH
        d.text((gx + 4, gy + 2), f"{code:02X}", fill=200)
        g = font.get_glyph(code)
        if not g:
            continue
        ox, oy = gx + 3 * S, gy + 9 * S
        for cx, col in enumerate(g):
            for bit in range(min(font.height, 16)):
                if (col >> bit) & 1:
                    d.rectangle([ox + cx * S, oy + bit * S,
                                 ox + cx * S + S - 1, oy + bit * S + S - 1],
                                fill=255)
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    img.save(args.out)
    print(f"wrote {args.out} ({font.count} slots)")


def cmd_extract(args):
    legacy = parse_glcd_header(args.src)
    fallback = parse_glcd_header(args.fallback_font) \
        if args.fallback_font else None
    if fallback and fallback.height != legacy.height:
        sys.exit("ERROR: legacy and fallback font heights differ")
    norm = []
    if Path(args.map).exists():
        raw = json.loads(Path(args.map).read_text(encoding="utf-8"))
        for k, v in raw.items():
            try:
                key = int(str(k), 0)
            except ValueError:
                key = ord(str(k)[0])
            values = v if isinstance(v, list) else [v]
            for value in values:
                val = int(value, 0) if isinstance(value, str) else int(value)
                norm.append((key, val))
    cp_to_legacy = {cp: key for key, cp in norm if cp in CP_TO_SLOT}
    font = GlcdFont(legacy.height, legacy.nominal_width)
    space = fallback.get_glyph(0x20) if fallback else None
    font.set_glyph_cols(0x20, space or [0] * max(3, legacy.widths[0]))
    font.set_glyph_cols(0x7F, [0])
    font.set_glyph_cols(0xFF, [0])
    unresolved = []
    legacy_count = 0
    fallback_count = 0
    for f in FORMS:
        if f.cp == 0xFE80:  # standalone hamza: corrected Basit bitmap
            font.set_glyph_cols(
                f.slot,
                [0, 12, 18, 18, 2, 4] if legacy.height == 7 else
                [0, 0, 2816, 1664, 1728, 1728, 512, 0, 0])
            legacy_count += 1
            continue
        # Some contextual slots intentionally share a presentation code point
        # (teh-marbuta INI/MED), so resolve by code point for every slot.
        lk = cp_to_legacy.get(f.cp)
        g = legacy.get_glyph(lk) if lk is not None else None
        if g:
            font.set_glyph_cols(f.slot, g)
            legacy_count += 1
        elif fallback and fallback.get_glyph(f.slot):
            font.set_glyph_cols(f.slot, fallback.get_glyph(f.slot))
            fallback_count += 1
        else:
            unresolved.append(f)
    for cp in range(0x21, 0x7F):
        g = fallback.get_glyph(cp) if fallback else None
        if g:
            font.set_glyph_cols(cp, g)
    if unresolved and not args.allow_missing:
        names = ", ".join(f"U+{u.cp:04X}" for u in unresolved[:10])
        sys.exit(f"ERROR: {len(unresolved)} forms unmapped, e.g.: {names}\n"
                 f"Extend {args.map} (legacy-code -> presentation-cp) "
                 "or pass --allow-missing.")
    font.save_c(args.out_font, Path(args.out_font).stem)
    emit_map_header(args.out_map)
    print(f"wrote {args.out_font} + {args.out_map}; "
          f"legacy={legacy_count} fallback={fallback_count} "
          f"unresolved={len(unresolved)}")


def main():
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("emit-spec").set_defaults(fn=cmd_emit_spec)

    t = sub.add_parser("ttf", help="generate font from a TTF")
    t.add_argument("--font", required=True)
    t.add_argument("--height", type=int, required=True)
    t.add_argument("--method", choices=["direct", "supersample"],
                   default="direct")
    t.add_argument("--plain-alef", action="store_true",
                   help="render decorated alefs as plain alef (gains height)")
    t.add_argument("--out-font", required=True)
    t.add_argument("--out-map", default=str(REPO / "src/arabic_glyph_map.h"))
    t.set_defaults(fn=cmd_ttf)

    pv = sub.add_parser("preview", help="shape + render text to PNG")
    pv.add_argument("--font", required=True)
    pv.add_argument("--out", required=True)
    pv.add_argument("--scale", type=int, default=8)
    pv.add_argument("--text", action="append")
    pv.set_defaults(fn=cmd_preview)

    sh = sub.add_parser("sheet", help="contact sheet of a GLCD header")
    sh.add_argument("--src", required=True)
    sh.add_argument("--out", required=True)
    sh.set_defaults(fn=cmd_sheet)

    ex = sub.add_parser("extract", help="reuse legacy bitmaps via map json")
    ex.add_argument("--src", required=True)
    ex.add_argument("--map", required=True)
    ex.add_argument("--fallback-font",
                    help="extended GLCD font for ASCII and missing forms")
    ex.add_argument("--out-font", required=True)
    ex.add_argument("--out-map", default=str(REPO / "src/arabic_glyph_map.h"))
    ex.add_argument("--allow-missing", action="store_true")
    ex.set_defaults(fn=cmd_extract)

    args = p.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
