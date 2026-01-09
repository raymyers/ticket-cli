#!/usr/bin/env bash
# Wrapper script to run the Go ticket CLI implementation

set -euo pipefail

# Get the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build and run the Go CLI
cd "$SCRIPT_DIR/go/ticket"
go run ./cmd/ticket "$@"
