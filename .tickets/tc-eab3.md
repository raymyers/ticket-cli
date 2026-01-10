---
id: tc-eab3
status: open
deps: []
links: []
created: 2026-01-10T00:54:33Z
type: task
priority: 2
assignee: Ray Myers
---
# Scope out BDD TypeScript

Create tickets for porting each BDD feature to TypeScript.

For each feature in features/, create a ticket with priority 2:

```bash
./ticket create "Port feature <feature_name> to TypeScript" --priority 2
```

Add to each ticket description:

```
Implement the <feature_name> functionality. Add this line to typescript/bdd.sh:

# Run <feature_name> feature tests
bun test features/<feature_name>.feature
```

**Features to port**:
1. ticket_creation (foundation)
2. ticket_show (basic display)
3. ticket_status (simple operations)
4. ticket_listing (more complex queries)
5. ticket_notes (depends on show)
6. ticket_edit (depends on show)
7. ticket_dependencies (depends on show and listing)
8. ticket_links (similar to dependencies)
9. ticket_query (depends on listing)
10. id_resolution (depends on query)

