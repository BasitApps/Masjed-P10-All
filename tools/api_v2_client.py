#!/usr/bin/env python3
"""Root-path compact UTF-8 POST client; AP credentials are the access boundary."""
import argparse
import re
import urllib.error
import urllib.request


def body_fields(fields):
    lines = ["v=2"]
    seen = {"v"}
    for field in fields:
        key, separator, value = field.partition("=")
        if not separator or not re.fullmatch(r"[A-Za-z0-9_]{1,19}", key) or key in seen:
            raise ValueError("fields must have unique key=value names (v is automatic)")
        seen.add(key)
        lines.append(key + "=" + value.replace("\\", "\\\\").replace("\n", "\\n"))
    body = "\n".join(lines).encode("utf-8")
    if len(seen) > 16 or len(body) >= 512:
        raise ValueError("request exceeds the clock's field/body capacity")
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="http://192.168.1.177")
    parser.add_argument("--path", default="/getData")
    parser.add_argument("--field", action="append", default=[], metavar="KEY=VALUE")
    args = parser.parse_args()
    if not re.fullmatch(r"/[A-Za-z][A-Za-z0-9]*", args.path):
        parser.error("use one root endpoint, such as /getData or /setDateTime")
    try:
        body = body_fields(args.field)
    except ValueError as error:
        parser.error(str(error))
    request = urllib.request.Request(args.base.rstrip("/") + args.path, data=body,
                                     headers={"Content-Type": "text/plain; charset=utf-8"}, method="POST")
    try:
        response = urllib.request.urlopen(request, timeout=8)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        print(response.status)
        print(response.read().decode("utf-8"))


if __name__ == "__main__":
    main()
