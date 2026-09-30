#!/usr/bin/env python3
"""Minimal Basit API v2 tester; Python standard library only."""

import argparse
import base64
import hashlib
import hmac
import urllib.error
import urllib.parse
import urllib.request


def derive(password: str, device_id: str) -> bytes:
    return hashlib.pbkdf2_hmac(
        "sha256", password.encode(), f"{device_id}|Basit-v2".encode(), 4096, 32
    )


def tag(key: bytes, data: bytes) -> str:
    raw = hmac.new(key, data, hashlib.sha256).digest()[:16]
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode()


def fields(data: bytes) -> dict[str, str]:
    return dict(line.split("=", 1) for line in data.decode().splitlines())


def self_test() -> None:
    key = derive("ExamplePass12", "5238166")
    assert key.hex() == "519f2190760ed7cc10d83ee3446fcf67da3849b1cca2e697310b8895ff798e96"
    body = b"y=2026\nm=9\nd=2"
    signed = b"BASIT2\nPOST\n/v2/setDateTime\n5238166\nAAAAAAAAAAAAAAAAAAAAAA\n" + body
    assert tag(key, signed) == "fv3I3enroN3ZzBBumuxKtg"
    print("PASS")


def call(base: str, device_id: str, password: str, path: str, body: bytes) -> None:
    challenge_url = f"{base}/v2/challenge?id={urllib.parse.quote(device_id)}"
    challenge = fields(urllib.request.urlopen(challenge_url, timeout=5).read())
    nonce = challenge["nonce"]
    key = derive(password, device_id)
    canonical = (
        f"BASIT2\nPOST\n{path}\n{device_id}\n{nonce}\n".encode() + body
    )
    request = urllib.request.Request(
        base + path,
        data=body,
        method="POST",
        headers={
            "Content-Type": "text/plain; charset=utf-8",
            "X-Basit-Id": device_id,
            "X-Basit-Nonce": nonce,
            "X-Basit-Mac": tag(key, canonical),
        },
    )
    try:
        response = urllib.request.urlopen(request, timeout=8)
    except urllib.error.HTTPError as error:
        response = error
    response_body = response.read()
    expected = tag(
        key,
        f"BASIT2\nRESP\n{nonce}\n{response.status}\n".encode() + response_body,
    )
    assert hmac.compare_digest(response.headers.get("X-Basit-Mac", ""), expected)
    print(response.status)
    print(response_body.decode())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--base", default="http://192.168.1.177")
    parser.add_argument("--id")
    parser.add_argument("--password")
    parser.add_argument("--path", default="/v2/getData")
    parser.add_argument("--body", default="")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if not args.id or not args.password:
        parser.error("--id and --password are required")
    call(args.base, args.id, args.password, args.path, args.body.encode())


if __name__ == "__main__":
    main()
