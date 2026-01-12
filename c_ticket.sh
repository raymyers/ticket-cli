#!/usr/bin/env bash
# Wrapper script to run the C ticket CLI implementation

set -euo pipefail

# Get the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build the binary if it doesn't exist or if source is newer
BIN_PATH="$SCRIPT_DIR/c/ticket/bin/ticket"
SRC_DIR="$SCRIPT_DIR/c/ticket/src"

if [ ! -f "$BIN_PATH" ] || [ "$SRC_DIR" -nt "$BIN_PATH" ]; then
    (cd "$SCRIPT_DIR/c/ticket" && make) >/dev/null 2>&1
fi

# Run the binary
"$BIN_PATH" "$@"
