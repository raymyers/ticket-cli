# Python Implementation Review vs SPEC.md

**Date:** 2026-01-09  
**Reviewer:** AI Assistant  
**Ticket:** tc-32f5

## Executive Summary

The Python implementation (`python/ticket/src/ticket/cli.py`) is largely complete and functional, with **all 40 BDD test scenarios passing**. However, there are **2 significant discrepancies** compared to the specification and bash reference implementation:

1. **Missing command:** `migrate-beads` is not implemented
2. **Incorrect behavior:** `dep tree` does not sort children by subtree depth

## Methodology

- Compared `docs/SPEC.md` (301 lines) with `python/ticket/src/ticket/cli.py` (1142 lines)
- Ran all BDD tests: 40 scenarios passed, 0 failed
- Created test cases for dependency tree sorting behavior
- Cross-referenced with bash reference implementation (`ticket` script)

## Detailed Findings

### 1. MISSING COMMAND: migrate-beads

**Severity:** Medium  
**Location:** SPEC.md lines 230-244  

**Description:**  
The `migrate-beads` command is specified but not implemented in Python.

**Evidence:**
- Bash implementation: ✓ Implemented at line 1127 in `ticket` script
- Python implementation: ✗ Not present in `cli.py`
- Help text: Listed in spec, listed in bash help, missing from Python `main()` dispatcher

**Spec requirement:**
```
### migrate-beads
Import from `.beads/issues.jsonl` format.

Dependency type mapping:
- `blocks` → `deps`
- `parent-child` → `parent`
- `related` → `links`

Output:
- Prints `Migrated: {id}` for each ticket
- Prints `Migrated {count} tickets from beads` at end

Requires: `jq`

Error: `Error: .beads/issues.jsonl not found` if file missing
```

**Impact:**  
Users cannot migrate from the `.beads` format when using the Python version.

**Recommendation:**  
Implement the `migrate-beads` command in Python to achieve feature parity with bash version.

---

### 2. INCORRECT SORTING: dep tree children

**Severity:** High  
**Location:** SPEC.md line 105, cli.py lines 988-991  

**Description:**  
The `dep tree` command does not sort children by subtree depth and ID as specified.

**Spec requirement:**
> Sort children by subtree depth (shallowest first), then by ID (ascending)

**Current Python implementation:**
```python
# Lines 988-991 in cli.py
for i, dep_id in enumerate(deps):
    is_last_dep = (i == len(deps) - 1)
    print_tree(dep_id, new_prefix, is_last_dep, new_path)
```

This simply iterates through dependencies in the order they appear in the `deps` array, without calculating subtree depths or sorting.

**Bash implementation:**
The bash version correctly implements sorting at lines 405-415:
```bash
# Sort by subtree_depth, then by ticket ID (insertion sort)
for (i = 2; i <= n; i++) {
    tmp = arr[i]
    j = i - 1
    while (j >= 1 && (subtree_depth[arr[j]] > subtree_depth[tmp] || \
           (subtree_depth[arr[j]] == subtree_depth[tmp] && arr[j] > tmp))) {
        arr[j + 1] = arr[j]
        j--
    }
    arr[j + 1] = tmp
}
```

**Test Evidence:**

Created test scenario:
- `root` depends on `[b, c]` (in that order)
- `b` has deep subtree: `b → d → e` (subtree depth: 3)
- `c` is a leaf node (subtree depth: 1)

Expected output (shallowest first):
```
root [open] Root Task
├── c [open] Task C
└── b [open] Task B
    └── d [open] Task D
        └── e [open] Task E
```

**Actual outputs:**

Bash implementation (CORRECT):
```
root [open] Root Task
├── c [open] Task C          # ✓ Shallower subtree shown first
└── b [open] Task B
    └── d [open] Task D
        └── e [open] Task E
```

Python implementation (INCORRECT):
```
root [open] Root Task
├── b [open] Task B          # ✗ Just follows array order
│   └── d [open] Task D
│       └── e [open] Task E
└── c [open] Task C
```

**Impact:**  
Users see inconsistent tree ordering compared to bash version. Tree visualization doesn't follow the logical pattern of showing simpler dependencies before complex ones.

**Recommendation:**  
Implement subtree depth calculation and sorting in the Python `cmd_dep_tree` function:

1. Calculate subtree depth for each node (max depth to any descendant)
2. Before printing children, sort them by:
   - Primary: subtree_depth (ascending - shallowest first)
   - Secondary: ticket ID (ascending - alphabetical)

---

## Commands Implementation Status

All commands from SPEC.md:

| Command | Bash | Python | Notes |
|---------|------|--------|-------|
| create | ✓ | ✓ | |
| status | ✓ | ✓ | |
| start | ✓ | ✓ | |
| close | ✓ | ✓ | |
| reopen | ✓ | ✓ | |
| dep | ✓ | ✓ | |
| dep tree | ✓ | ⚠️ | Sorting incorrect |
| undep | ✓ | ✓ | |
| link | ✓ | ✓ | |
| unlink | ✓ | ✓ | |
| ls | ✓ | ✓ | |
| ready | ✓ | ✓ | |
| blocked | ✓ | ✓ | |
| closed | ✓ | ✓ | |
| show | ✓ | ✓ | |
| edit | ✓ | ✓ | |
| add-note | ✓ | ✓ | |
| query | ✓ | ✓ | |
| migrate-beads | ✓ | ✗ | Not implemented |

**Summary:** 18/19 commands implemented, 17/19 fully correct

---

## Minor Observations (Non-Issues)

### Cycle Detection Implementation Difference

**Spec suggests (line 280):**
> Path format: `:id1:id2:id3:` for O(n) containment check

**Python uses:**
```python
path = set()  # O(1) containment check
if ticket_id in path:
    return
```

**Assessment:** Not a problem. Python's set-based approach is functionally equivalent and actually more efficient (O(1) vs O(n) for containment checks).

### Overall Code Quality

The Python implementation demonstrates:
- ✓ Clean, readable code structure
- ✓ Proper error handling
- ✓ Correct YAML field operations
- ✓ Accurate error messages matching spec
- ✓ Proper ID resolution (exact match, then partial match)
- ✓ Correct box-drawing characters for tree visualization
- ✓ All data formats match spec (JSONL, array formatting, etc.)

---

## Testing Coverage

### BDD Test Results
- **Total scenarios:** 40
- **Passed:** 40
- **Failed:** 0

### Test Coverage Gaps

The BDD tests don't verify:
1. `migrate-beads` command (no tests exist)
2. `dep tree` sorting order (tests only check presence, not order)

Example from `ticket_dependencies.feature`:
```gherkin
Scenario: Dependency tree with multiple children
  # ...
  Then the output should contain "task-0002"
  And the output should contain "task-0003"
```

This test passes as long as both tickets appear, regardless of order.

**Recommendation:**  
Add BDD scenarios that verify:
- Specific ordering in `dep tree` output
- `migrate-beads` functionality (if/when implemented)

---

## Recommendations Priority

### High Priority
1. **Implement subtree depth sorting in `dep tree`**
   - Required for spec compliance
   - Affects user experience and consistency with bash version
   - Algorithm available in bash reference implementation

### Medium Priority
2. **Implement `migrate-beads` command**
   - Required for feature parity
   - May be less critical if migration is one-time operation
   - Consider if this feature is still needed

### Low Priority
3. **Add BDD tests for ordering**
   - Would catch regressions in future
   - Could use the test case from this review

---

## Conclusion

The Python implementation is **largely complete and well-implemented**, with excellent test coverage for implemented features. The two identified issues are:

1. A missing feature (`migrate-beads`)
2. Incorrect sorting behavior (`dep tree`)

Both issues have clear remediation paths. Once addressed, the Python implementation will fully match the specification and bash reference implementation.
