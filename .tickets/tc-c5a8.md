---
id: tc-c5a8
status: closed
deps: [tc-3cff]
links: []
created: 2026-01-09T17:20:20Z
type: task
priority: 2
assignee: Ray Myers
---
# Port feature id_resolution to Go

Implement the id_resolution functionality in Go. 

Add this line to go/bdd.sh:

```
# Run id_resolution feature tests
godog features/id_resolution.feature
```

## Notes

**2026-01-09T17:59:41Z**

Implemented dep and link commands in Go with full ID resolution support. The resolveTicketID function was already in place for other commands (show, status, edit, add-note), so the implementation mainly required adding cmdDep and cmdLink functions. All 10 id_resolution feature scenarios now pass.
