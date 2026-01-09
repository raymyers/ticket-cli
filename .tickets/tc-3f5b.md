---
id: tc-3f5b
status: open
deps: [tc-3cff]
links: []
created: 2026-01-09T17:20:14Z
type: task
priority: 2
assignee: Ray Myers
---
# Port feature ticket_status to Go

Implement the ticket_status functionality in Go. 

Add this line to go/bdd.sh:

```
# Run ticket_status feature tests
godog features/ticket_status.feature
```
