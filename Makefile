.PHONY: python python-lint python-format python-type python-test python-check

PYTHON_DIR := python/ticket

python: python-check

python-check: python-lint python-type python-test

python-lint:
	@echo "Running ruff linter..."
	cd $(PYTHON_DIR) && uv run ruff check src tests

python-format:
	@echo "Running ruff formatter..."
	cd $(PYTHON_DIR) && uv run ruff format src tests

python-format-check:
	@echo "Checking ruff formatting..."
	cd $(PYTHON_DIR) && uv run ruff format --check src tests

python-type:
	@echo "Running mypy type checker..."
	cd $(PYTHON_DIR) && uv run mypy src tests

python-test:
	@echo "Running pytest..."
	cd $(PYTHON_DIR) && uv run pytest
