#!/bin/bash
set -euo pipefail

function context() {
    echo "# Ready"
    ./ticket ready

    echo
    echo "# Blocked"
    ./ticket blocked

    echo
    echo "# Closed"
    ./ticket closed
}

if command -v bat >/dev/null 2>&1; then
  context | bat -l md --style plain
else
  context
fi
