#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compare the retained host contracts with the locked upstream source offline."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMMIT = "417f333263ff169e8f449c931b70db9dfd7b8ef4"
EXPECTED_CASES = 7663
FILES = ("layout.c", "layout.h", "vbios.c", "vbios.h", "prep.c", "prep.h",
         "boot.c", "boot.h", "diagnostic.h")


def run(args, **kwargs):
    return subprocess.run(args, check=True, text=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream", type=Path, help="local pinned upstream checkout")
    parser.add_argument("--cc", default="cc", help="C compiler executable (default: cc)")
    args = parser.parse_args()
    source = args.upstream.resolve() / "kernel" / "turing_unlock"
    head = run(["git", "-C", str(args.upstream), "rev-parse", "HEAD"], capture_output=True).stdout.strip()
    if head != COMMIT:
        raise ValueError(f"upstream HEAD is {head}, expected {COMMIT}")
    lock = (ROOT / "UPSTREAM_LOCK.md").read_text(encoding="utf-8")
    for name in FILES:
        match = re.search(r"^- `" + re.escape(name) + r"`: `([0-9a-f]{64})`$", lock, re.MULTILINE)
        if not match:
            raise ValueError(f"missing hash for {name} in UPSTREAM_LOCK.md")
        digest = hashlib.sha256((source / name).read_bytes()).hexdigest()
        if digest != match.group(1):
            raise ValueError(f"upstream source hash mismatch for {name}: {digest}")
    outputs = []
    with tempfile.TemporaryDirectory(prefix="turing-differential-") as tmp:
        for label, directory in (("host", ROOT / "host"), ("upstream", source)):
            build = Path(tmp) / label
            build.mkdir()
            executable = build / "driver"
            # Upstream boot.c includes Linux firmware/string headers; its parser
            # only needs this struct and ordinary host errno/string definitions.
            linux = build / "linux"
            linux.mkdir()
            (linux / "firmware.h").write_text(
                "#include <stddef.h>\n#include <stdint.h>\n"
                "struct firmware { size_t size; const uint8_t *data; };\n")
            (linux / "string.h").write_text("#include <string.h>\n#include <errno.h>\n")
            flags = ["-DTU_UPSTREAM_BOOT"] if label == "upstream" else []
            run([args.cc, *flags, "-I", str(build), "-std=c11", "-Wall", "-Wextra", "-I", str(directory),
                 str(ROOT / "tests" / "differential_driver.c"), str(directory / "layout.c"),
                 str(directory / "prep.c"), str(directory / "boot.c"),
                 str(directory / "vbios.c"), "-o", str(executable)])
            result = run([str(executable)], capture_output=True)
            cases = result.stdout.splitlines()
            if len(cases) != EXPECTED_CASES:
                raise ValueError(f"{label} driver returned {len(cases)} cases; expected {EXPECTED_CASES}")
            outputs.append(cases)
    if outputs[0] != outputs[1]:
        from itertools import zip_longest
        for line, (host, upstream) in enumerate(zip_longest(*outputs), 1):
            if host != upstream:
                print(f"Mismatch at case {line}:\n  host: {host}\n  upstream: {upstream}", file=sys.stderr)
                break
        return 1
    print(f"Differential check passed: {len(outputs[0])} synthetic cases; upstream {COMMIT}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"Differential check failed: {exc}", file=sys.stderr)
        if isinstance(exc, subprocess.CalledProcessError) and exc.stderr:
            print(exc.stderr, file=sys.stderr)
        sys.exit(1)
