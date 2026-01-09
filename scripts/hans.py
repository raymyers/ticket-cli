#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = [
#   "openhands-sdk==1.7.4",
#   "openhands-tools==1.7.4"
# ]
# ///
import os
import json
import subprocess

from openhands.sdk import LLM, Agent, Conversation, Event, MessageEvent, Tool
from openhands.sdk.conversation.visualizer import ConversationVisualizerBase
from openhands.tools.file_editor import FileEditorTool
from openhands.tools.task_tracker import TaskTrackerTool
from openhands.tools.terminal import TerminalTool

print("🚀 Starting Hans")

MAX_ITERATIONS=10

class MinimalVisualizer(ConversationVisualizerBase):
    """A minimal visualizer that print the raw events as they occur."""

    def on_event(self, event: Event) -> None:
        """Handle events for minimal progress visualization."""
        if isinstance(event, MessageEvent):
            print(f"\n\n[EVENT] {str(event)[:200]}...")
        else:
            print(f"\n\n[EVENT] {type(event).__name__}: {event.model_dump_json()[:200]}...")


def run_agent(llm: LLM, prompt: str):
    agent = Agent(
        llm=llm,
        tools=[
            Tool(name=TerminalTool.name),
            Tool(name=FileEditorTool.name),
            Tool(name=TaskTrackerTool.name),
        ]
    )

    cwd = os.getcwd()
    conversation = Conversation(
        agent=agent,
        workspace=cwd,
        visualizer=MinimalVisualizer(),
    )

    conversation.send_message(prompt)
    conversation.run()


llm = LLM(
    model=os.getenv("LLM_MODEL", "anthropic/claude-sonnet-4-5-20250929"),
    api_key=os.getenv("LLM_API_KEY"),
    base_url=os.getenv("LLM_BASE_URL", None),
)

def next_ticket() -> str:
    """Simple git checkout with minimal error handling."""
    result = subprocess.run(['./ticket', 'ready'], check=True, capture_output=True)
    lines = result.stdout.splitlines()
    if not lines:
        return ""
    return str(lines[0], "utf-8").split()[0]

for i in range(MAX_ITERATIONS):
    ticket_id = next_ticket()
    if not ticket_id:
        print("No work on the hopper!")
        break
    ticket_file = f'.tickets/{ticket_id}.md'
    print(f"═══ Iteration {i+1} ═══")
    prompt = f"""
    * Do only the task in `{ticket_file}`.
    * Record your learnings with `./ticket add-note "..."`
    * If complete use `./ticket close {ticket_id}` to mark it complete
    * Commit progress if tests pass
    * Commit .tickets updates if needed
    """
    run_agent(llm=llm, prompt=prompt)
