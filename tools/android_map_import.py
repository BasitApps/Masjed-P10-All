"""Convert the Android ArabicShaping.java glyph table to legacy map JSON."""
import json, re, sys
from pathlib import Path
import gen_font

if len(sys.argv) != 3:
    raise SystemExit("usage: android_map_import.py ArabicShaping.java output.json")
src, out = map(Path, sys.argv[1:])
text = src.read_text(encoding="utf-8")
android = {}
for m in re.finditer(r"addGlyph\('(.{1})',\s*'(.{1})',\s*'(.{1})',\s*'(.{1})',\s*'(.{1})',", text):
    android[ord(m.group(1))] = [ord(x) for x in m.groups()[1:5]]

forms = {}
for name, base, _join, variants in gen_font.LETTERS:
    forms[base] = [(tag, cp) for tag, cp in variants]

out_map = {}
for base, variants in forms.items():
    glyph = android.get(base)
    if not glyph:
        continue
    by_tag = dict(zip(("ISO", "INI", "MED", "FIN"), glyph))
    for tag, cp in variants:
        slot = by_tag[tag]
        out_map.setdefault(f"0x{slot:02X}", []).append(f"0x{cp:04X}")

Path(out).write_text(json.dumps(out_map, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"wrote {out}: {len(out_map)} Janat slots")
