#!/usr/bin/env python3
"""
Extract release notes for a specific tag from a Keep-a-Changelog file.

Usage:
    extract-changelog.py --tag v0.2.1 --output notes.md CHANGELOG.md

Looks for an H2 section matching the tag (with or without leading 'v', and
with or without a date suffix). Writes that section to the output file.
Exits 0 on success, 1 if the section was not found.
"""

import argparse
import re
import sys


def extract(changelog: str, tag: str) -> str:
    """Return the body of the section for the given tag, or empty string."""
    # Accept all of: ## [v0.2.1], ## [0.2.1], ## v0.2.1, ## 0.2.1, with optional " - <date>"
    bare = tag.lstrip("v")
    patterns = [
        rf"^##\s*\[?\s*v?{re.escape(bare)}\s*\]?\s*(?:-\s*\S+\s*)?$",
    ]
    lines = changelog.splitlines()

    start = None
    for i, line in enumerate(lines):
        for pat in patterns:
            if re.match(pat, line.strip(), re.IGNORECASE):
                start = i
                break
        if start is not None:
            break

    if start is None:
        return ""

    # Capture lines until the next H2 (or EOF).
    body_lines = []
    for line in lines[start + 1 :]:
        if re.match(r"^##\s+", line):
            break
        body_lines.append(line)

    return "\n".join(body_lines).strip() + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", required=True, help="e.g. v0.2.1 or 0.2.1")
    ap.add_argument("--output", required=True, help="output file")
    ap.add_argument("changelog", help="path to CHANGELOG.md")
    args = ap.parse_args()

    with open(args.changelog, encoding="utf-8") as f:
        body = extract(f.read(), args.tag)

    if not body.strip():
        sys.stderr.write(f"no section found for {args.tag}\n")
        return 1

    with open(args.output, "w", encoding="utf-8") as f:
        f.write(body)
    return 0


if __name__ == "__main__":
    sys.exit(main())
