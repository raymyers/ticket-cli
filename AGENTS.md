# AGENTS.md

See @README.md for usage documentation. Run `tk help` for command reference. Always update the README.md usage content when adding/changing commands and flags.

## Architecture

Single-file bash implementation (~900 lines). Uses awk for performant bulk operations on large ticket sets.

Key functions:
- `generate_id()` - Creates IDs from directory name prefix + timestamp hash
- `ticket_path()` - Resolves partial IDs to full file paths
- `yaml_field()` / `update_yaml_field()` - YAML frontmatter manipulation via sed
- `cmd_*()` - Command handlers
- `cmd_ready()`, `cmd_blocked()`, `cmd_ls()` - awk-based bulk listing with sorting

Dependencies: bash, sed, awk, find. Optional: ripgrep (faster grep), jq (for query command).

## Changelog

When committing notable changes to the `ticket` script (new commands, flags, bug fixes, behavior changes), update CHANGELOG.md in the same commit:
- Create `## [Unreleased]` section at top if it doesn't exist
- Add bullet points under appropriate heading (Added, Fixed, Changed, Removed)
- Only script changes need logging; docs/workflow changes don't
