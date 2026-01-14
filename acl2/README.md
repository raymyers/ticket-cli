# ACL2 Implementation

Ticket-cli implemented in ACL2, the theorem prover.

## Setup

```bash
./install-acl2.sh   # Downloads SBCL + builds ACL2 (~1 min)
```

## Usage

```bash
../acl2_ticket.sh create "My ticket" task 2
../acl2_ticket.sh help
```

## Certify

Verify the code is admitted by ACL2:

```bash
cd ticket
../../acl2/.acl2/saved_acl2 <<< '(certify-book "ticket" ?)'
```

Creates `ticket.cert` proving all functions admitted.

## Code Structure

`ticket/ticket.lisp` contains:

**Logic mode** (pure, provable):
- `char-downcase-simple` - Lowercase a character
- `downcase-chars` - Map downcase over char list
- `string-downcase-simple` - Lowercase a string
- `split-by-char`, `split-string` - Split string by delimiter
- `first-chars` - First char of each string in list
- `extract-prefix` - Get prefix from directory name
- `build-frontmatter` - Build YAML frontmatter
- `join-with-newlines` - Join strings with newlines
- `build-ticket-content` - Build full ticket file content

**Program mode** (I/O):
- `write-ticket-file` - Write string to file
- `get-timestamp` - Get current timestamp
- `ticket-create` - Main create command

## Requirements

- SBCL (installed by `install-acl2.sh`)
- ACL2 8.5 (built by `install-acl2.sh`)
