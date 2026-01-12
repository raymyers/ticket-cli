---
id: tc-abbd
status: closed
deps: []
links: []
created: 2026-01-12T03:32:52Z
type: task
priority: 2
assignee: Ray Myers
---
# Port feature ticket_listing to C

Implement ticket_listing functionality in C. Add to c/bdd.sh: behave features/ticket_listing.feature


## Notes

**2026-01-12T04:18:19Z**

Implemented ticket listing functionality in C including: cmd_ls, cmd_ready, cmd_blocked, cmd_closed. Added priority field to Ticket struct and parsing. All 18 scenarios in ticket_listing.feature are now passing.
