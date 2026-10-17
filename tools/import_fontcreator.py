"""Import all eight FontCreator exports into the production GLCD headers.

FontCreator writes an older GLCD C-header dialect.  This tool validates every
export first, then rewrites the headers through gen_font so production files
keep the current comments, slot labels, and naming convention.
"""
import argparse
import tempfile
from pathlib import Path

import gen_font


SPECS = (
    ("BasitFont7.h", 7),
    ("BasitFont7Indic.h", 7),
    ("BasitFont16.h", 16),
    ("BasitFont16Indic.h", 16),
    ("Tahoma7.h", 7),
    ("Tahoma7Indic.h", 7),
    ("TahomaBold16.h", 16),
    ("TahomaBold16Indic.h", 16),
)


def convert(exports: Path, output: Path) -> None:
    missing = [name for name, _ in SPECS if not (exports / name).is_file()]
    if missing:
        raise SystemExit(
            "Missing FontCreator exports in %s:\n  %s" %
            (exports, "\n  ".join(missing))
        )

    with tempfile.TemporaryDirectory(prefix="font-import-", dir=output.parent) as tmp:
        stage = Path(tmp)
        for name, height in SPECS:
            src = exports / name
            font = gen_font.parse_glcd_header(src)
            if font.height != height or font.first != 0x20 or font.count != 0xE0:
                raise SystemExit(
                    "%s: expected %dpx, first=0x20, count=0xE0; got %dpx, "
                    "first=0x%02X, count=0x%02X" %
                    (src, height, font.height, font.first, font.count)
                )
            dest = stage / name
            font.save_c(dest, Path(name).stem)

        output.mkdir(parents=True, exist_ok=True)
        for name, _ in SPECS:
            staged = stage / name
            staged.replace(output / name)

    print("Imported and validated %d FontCreator exports" % len(SPECS))


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exports", type=Path,
        default=root / "GLCDFontCreator2" / "exports",
        help="folder containing the eight exported .h files",
    )
    parser.add_argument(
        "--output", type=Path,
        default=root / "lib" / "DMDESP" / "fonts",
        help="production font-header folder",
    )
    args = parser.parse_args()
    convert(args.exports.resolve(), args.output.resolve())


if __name__ == "__main__":
    main()
