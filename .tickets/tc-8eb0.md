---
id: tc-8eb0
status: closed
deps: []
links: []
created: 2026-01-09T16:53:24Z
type: task
priority: 2
assignee: Ray Myers
---
# Go implementation

Use these instructions to schedule a go implementation of ticket CLI:

docs/NEW_IMPLEMENTATION.md

## Notes

**2026-01-09T17:08:03Z**

Created 6 tracking tickets for Go implementation:

1. tc-fd16: Setup project for Go port
2. tc-55a8: Lint / checks in Go port (depends on tc-fd16)
3. tc-3cff: Scope out BDD Go (depends on tc-55a8)
4. tc-7456: Add full BDD suite to Go build (depends on tc-3cff)
5. tc-26e7: Manual smoke test of Go port (depends on tc-7456)
6. tc-837f: Spec review vs Go implementation (depends on tc-7456, tc-26e7)

Dependencies form a chain: tc-fd16 -> tc-55a8 -> tc-3cff -> tc-7456 -> [tc-26e7, tc-837f]

Next step: Execute tc-fd16 to begin Go port implementation.
