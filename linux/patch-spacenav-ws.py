#!/usr/bin/env python3
"""
Patches spacenav-ws so buttons don't snap the OnShape view.

By default, spacenav-ws interprets button presses as "snap to front view".
This patch makes button events a no-op in spacenav-ws — button mapping is
handled separately by spnav-buttons.

Usage:
    python3 patch-spacenav-ws.py           # auto-find and patch
    python3 patch-spacenav-ws.py /path/to/controller.py
"""
import glob
import re
import subprocess
import sys


def find_controller():
    # Try via uv run (works if spacenav-ws is installed as a uv tool)
    try:
        result = subprocess.run(
            [
                "uv", "run", "--with", "spacenav-ws", "python3", "-c",
                "import spacenav_ws.controller; print(spacenav_ws.controller.__file__)",
            ],
            capture_output=True, text=True, timeout=30,
        )
        path = result.stdout.strip()
        if result.returncode == 0 and path:
            return path
    except Exception:
        pass

    # Search in uv cache (works when spacenav-ws is run via uvx)
    import os
    home = os.path.expanduser("~")
    for pattern in [
        f"{home}/.cache/uv/**/spacenav_ws/controller.py",
        f"{home}/.local/share/uv/**/spacenav_ws/controller.py",
    ]:
        matches = glob.glob(pattern, recursive=True)
        if matches:
            return matches[0]

    return None


def patch(path):
    with open(path) as f:
        content = f.read()

    # Already patched: ButtonEvent handler body is just 'return'
    if re.search(
        r"if isinstance\(event, ButtonEvent\):\s*\n\s+return\s*\n", content
    ):
        print(f"Already patched: {path}")
        return True

    if "isinstance(event, ButtonEvent)" not in content:
        print(f"ERROR: ButtonEvent check not found in {path}", file=sys.stderr)
        print("Wrong file or spacenav-ws version not supported.", file=sys.stderr)
        return False

    # Replace the entire body of the ButtonEvent block with just 'return'
    patched = re.sub(
        r"(        if isinstance\(event, ButtonEvent\):)(?:\n            [^\n]+)+",
        r"\1\n            return",
        content,
    )

    if patched == content:
        print(f"ERROR: Could not apply patch to {path}", file=sys.stderr)
        print(
            "Manual fix: find 'if isinstance(event, ButtonEvent):' and make "
            "the only line in that block 'return'",
            file=sys.stderr,
        )
        return False

    with open(path, "w") as f:
        f.write(patched)
    print(f"Patched: {path}")
    return True


if __name__ == "__main__":
    if len(sys.argv) > 1:
        path = sys.argv[1]
    else:
        print("Locating spacenav-ws controller.py...")
        path = find_controller()
        if not path:
            print(
                "ERROR: spacenav-ws not found in uv cache.\n"
                "Run 'uvx spacenav-ws@latest serve' once first, then re-run this script.",
                file=sys.stderr,
            )
            sys.exit(1)
        print(f"Found: {path}")

    sys.exit(0 if patch(path) else 1)
