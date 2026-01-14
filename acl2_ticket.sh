#!/usr/bin/env bash
# Wrapper script to run the ACL2/Common Lisp ticket CLI implementation
# Requires SBCL (Steel Bank Common Lisp)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ACL2_DIR="$SCRIPT_DIR/acl2/ticket"

# Check if SBCL is installed
if ! command -v sbcl &> /dev/null; then
    echo "Error: sbcl not found. Install with: apt-get install sbcl" >&2
    exit 1
fi

# Build the argument list for Lisp
LISP_ARGS=""
for arg in "$@"; do
    escaped_arg="${arg//\\/\\\\}"
    escaped_arg="${escaped_arg//\"/\\\"}"
    LISP_ARGS="$LISP_ARGS \"$escaped_arg\""
done

exec sbcl --noinform --non-interactive \
    --load "$ACL2_DIR/src/package.lisp" \
    --load "$ACL2_DIR/src/utils.lisp" \
    --load "$ACL2_DIR/src/create.lisp" \
    --load "$ACL2_DIR/src/cli.lisp" \
    --eval "(sb-ext:exit :code (ticket:main (list $LISP_ARGS)))"
