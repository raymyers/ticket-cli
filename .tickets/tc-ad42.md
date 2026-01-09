---
id: tc-ad42
status: open
deps: []
links: []
created: 2026-01-09T11:40:20Z
type: task
priority: 2
assignee: Ray Myers
---
# Feature: Specify scripts to run tests with

In features/steps/ticket_steps.py, it hard codes the bash implementation "./ticket".

Let's make that the default and accept and env var TICKET_SCRIPT to override.

This will allow us to use the same spec against multiple implementations.
