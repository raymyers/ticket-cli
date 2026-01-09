---
id: tc-ba73
status: open
deps: []
links: []
created: 2026-01-09T19:58:06Z
type: task
priority: 2
assignee: Ray Myers
---
# Scope out BDD Zig

Create individual tickets for each BDD feature to be ported to Zig.

**For each feature in features/, create a ticket with:**

```bash
./ticket create "Port feature <feature_name> to Zig"
```

**Add this to the ticket description:**

"Implement the [feature_name] functionality. Add this line to zig/bdd.sh:

```bash
# Run [feature_name] feature tests
behave features/<feature_name>.feature
```

**Then make each new ticket depend on this scoping ticket:**

```bash
./ticket dep <new-ticket-id> tc-ba73
```

**Features to port:**
- ticket_creation
- ticket_show
- ticket_status
- ticket_listing
- ticket_notes
- ticket_edit
- ticket_dependencies
- ticket_links
- ticket_query
- id_resolution

**Recommended implementation order:**
1. ticket_creation (foundation)
2. ticket_show (basic display)
3. ticket_status (simple operations)
4. ticket_listing (more complex queries)
5. ticket_notes (depends on show)
6. ticket_edit (depends on show)
7. ticket_dependencies (depends on show and listing)
8. ticket_links (similar to dependencies)
9. ticket_query (depends on listing)
10. id_resolution (depends on query)

**Note**: The zig/bdd.sh script should be created with this structure:

```bash
#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export TICKET_SCRIPT="${PROJECT_ROOT}/zig_ticket.sh"

# Features will be added here as they are implemented
```

