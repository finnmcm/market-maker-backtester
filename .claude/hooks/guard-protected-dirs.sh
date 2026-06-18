#!/usr/bin/env bash
# PreToolUse guard: keep the assistant out of the author-owned, learning-critical
# code. Reads the PreToolUse hook payload (JSON) from stdin.
#   - Hard block (exit 2):  mm/  and  concurrency/
#   - Warn but allow (exit 1):  flow/  (mechanics are fine; behavioral design is the author's)

payload="$(cat)"

path="$(printf '%s' "$payload" | python3 -c '
import sys, json
try:
    d = json.load(sys.stdin)
except Exception:
    sys.exit(0)
ti = d.get("tool_input", {}) or {}
print(ti.get("file_path") or ti.get("notebook_path") or ti.get("path") or "")
')"

[ -z "$path" ] && exit 0

case "$path" in
  *"/mm/"*|"mm/"*|*"/concurrency/"*|"concurrency/"*)
    echo "BLOCKED: $path is in an author-owned directory (mm/ or concurrency/)." >&2
    echo "These hold the market-making logic and the lock-free / hot-path code the" >&2
    echo "author is learning to write by hand. Do not edit them. Switch to review or" >&2
    echo "tutor mode: explain the approach and the issues, and let the author write it." >&2
    exit 2
    ;;
  *"/flow/"*|"flow/"*)
    echo "NOTE: $path is in flow/. Mechanical plumbing (sampling, formatting, wiring) is" >&2
    echo "fine to help with, but the *behavioral design* of the flow -- directional and" >&2
    echo "informed components, the parameters that create inventory risk and adverse" >&2
    echo "selection -- is the author's to own. Proceed only if this is mechanics." >&2
    exit 1
    ;;
esac

exit 0
