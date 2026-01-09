"""Tests for the CLI module."""

import sys

from ticket import cli


def test_main_with_help() -> None:
    """Test that main returns 0 with help flag."""
    original_argv = sys.argv
    try:
        sys.argv = ["tk", "--help"]
        result = cli.main()
        assert result == 0
    finally:
        sys.argv = original_argv


def test_main_without_args() -> None:
    """Test that main prints help when called without args."""
    original_argv = sys.argv
    try:
        sys.argv = ["tk"]
        result = cli.main()
        assert result == 0
    finally:
        sys.argv = original_argv
