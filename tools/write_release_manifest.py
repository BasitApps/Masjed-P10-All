#!/usr/bin/env python3
"""Write a small, reviewable manifest for the four P10 release binaries."""

import argparse
from hashlib import sha256
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
PROFILES = ("1x1", "2x1", "3x1", "4x1")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    args = parser.parse_args()
    if any(not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", v) for v in (args.version, args.tag)):
        parser.error("version/tag must be plain names, not paths")
    lines = [f"version={args.version}", f"tag={args.tag}"]
    for profile in PROFILES:
        relative = Path(f".pio/build/p10_{profile}_release/updateP104_p10_{profile}_release.bin")
        path = ROOT / relative
        if not path.is_file():
            raise SystemExit(f"missing build artifact: {relative}")
        digest = sha256(path.read_bytes()).hexdigest()
        lines.extend(
            (
                f"profile={profile}",
                f"file={relative.as_posix()}",
                f"size={path.stat().st_size}",
                f"sha256={digest}",
            )
        )

    output = ROOT / "release" / args.tag / "manifest.txt"
    if output.exists():
        raise SystemExit(f"archive already exists; not overwritten: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write("\n".join(lines) + "\n")
    print(output)
    print(output.read_text(encoding="ascii"), end="")


if __name__ == "__main__":
    main()
