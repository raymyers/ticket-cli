"""Command-line interface for the ticket system."""

import sys


def main() -> int:
    """Main entry point for the ticket CLI."""
    args = sys.argv[1:]
    
    if not args or args[0] in ("-h", "--help", "help"):
        print_help()
        return 0
    
    print("Ticket CLI - Python port (work in progress)")
    print(f"Command not yet implemented: {args[0]}")
    return 1


def print_help() -> None:
    """Print help message."""
    print("""tk - minimal ticket system with dependency tracking

Usage: tk <command> [args]

Commands:
  create [title] [options] Create ticket, prints ID
    -d, --description      Description text
    --design               Design notes
    --acceptance           Acceptance criteria
    -t, --type             Type (bug|feature|task|epic|chore) [default: task]
    -p, --priority         Priority 0-4, 0=highest [default: 2]
    -a, --assignee         Assignee [default: git user.name]
    --external-ref         External reference (e.g., gh-123, JIRA-456)
    --parent               Parent ticket ID
  start <id>               Set status to in_progress
  close <id>               Set status to closed
  reopen <id>              Set status to open
  status <id> <status>     Update status (open|in_progress|closed)
  dep <id> <dep-id>        Add dependency (id depends on dep-id)
  dep tree [--full] <id>   Show dependency tree (--full disables dedup)
  undep <id> <dep-id>      Remove dependency
  link <id> <id> [id...]   Link tickets together (symmetric)
  unlink <id> <target-id>  Remove link between tickets
  ls [--status=X]          List tickets
  ready                    List open/in-progress tickets with deps resolved
  blocked                  List open/in-progress tickets with unresolved deps
  closed [--limit=N]       List recently closed tickets (default 20, by mtime)
  show <id>                Display ticket
  edit <id>                Open ticket in $EDITOR
  add-note <id> [text]     Append timestamped note (or pipe via stdin)
  query [jq-filter]        Output tickets as JSON, optionally filtered
  migrate-beads            Import tickets from .beads/issues.jsonl

Tickets stored as markdown files in .tickets/
Supports partial ID matching (e.g., 'tk show 5c4' matches 'nw-5c46')
""")


if __name__ == "__main__":
    sys.exit(main())
