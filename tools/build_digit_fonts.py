"""Replace clipped numerals; generate selectable digit artwork in Flash only."""
from pathlib import Path
import gen_font as g

ROOT = Path(__file__).resolve().parents[1]
DIGITS = [
    [0x3e,0x51,0x49,0x45,0x3e], [0,0x42,0x7f,0x40,0],
    [0x42,0x61,0x51,0x49,0x46], [0x21,0x41,0x45,0x4b,0x31],
    [0x18,0x14,0x12,0x7f,0x10], [0x27,0x45,0x45,0x45,0x39],
    [0x3c,0x4a,0x49,0x49,0x30], [1,0x71,9,5,3],
    [0x36,0x49,0x49,0x49,0x36], [6,0x49,0x49,0x29,0x1e],
]

def run():
    for height in (7,16):
        scale = 1 if height == 7 else 2
        legacy = g.parse_glcd_header(ROOT / f'GLCDFontCreator2/Janat{height}.h')
        for family in ('BasitFont', 'Tahoma'):
            name = f'{family}{height}'
            path = ROOT / f'lib/DMDESP/fonts/{name}.h'
            font = g.parse_glcd_header(path)
            for n, cols in enumerate(DIGITS):
                expanded = []
                for col in cols:
                    bits = sum(((1 << scale)-1) << (y*scale)
                               for y in range(7) if col & (1 << y))
                    expanded.extend([bits] * scale)
                font.set_glyph_cols(48+n, expanded + [0])
            font.save_c(path, name)
            for n in range(10):
                cols = legacy.get_glyph(48+n)
                if height == 7:
                    assert all(c & 1 == 0 for c in cols)
                    cols = [c >> 1 for c in cols]
                font.set_glyph_cols(48+n, cols)
            alternate = name + 'Indic'
            font.save_c(path.with_name(alternate+'.h'), alternate)
            for target in (path, path.with_name(alternate+'.h')):
                saved = g.parse_glcd_header(target)
                assert all(0 <= c < 1 << height for n in range(10)
                           for c in saved.get_glyph(48+n))
    print('PASS: complete Western/Indic digit fonts, both heights, two families')

if __name__ == '__main__':
    run()
