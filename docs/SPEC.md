# Ticket System Specification

A minimal ticket tracking system with dependency management. Tickets are stored as markdown files with YAML frontmatter.

## Storage

- Directory: `.tickets/`
- Format: `{id}.md` (markdown with YAML frontmatter)
- Directory created on demand if it doesn't exist

## Ticket ID Generation

1. Extract project prefix from current directory name:
   - Split directory name on `-` or `_` into segments
   - Take first letter of each segment
   - Fallback: first 3 chars of directory name if no segments found (empty segments ignored)
   - If result is empty string, use first 3 chars of directory name (may be <3 chars if directory name is short)
2. Generate 4-char hash from `sha256(concat(PID, timestamp))`
   - PID and timestamp (seconds since epoch) concatenated without separator: `"$PID$timestamp"`
   - Example: PID 12345, timestamp 1640000000 → hash of "123451640000000"
   - Take first 4 characters of hex digest
3. Format: `{prefix}-{hash}` (e.g., `nw-5c46`)

## Ticket Schema

```yaml
---
id: string           # generated, immutable
status: string       # open | in_progress | closed (always present, default: open)
deps: [string]       # ticket IDs this depends on (always present, default: [])
links: [string]      # symmetric relationships (always present, default: [])
created: string      # creation timestamp (YYYY-MM-DDTHH:MM:SSZ format, UTC)
type: string         # task | bug | feature | epic | chore (always present, default: task)
priority: int        # 0-4 (0=highest) (always present, default: 2)
assignee?: string    # optional, defaults to git user.name if available
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

Timestamps formatted as YYYY-MM-DDTHH:MM:SSZ (UTC)
```

## Commands

### create [title] [options]
Create ticket, prints ID to stdout.

If no title provided, defaults to "Untitled".

Output: `{generated-id}`

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

Output: `Updated {id} -> {status}`

### start \<id\>
Alias for `status <id> in_progress`.

### close \<id\>
Alias for `status <id> closed`.

### reopen \<id\>
Alias for `status <id> open`.

### dep \<id\> \<dep-id\>
Add dependency: `id` depends on `dep-id`. Idempotent.

Output:
- `Added dependency: {id} -> {dep-id}` - if added
- `Dependency already exists` - if already present

Verifies that both tickets exist before adding.

### dep tree [--full] \<id\>
Display dependency tree rooted at `id`.

- Default: each ticket appears once at its deepest level
- `--full`: show all occurrences (cycles detected and skipped)
- Sort children by subtree depth (shallowest first), then by ID (ascending)
- Output format uses box-drawing characters:
  ```
  root-id [status] Title
  ├── child-id [status] Title
  └── child-id [status] Title
      └── grandchild-id [status] Title
  ```
  - `├── ` for non-last children
  - `└── ` for last child
  - `│   ` for continuation line (under non-last children)
  - `    ` (4 spaces) for continuation line (under last child)

### undep \<id\> \<dep-id\>
Remove dependency.

Output:
- `Removed dependency: {id} -/-> {dep-id}` - if removed
- `Dependency not found` - if not present (exits with error)

### link \<id\> \<id\> [id...]
Create symmetric links between 2+ tickets. Bidirectional and idempotent.

Each ticket's `links` array is updated to include all other tickets in the command.

Outputs:
- "Added N link(s) between M tickets" - N is count of new array entries added across all files (bidirectional, so linking 2 tickets adds 2 entries)
- "All links already exist" - if no changes made

Example: `link A B C` creates links A↔B, A↔C, B↔C (6 total array entries if none existed)

### unlink \<id\> \<target-id\>
Remove symmetric link between two tickets.

Output:
- `Removed link: {id} <-> {target-id}` - if removed
- `Link not found` - if not present (exits with error)

Removes link from both tickets' `links` arrays.

### ls [--status=X]
List all tickets. Optional status filter.

Output format: `{id} [{status}] - {title} <- [{deps}]`
- ID is left-aligned in an 8-character field (padding on the right)
- Dependencies section (` <- [{deps}]`) only shown if ticket has dependencies
- Dependency arrays formatted with spaces: `[a, b, c]`
- No output if no tickets match (or .tickets directory doesn't exist)

### ready
List tickets where:
- Status is `open` or `in_progress`
- All dependencies are `closed`

Sorted by priority (ascending), then ID (ascending).

Output format: `{id} [P{priority}][{status}] - {title}`
- ID is left-aligned in an 8-character field (padding on the right)
- No output if no tickets match

### blocked
List tickets where:
- Status is `open` or `in_progress`
- At least one dependency is not `closed`

Shows only unclosed blockers. Sorted by priority (ascending), then ID (ascending).

Output format: `{id} [P{priority}][{status}] - {title} <- [{unclosed deps}]`
- ID is left-aligned in an 8-character field (padding on the right)
- Dependency arrays formatted with spaces: `[a, b]`
- No output if no tickets match

### closed [--limit=N]
List closed tickets, most recently modified first. Default limit: 20.

Implementation: Examines the 100 most recently modified ticket files, filters for status `closed` or `done`, then applies the limit.

Output format: `{id} [{status}] - {title}`
- ID is left-aligned in an 8-character field (padding on the right)

### show \<id\>
Display full ticket with computed sections appended:

- **Blockers**: unclosed dependencies (only shown if any exist)
- **Blocking**: tickets where this ticket is an unclosed dependency (only shown if any exist)
- **Children**: tickets with `parent: {id}` (only shown if any exist)
- **Linked**: tickets in `links` array (only shown if any exist)

Format for computed sections:
```
## {Section Name}

- {id} [{status}] {title}
- {id} [{status}] {title}
```

In the output (not the file itself), parent field enhanced with title comment: `parent: abc-1234  # Parent Title` (if parent exists)

### edit \<id\>
Open ticket in `$EDITOR` (default: vi).

If not a TTY (non-interactive), prints: `Edit ticket file: {full-path}` to stdout instead of opening editor.

### add-note \<id\> [text]
Append timestamped note. Text from argument or stdin (if no argument and stdin not a TTY).

If `## Notes` section doesn't exist, appends `\n## Notes\n` to file first.

Then appends note in format:
```
\n**{YYYY-MM-DDTHH:MM:SSZ timestamp}**\n\n{note text}\n
```

Output: `Note added to {id}`

Error if no text provided: `Error: no note provided`

### query [jq-filter]
Output tickets as JSON Lines (JSONL), one object per ticket.

- Arrays serialized as JSON arrays
- Optional jq filter: `query '.status == "open"'`
- Without filter: outputs all tickets as JSONL (no jq required)
- With filter: requires `jq` installed

### migrate-beads
Import from `.beads/issues.jsonl` format.

Dependency type mapping:
- `blocks` → `deps`
- `parent-child` → `parent`
- `related` → `links`

Output:
- Prints `Migrated: {id}` for each ticket
- Prints `Migrated {count} tickets from beads` at end

Requires: `jq`

Error: `Error: .beads/issues.jsonl not found` if file missing

## ID Resolution

All commands accepting `<id>` support partial matching:
- Exact match tried first: `.tickets/{id}.md`
- Then substring match across ticket filenames: `.tickets/*{id}*.md`
- Error if ambiguous (multiple matches): "Error: ambiguous ID '{id}' matches multiple tickets"
- Error if no match: "Error: ticket '{id}' not found"

Example: `abc` matches `.tickets/abc-1234.md`, and `234` also matches it.

## YAML Field Operations

### Read field
Extract value between `---` markers:
1. Find lines between `---` delimiters
2. Match line starting with `^{field}:`
3. Strip `{field}: ` prefix (note: space after colon included in prefix)
4. Strip leading/trailing whitespace from value

Example: `status:   open  ` → value is `open`

### Update field
- If exists: replace entire line with `{field}: {value}`
- If missing: insert `{field}: {value}` after first `---` line

### Formatting Conventions
- Array fields formatted with spaces after commas: `[a, b, c]`
- Empty arrays: `[]`
- Field values with colons may cause parsing issues (not officially supported)

## Dependency Cycle Handling

- `dep tree` detects cycles via path tracking
- Cycles silently skipped (no output, no error)
- Path format: `:id1:id2:id3:` for O(n) containment check

## Error Handling

Commands exit with status 0 on success, non-zero on failure.

Common error messages:
- `"Error: ticket '{id}' not found"` - ID resolution failed
- `"Error: ambiguous ID '{id}' matches multiple tickets"` - Multiple partial matches
- `"Error: invalid status '{status}'. Must be one of: open in_progress closed"` - Invalid status
- `"Usage: ..."` - Insufficient arguments

Commands that verify dependencies or links will exit with error if referenced tickets don't exist.

## Platform Compatibility

Implementations should handle:
- `sha256sum` (Linux) vs `shasum -a 256` (macOS)
- GNU vs BSD `sed -i` differences
- Date formatting without GNU extensions
- `ripgrep` preferred over `grep` when available
