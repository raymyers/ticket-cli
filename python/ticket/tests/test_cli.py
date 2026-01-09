"""Tests for the CLI module."""

from ticket import cli


def test_main_with_help():
    """Test that main returns 0 with help flag."""
    import sys
    original_argv = sys.argv
    try:
        sys.argv = ["tk", "--help"]
        result = cli.main()
        assert result == 0
    finally:
        sys.argv = original_argv


def test_main_without_args():
    """Test that main prints help when called without args."""
    import sys
    original_argv = sys.argv
    try:
        sys.argv = ["tk"]
        result = cli.main()
        assert result == 0
    finally:
        sys.argv = original_argv
