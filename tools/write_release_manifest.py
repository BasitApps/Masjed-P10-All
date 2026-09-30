#!/usr/bin/env python3
"""Write a small, reviewable manifest for the four P10 release binaries."""

from hashlib import sha256
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSION = "P104-rc1"
TAG = "firmware-rc1"
PROFILES = ("1x1", "2x1", "3x1", "4x1")


def main() -> None:
    lines = [f"version={VERSION}", f"tag={TAG}"]
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

    output = ROOT / "release" / TAG / "manifest.txt"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(output)
    print(output.read_text(encoding="ascii"), end="")


if __name__ == "__main__":
    main()
