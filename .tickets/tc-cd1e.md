---
id: tc-cd1e
status: closed
deps: [tc-3cff]
links: []
created: 2026-01-09T17:20:20Z
type: task
priority: 2
assignee: Ray Myers
---
# Port feature ticket_dependencies to Go

Implement the ticket_dependencies functionality in Go. 

Add this line to go/bdd.sh:

```
# Run ticket_dependencies feature tests
godog features/ticket_dependencies.feature
```

## Notes

**2026-01-09T18:04:04Z**

Implemented ticket_dependencies functionality in Go. Added cmdUndep to remove dependencies and cmdDepTree to display dependency tree with cycle detection and box-drawing characters. All 12 BDD scenarios passing.
