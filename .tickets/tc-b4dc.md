---
id: tc-b4dc
status: closed
deps: [tc-9c9c, tc-69a8]
links: []
created: 2026-01-09T13:13:37Z
type: task
priority: 2
assignee: Ray Myers
---
# Port feature ticket_edit to Python


## Notes

**2026-01-09T13:14:05Z**

Implement Python version of ticket_edit feature.

Add this line to python/bdd.sh:

```sh
# export TICKET_SCRIPT=py_ticket.sh
# ...
uv run --with behave behave features/ticket_edit.feature
```

**2026-01-09T13:39:13Z**

Successfully implemented ticket_edit feature in Python. Added cmd_edit() function to cli.py that handles both TTY and non-TTY modes. Added the command to main() dispatcher. Added ticket_edit.feature test to python/bdd.sh. All BDD tests passing.
