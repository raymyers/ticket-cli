#!/usr/bin/env bash
# BDD tests for C implementation

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Build the C implementation first
echo "Building C implementation..."
cd c/ticket && make >/dev/null 2>&1
cd "$PROJECT_ROOT"

# Run BDD tests using the C wrapper script
export TICKET_CMD="$PROJECT_ROOT/c_ticket.sh"

echo "Running BDD tests with C implementation..."
cd features
behave ticket_creation.feature
behave id_resolution.feature
behave ticket_links.feature
behave ticket_notes.feature
behave ticket_dependencies.feature
behave ticket_query.feature
behave ticket_listing.feature
behave ticket_show.feature
behave ticket_edit.feature
