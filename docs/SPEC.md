# Ticket System Specification

A minimal ticket tracking system with dependency management. Tickets are stored as markdown files with YAML frontmatter.

## Storage

- Directory: `.tickets/`
- Format: `{id}.md` (markdown with YAML frontmatter)
- Directory created on demand if it doesn't exist

## Ticket ID Generation

1. Extract project prefix from current directory name:
   - Split on `-` or `_`
   - Take first letter of each segment
   - Fallback: first 3 chars if no segments
2. Generate 4-char hash from `sha256(PID + timestamp)`
3. Format: `{prefix}-{hash}` (e.g., `nw-5c46`)

## Ticket Schema

```yaml
---
id: string           # generated, immutable
status: string       # open | in_progress | closed
deps: [string]       # ticket IDs this depends on
links: [string]      # symmetric relationships
created: ISO8601     # creation timestamp
type: string         # task | bug | feature | epic | chore
priority: int        # 0-4 (0=highest), default 2
assignee?: string    # optional, defaults to git user.name
external-ref?: string # optional external reference
parent?: string      # optional parent ticket ID
---
# Title

Description text

## Design
Design notes (optional)

## Acceptance Criteria  
Acceptance criteria (optional)

## Notes
**{timestamp}**
Note content (appended by add-note)
```

## Commands

### create [title] [options]
Create ticket, returns ID.

If no title provided, defaults to "Untitled".

| Option | Description |
|--------|-------------|
| `-d, --description` | Description text |
| `--design` | Design notes |
| `--acceptance` | Acceptance criteria |
| `-t, --type` | Type (default: task) |
| `-p, --priority` | Priority 0-4 (default: 2) |
| `-a, --assignee` | Assignee (default: git user.name) |
| `--external-ref` | External reference |
| `--parent` | Parent ticket ID |

### status \<id\> \<status\>
Set status. Valid values: `open`, `in_progress`, `closed`.

### start \<id\>
Alias for `status <id> in_progress`.

### close \<id\>
Alias for `status <id> closed`.

### reopen \<id\>
Alias for `status <id> open`.

### dep \<id\> \<dep-id\>
Add dependency: `id` depends on `dep-id`. Idempotent (prints "Dependency already exists" if already present).

### dep tree [--full] \<id\>
Display dependency tree rooted at `id`.

- Default: each ticket appears once at its deepest level
- `--full`: show all occurrences (cycles detected and skipped)
- Sort children by subtree depth (shallowest first), then by ID (ascending)
- Output format:
  ```
  root-id [status] Title
  ├── child-id [status] Title
  └── child-id [status] Title
      └── grandchild-id [status] Title
  ```

### undep \<id\> \<dep-id\>
Remove dependency.

### link \<id\> \<id\> [id...]
Create symmetric links between 2+ tickets. Bidirectional and idempotent.

Outputs: "Added N link(s) between M tickets" where N is the count of new link entries added across all files, or "All links already exist" if no changes made.

### unlink \<id\> \<target-id\>
Remove symmetric link between two tickets.

### ls [--status=X]
List all tickets. Optional status filter.

Output: `{id} [{status}] - {title} <- [{deps}]`
- ID is left-padded to 8 characters
- Dependencies section (` <- [{deps}]`) only shown if ticket has dependencies
- Dependency arrays formatted with spaces: `[a, b, c]`

### ready
List tickets where:
- Status is `open` or `in_progress`
- All dependencies are `closed`

Sorted by priority (ascending), then ID (ascending).

Output: `{id} [P{priority}][{status}] - {title}`

### blocked
List tickets where:
- Status is `open` or `in_progress`
- At least one dependency is not `closed`

Shows only unclosed blockers. Sorted by priority (ascending), then ID (ascending).

Output: `{id} [P{priority}][{status}] - {title} <- [{unclosed deps}]`

### closed [--limit=N]
List closed tickets, most recently modified first. Default limit: 20.

Implementation: Examines the 100 most recently modified ticket files, filters for closed status, then applies the limit.

### show \<id\>
Display full ticket with computed sections:

- **Blockers**: unclosed dependencies
- **Blocking**: tickets where this ticket is an unclosed dependency
- **Children**: tickets with `parent: {id}`
- **Linked**: tickets in `links` array

In the output (not the file itself), parent field enhanced with title comment: `parent: abc-1234  # Parent Title`

### edit \<id\>
Open ticket in `$EDITOR` (default: vi). If not a TTY, prints file path to stdout instead of opening editor.

### add-note \<id\> [text]
Append timestamped note. Text from argument or stdin.

Creates `## Notes` section if missing.

Format:
```
**{ISO8601 timestamp}**

{note text}
```

### query [jq-filter]
Output tickets as JSON Lines, one object per ticket.

- Arrays serialized as JSON arrays
- Optional jq filter: `query '.status == "open"'`
- Requires: `jq` (for filtering)

### migrate-beads
Import from `.beads/issues.jsonl` format.

Dependency type mapping:
- `blocks` → `deps`
- `parent-child` → `parent`
- `related` → `links`

Requires: `jq`

## ID Resolution

All commands accepting `<id>` support partial matching:
- Exact match tried first
- Then substring match across all ticket filenames
- Error if ambiguous (multiple matches)
- Error if no match

## YAML Field Operations

### Read field
Extract value between `---` markers, match `^{field}:`, strip prefix.

### Update field
- If exists: replace entire line
- If missing: insert after first `---` line

### Formatting Conventions
- Array fields formatted with spaces after commas: `[a, b, c]`
- Leading/trailing whitespace in field values stripped when reading

## Dependency Cycle Handling

- `dep tree` detects cycles via path tracking
- Cycles silently skipped (no output, no error)
- Path format: `:id1:id2:id3:` for O(n) containment check

## Platform Compatibility

Implementations should handle:
- `sha256sum` (Linux) vs `shasum -a 256` (macOS)
- GNU vs BSD `sed -i` differences
- Date formatting without GNU extensions
- `ripgrep` preferred over `grep` when available
