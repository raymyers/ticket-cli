---
id: tc-6df3
status: open
deps: []
links: []
created: 2026-01-10T00:55:07Z
type: task
priority: 3
assignee: Ray Myers
---
# Manual smoke test of TypeScript port

Perform manual validation of the TypeScript implementation in a real-world usage scenario.

**Test procedure**:
1. Create a fresh temporary directory
2. Initialize a new ticket repository: `./ts_ticket.sh init`
3. Test basic workflows:
   - Create several tickets with different priorities
   - View individual tickets
   - Update ticket status (open → in_progress → done)
   - Add notes to tickets
   - Create dependencies between tickets
   - Add links to tickets
   - List tickets with various filters
   - Query tickets by status, priority, etc.
4. Verify edge cases:
   - Invalid ticket IDs
   - Circular dependencies
   - Empty repositories
5. Check CLI usability:
   - Help messages are clear
   - Error messages are helpful
   - Command syntax is intuitive

Document any issues found and verify fixes.

