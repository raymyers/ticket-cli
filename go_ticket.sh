#!/usr/bin/env bash
# Wrapper script to run the Go ticket CLI implementation

set -euo pipefail

# Get the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build the binary if it doesn't exist or if source is newer
BIN_PATH="$SCRIPT_DIR/go/ticket/bin/ticket"
if [ ! -f "$BIN_PATH" ] || [ "$SCRIPT_DIR/go/ticket" -nt "$BIN_PATH" ]; then
    (cd "$SCRIPT_DIR/go/ticket" && go build -o bin/ticket ./cmd/ticket) >/dev/null 2>&1
fi

# Run the binary
"$BIN_PATH" "$@"
