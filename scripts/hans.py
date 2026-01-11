#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = [
#   "openhands-sdk==1.7.4",
#   "openhands-tools==1.7.4"
# ]
# ///
import os
import subprocess

from openhands.sdk import LLM, Agent, Conversation, Event, Tool
from openhands.sdk.event import MessageEvent, SystemPromptEvent, ActionEvent, ObservationEvent
from openhands.sdk.conversation.visualizer import ConversationVisualizerBase
from openhands.tools.file_editor import FileEditorTool
from openhands.tools.task_tracker import TaskTrackerTool
from openhands.tools.terminal import TerminalTool

print("🚀 Starting Hans")

MAX_ITERATIONS=10

def truncate(s: str, max_len: int) -> str:
    """Truncate string with ellipsis if needed."""
    if len(s) <= max_len:
        return s
    return s[:max_len - 3] + "..."

def format_system_prompt(data: dict) -> str:
    """SystemPromptEvent: Show source and prompt snippet."""
    source = data.get("source", "?")
    prompt = data.get("system_prompt", {})
    text = prompt.get("text", "") if isinstance(prompt, dict) else str(prompt)
    text = text.replace("\n", " ")
    return f"SysPrompt src={source} {truncate(text, 60)}"

def format_message(data: dict) -> str:
    """MessageEvent: Show role and message content snippet."""
    msg = data.get("llm_message", {})
    role = msg.get("role", "?")
    content = msg.get("content", [])
    text = ""
    if isinstance(content, list) and content:
        first = content[0]
        if isinstance(first, dict):
            text = first.get("text", "")
        else:
            text = str(first)
    text = text.replace("\n", " ")
    return f"Message {role} {truncate(text, 70)}"

def format_action(data: dict) -> str:
    """ActionEvent: Show tool name and action details."""
    tool = data.get("tool_name", "?")
    action = data.get("action", {})

    if tool == "file_editor":
        cmd = action.get("command", "?")
        path = action.get("path", "")
        path = path.split("/")[-1] if path else "?"
        return f"Action {tool}:{cmd} → {path}"
    elif tool == "terminal":
        cmd = action.get("command", "").replace("\n", " ")
        return f"Action terminal {truncate(cmd, 75)}"
    else:
        thought = data.get("thought", [])
        thought_text = ""
        if thought and isinstance(thought[0], dict):
            thought_text = thought[0].get("text", "")
        thought_text = thought_text.replace("\n", " ")
        return f"Action {tool} {truncate(thought_text, 70)}"

def format_observation(data: dict) -> str:
    """ObservationEvent: Show tool name and result snippet."""
    tool = data.get("tool_name", "?")
    obs = data.get("observation", {})

    is_error = obs.get("is_error", False)

    content = obs.get("content", [])
    text = ""
    if isinstance(content, list) and content:
        first = content[0]
        if isinstance(first, dict):
            text = first.get("text", "")
        else:
            text = str(first)
    text = text.replace("\n", " ")

    if is_error:
        return f"Obs {tool} [ERR] {truncate(text, 65)}"
    else:
        return f"Obs {tool} {truncate(text, 70)}"

def format_unknown(event_type: str, data: dict) -> str:
    """Unknown event type: Show type and first few keys."""
    keys = list(data.keys())[:5]
    return f"{event_type} keys={keys}"

class MinimalVisualizer(ConversationVisualizerBase):
    """A minimal visualizer that print the raw events as they occur."""

    def on_event(self, event: Event) -> None:
        """Handle events for minimal progress visualization."""
        if isinstance(event, MessageEvent):
            print(format_message(event.model_dump()))
        elif isinstance(event, SystemPromptEvent):
            print(format_system_prompt(event.model_dump()))
        elif isinstance(event, ActionEvent):
            print(format_action(event.model_dump()))
        elif isinstance(event, ObservationEvent):
            print(format_observation(event.model_dump()))
        else:
            print(format_unknown(type(event).__name__, event.model_dump()))
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
