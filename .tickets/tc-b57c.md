---
id: tc-b57c
status: open
deps: []
links: []
created: 2026-01-10T00:54:50Z
type: task
priority: 3
assignee: Ray Myers
---
# Add full BDD suite to TypeScript build

Integrate the complete BDD test suite into the TypeScript build process.

**Tasks**:
- Update `typescript/bdd.sh` to run all feature tests
- Add `typescript-bdd` target to root Makefile
- Ensure BDD suite can be run as part of CI/CD

**typescript/bdd.sh structure**:
```bash
#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export TICKET_SCRIPT="${PROJECT_ROOT}/ts_ticket.sh"

# Run each feature
bun test "${PROJECT_ROOT}/features/ticket_creation.feature"
bun test "${PROJECT_ROOT}/features/ticket_show.feature"
bun test "${PROJECT_ROOT}/features/ticket_status.feature"
bun test "${PROJECT_ROOT}/features/ticket_listing.feature"
bun test "${PROJECT_ROOT}/features/ticket_notes.feature"
bun test "${PROJECT_ROOT}/features/ticket_edit.feature"
bun test "${PROJECT_ROOT}/features/ticket_dependencies.feature"
bun test "${PROJECT_ROOT}/features/ticket_links.feature"
bun test "${PROJECT_ROOT}/features/ticket_query.feature"
bun test "${PROJECT_ROOT}/features/id_resolution.feature"
```

