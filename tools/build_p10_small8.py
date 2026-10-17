"""Build the complete bold 8-row Arabic font used by P10 small text.

Arabic forms come from the reviewed Tahoma Bold 16px font and are reduced
with a 2x2 max-pool so thin strokes survive the physical P10 dot gap. ASCII
and digits stay byte-identical to the existing Tahoma7 family.
"""
from pathlib import Path
import argparse
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_font as g


ALIASES = {
    "alef_madda": "alef",
    "alef_hamza": "alef",
    "alef_hamza_blw": "alef",
    "waw_hamza": "waw",
    "yeh_hamza": "yeh",
}


def pool_16_to_8(cols, threshold=1):
    """Reduce a reviewed 16-row glyph without averaging away lit pixels."""
    out = []
    for x in range(0, len(cols), 2):
        value = 0
        for y in range(8):
            lit = sum((cols[xx] >> yy) & 1
                      for xx in (x, min(x + 1, len(cols) - 1))
                      for yy in (2 * y, min(2 * y + 1, 15)))
            if lit >= threshold:
                value |= 1 << y
        out.append(value)
    return out


def build(ascii_source, arabic_source=None, ttf_source=None, threshold=1):
    ascii_font = g.parse_glcd_header(ascii_source)
    if ttf_source:
        source = g.ttf_render(ttf_source, 8, method="direct", plain_alef=True)
    else:
        source = g.parse_glcd_header(arabic_source)
        assert source.height == 16
    out = g.GlcdFont(8, 10)

    # Keep the established small-font ASCII/digits untouched.
    for code in range(32, 128):
        out.set_glyph_cols(code, ascii_font.get_glyph(code) or [0])
    for code in range(128, 256):
        cols = source.get_glyph(code) or [0]
        out.set_glyph_cols(code, pool_16_to_8(cols, threshold) if source.height == 16 else cols)

    # Small P10 text does not need detached hamza/madda variants.
    for form in g.FORMS:
        if form.letter == "hamza":
            out.set_glyph_cols(form.slot, [0])
            continue
        base = ALIASES.get(form.letter, form.letter)
        if base != form.letter:
            slot = g.FORM_SLOT_BY_LETTER_TAG[base, form.tag]
            out.set_glyph_cols(form.slot, out.get_glyph(slot))

    # All four lam-alef variants use the plain ligature at this size.
    plain_iso = g.CP_TO_SLOT[0xFEFB]
    plain_fin = g.CP_TO_SLOT[0xFEFC]
    for _, iso, fin in g.LAM_ALEFS:
        out.set_glyph_cols(g.CP_TO_SLOT[iso], out.get_glyph(plain_iso))
        out.set_glyph_cols(g.CP_TO_SLOT[fin], out.get_glyph(plain_fin))

    out.nominal_width = max(out.widths)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ascii", type=Path,
                        default=ROOT / "lib/DMDESP/fonts/Tahoma7.h")
    parser.add_argument("--arabic", type=Path,
                        default=ROOT / "lib/DMDESP/fonts/TahomaBold16.h")
    parser.add_argument("--ttf", type=Path,
                        help="Use a regular TTF rendered directly at 8px")
    parser.add_argument("--threshold", type=int, choices=(1, 2, 3, 4), default=1,
                        help="Minimum lit pixels in each 2x2 Bold source block")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--name", default="TahomaP10_8_Bold")
    args = parser.parse_args()
    font = build(args.ascii, args.arabic, args.ttf, args.threshold)
    font.save_c(args.out, args.name)
    print("wrote", args.out, "height", font.height,
          "slots", font.count, "max-width", font.nominal_width)


if __name__ == "__main__":
    main()
