#!/usr/bin/env bash
# SessionEnd hook: append a dated learning-journal stub so reflection becomes a
# habit. Non-interactive by design (works in any session); fill it in afterwards.
set -e
root="${CLAUDE_PROJECT_DIR:-$PWD}"
log="$root/journal/learning-log.md"
mkdir -p "$root/journal"
[ -f "$log" ] || printf '# Learning log\n\n' > "$log"
{
  printf '## %s\n\n' "$(date '+%Y-%m-%d %H:%M')"
  printf -- '- **Built:** \n'
  printf -- '- **Learned:** \n'
  printf -- '- **Still fuzzy:** \n'
  printf -- '- **Next:** \n\n'
} >> "$log"
