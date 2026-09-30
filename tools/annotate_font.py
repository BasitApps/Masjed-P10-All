"""Rewrite a GLCD font header with slot labels beside every glyph."""
import sys
from pathlib import Path

import gen_font


def main():
    if len(sys.argv) < 2:
        raise SystemExit("usage: annotate_font.py FONT.h [FONT2.h ...]")
    for raw in sys.argv[1:]:
        path = Path(raw)
        font = gen_font.parse_glcd_header(path)
        font.save_c(path, path.stem)
        print(f"annotated {path}")


if __name__ == "__main__":
    main()
