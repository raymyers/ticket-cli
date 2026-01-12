#!/usr/bin/env bash
# Run BDD tests with C implementation

set -uo pipefail

# Get the project root directory (parent of c/)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Build the C implementation first
echo "Building C implementation..."
cd "$PROJECT_ROOT/c/ticket" && make >/dev/null 2>&1

# Export the C ticket script for the tests
export TICKET_SCRIPT="${PROJECT_ROOT}/c_ticket.sh"

# Run each feature test in logical order (basic to complex)
# Capture exit codes but continue running all tests
EXIT_CODE=0

uv run --with behave behave "${PROJECT_ROOT}/features/ticket_creation.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_show.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_status.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_listing.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_notes.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_edit.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_dependencies.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_links.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/ticket_query.feature" || EXIT_CODE=$?
uv run --with behave behave "${PROJECT_ROOT}/features/id_resolution.feature" || EXIT_CODE=$?

exit $EXIT_CODE
