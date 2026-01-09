---
id: tc-3cff
status: open
deps: [tc-55a8]
links: []
created: 2026-01-09T17:06:01Z
type: task
priority: 2
assignee: Ray Myers
---
# Scope out BDD Go

Create individual tickets for porting each BDD feature to Go.

## Features to Port

For each feature in `features/`, create a ticket with:

```bash
./ticket create "Port feature <feature_name> to Go"
```

Add this to each ticket's description:

```
Implement the <feature_name> functionality in Go. 

Add this line to go/bdd.sh:

# Run <feature_name> feature tests
godog features/<feature_name>.feature
```

Then make each new ticket depend on this scoping ticket:

```bash
./ticket dep <new-ticket-id> tc-3cff
```

## Features List (in recommended implementation order):

1. **ticket_creation** - Foundation for creating tickets
2. **ticket_show** - Basic display functionality
3. **ticket_status** - Simple status operations
4. **ticket_listing** - More complex queries
5. **ticket_notes** - Depends on show
6. **ticket_edit** - Depends on show
7. **ticket_dependencies** - Depends on show and listing
8. **ticket_links** - Similar to dependencies
9. **ticket_query** - Depends on listing
10. **id_resolution** - Depends on query

## Tasks

1. Create 10 individual feature port tickets (one per feature)
2. Set dependencies for each ticket to depend on this ticket (tc-3cff)
3. Order tickets logically
4. Document the ticket IDs created

