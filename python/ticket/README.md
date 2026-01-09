# Ticket - Python Port

Python port of the git-backed issue tracker for AI agents.

## Installation

```bash
# Install with uv
uv pip install -e .

# Or install in development mode
uv sync
```

## Usage

```bash
tk --help
```

## Development

This project uses `uv` for dependency management.

```bash
# Install dependencies
uv sync

# Run tests
uv run pytest

# Format code
uv run ruff format

# Lint code
uv run ruff check
```

## License

MIT
