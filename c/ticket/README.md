# Ticket CLI - C Implementation

A C11 implementation of the ticket-cli system.

## Dependencies

The following libraries are required to build the C implementation:

- **libyaml** - YAML parsing for ticket files
- **json-c** (or jansson) - JSON output for query command
- **libcrypto** (OpenSSL) - SHA256 hashing for ticket ID generation
- **check** or **CUnit** - Testing framework (for tests only)

### Installing Dependencies

#### macOS (Homebrew)
```bash
brew install libyaml json-c openssl check
```

#### Ubuntu/Debian
```bash
sudo apt-get install libyaml-dev libjson-c-dev libssl-dev check
```

#### Fedora/RHEL
```bash
sudo dnf install libyaml-devel json-c-devel openssl-devel check-devel
```

## Building

```bash
# Build the project
make

# Build with debug symbols
make debug

# Build and run tests
make test

# Clean build artifacts
make clean
```

## Running

```bash
# Direct execution
./bin/ticket help

# Using the wrapper script (from project root)
./c_ticket.sh help
```

## Development

### Code Quality Tools

```bash
# Run static analysis
make check

# Check code formatting
make lint

# Format code
make format
```

### Project Structure

```
c/ticket/
├── src/           # Source files (*.c)
├── include/       # Header files (*.h)
├── tests/         # Unit tests
├── bin/           # Compiled binaries (generated)
├── obj/           # Object files (generated)
├── Makefile       # Build configuration
└── README.md      # This file
```

## Compiler Requirements

- GCC or Clang with C11 standard support
- Standard flags: `-std=c11 -Wall -Wextra -Werror -pedantic`

## Status

This is the initial setup for the C implementation. Core functionality is under development.
