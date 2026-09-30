"""Build Basit Arabic slots by copying the exact Janat slots named by Android."""
import argparse
import re
from pathlib import Path

import gen_font


ADD_GLYPH = re.compile(
    r"addGlyph\('((?:\\.|[^'])*)',\s*'((?:\\.|[^'])*)',\s*"
    r"'((?:\\.|[^'])*)',\s*'((?:\\.|[^'])*)',\s*"
    r"'((?:\\.|[^'])*)',\s*\d+\);"
)


def java_char(value):
    escapes = {r"\'": "'", r'\"': '"', r"\\": "\\"}
    return escapes.get(value, value)


def read_android_map(path):
    result = {}
    text = Path(path).read_text(encoding="utf-8")
    for match in ADD_GLYPH.finditer(text):
        base, iso, ini, med, fin = map(java_char, match.groups())
        if len(base) == 1:
            result[ord(base)] = {
                "ISO": ord(iso), "INI": ord(ini),
                "MED": ord(med), "FIN": ord(fin),
            }
    return result


def build(source, android_java, output):
    legacy = gen_font.parse_glcd_header(source)
    android = read_android_map(android_java)
    font = gen_font.GlcdFont(legacy.height, legacy.nominal_width)

    # Legacy 7px FontCreator exports occupy bits 1..7 (bit 0 is padding).
    shift = 1 if legacy.height == 7 else 0
    if shift:
        assert all(not (v & 1) for g in legacy.glyphs.values() for v in g), \
            "Expected original FontCreator 7px packing"

    def artwork(slot):
        cols = legacy.get_glyph(slot)
        return None if cols is None else [c >> shift for c in cols]

    def contextual_art(base, tag):
        # The legacy apostrophe slot is only a placeholder for U+0621 and
        # renders as two broken bars at 7px. Keep one canonical standalone
        # hamza bitmap so regenerated Basit fonts match the firmware assets.
        if base == 0x0621:
            return ([0, 12, 18, 18, 2, 4] if legacy.height == 7 else
                    [0, 0, 2816, 1664, 1728, 1728, 512, 0, 0])
        if base != 0x0629:
            return artwork(android[base][tag])
        # Janat aliases teh-marbuta to heh. Keep its body, add the missing dots.
        cols = artwork(ord('_'))
        dots = (2, 4) if legacy.height == 7 else (2, 3, 5, 6)
        mask = 1 if legacy.height == 7 else 0x0C
        assert all(x < len(cols) and not (cols[x] & mask) for x in dots)
        for x in dots:
            cols[x] |= mask
        return cols

    # ASCII must remain ASCII: legacy letters here are actually Arabic art.
    ascii_font = gen_font.parse_glcd_header(
        Path(__file__).resolve().parents[1] /
        ("lib/DMDESP/fonts/Tahoma%d.h" % legacy.height))
    for code in range(0x20, 0x80):
        glyph = ascii_font.get_glyph(code)
        if glyph is not None:
            font.set_glyph_cols(code, glyph)
    font.set_glyph_cols(0x7F, [0])
    font.set_glyph_cols(0xFF, [0])

    copied = 0
    for name, base, _join, variants in gen_font.LETTERS:
        if base not in android:
            raise SystemExit("ArabicShaping.java lacks " + name)
        for tag, _cp in variants:
            source_slot = android[base][tag]
            glyph = contextual_art(base, tag)
            if glyph is None:
                raise SystemExit(
                    "%s %s points to empty Janat slot 0x%02X" %
                    (name, tag, source_slot))
            target_slot = gen_font.FORM_SLOT_BY_LETTER_TAG[(name, tag)]
            font.set_glyph_cols(target_slot, glyph)
            copied += 1

    lam_alef = artwork(ord("}"))
    if lam_alef is None:
        raise SystemExit("Janat slot 0x7D (lam-alef) is empty")
    for form in gen_font.FORMS:
        if form.letter == "lam_alef":
            font.set_glyph_cols(form.slot, lam_alef)
            copied += 1

    if copied != len(gen_font.FORMS):
        raise SystemExit("expected 127 forms, copied %d" % copied)
    font.save_c(output, Path(output).stem)
    saved = gen_font.parse_glcd_header(output)
    for name, base, _join, variants in gen_font.LETTERS:
        for tag, _cp in variants:
            slot = gen_font.FORM_SLOT_BY_LETTER_TAG[(name, tag)]
            assert saved.get_glyph(slot) == contextual_art(base, tag)
    assert all(c < (1 << legacy.height)
               for slot in range(0x80, 0xFF)
               for c in saved.get_glyph(slot))
    print("wrote %s: %d/127 Janat-derived slots; added teh-marbuta dots" % (output, copied))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", required=True)
    parser.add_argument("--android-map", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    build(args.src, args.android_map, args.out)


if __name__ == "__main__":
    main()
