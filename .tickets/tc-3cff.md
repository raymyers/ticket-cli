---
id: tc-3cff
status: closed
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


## Notes

**2026-01-09T17:21:22Z**

Created 10 tickets for porting BDD features to Go:
1. tc-00c9: ticket_creation
2. tc-35f6: ticket_show
3. tc-3f5b: ticket_status
4. tc-5d93: ticket_listing
5. tc-32f8: ticket_notes
6. tc-5668: ticket_edit
7. tc-cd1e: ticket_dependencies
8. tc-eff4: ticket_links
9. tc-178a: ticket_query
10. tc-c5a8: id_resolution

All tickets have descriptions and dependencies set on tc-3cff.
