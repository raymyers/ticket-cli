#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = [
#   "openhands-sdk==1.7.4",
#   "openhands-tools==1.7.4",
#   "rich>=13.7.0"
# ]
# ///
import os
import subprocess
import argparse
import readline  # noqa: F401

from rich.console import Console
from rich.text import Text

from openhands.sdk import LLM, Agent, Conversation, Event, Tool
from openhands.sdk.event import MessageEvent, SystemPromptEvent, ActionEvent, ObservationEvent
from openhands.sdk.conversation.visualizer import ConversationVisualizerBase
from openhands.tools.file_editor import FileEditorTool
from openhands.tools.task_tracker import TaskTrackerTool
from openhands.tools.terminal import TerminalTool
from openhands.tools.glob import GlobTool
from openhands.tools.grep import GrepTool
from openhands.tools.browser_use import BrowserToolSet

console = Console()

# Color scheme: "Calm Ops" - professional, low fatigue
# Blue = intent, Green = result, Purple = artifact
LABEL_ACTION = "bright_cyan"       # #38BDF8 sky-400 - intent/actions
LABEL_OBS = "bright_green"         # #22C55E green-500 - results
LABEL_MESSAGE = "bright_cyan"      # #38BDF8 sky-400 - messages
LABEL_SYSPROMPT = "magenta"        # System prompts
TOOL = "bright_magenta"            # #A78BFA violet-400 - tool names
COMMAND = "bright_magenta"         # #A78BFA violet-400 - commands
PATH = "bright_magenta"            # #A78BFA violet-400 - file paths/artifacts
ERROR = "bright_red"               # #F87171 red-400 - errors
WARNING = "yellow"                 # #FBBF24 amber-400 - warnings
BRACKET = "dim"                    # De-emphasized structure
SECONDARY = "dim"                  # #94A3B8 slate-400 - secondary text

def truncate(s: str, max_len: int) -> str:
    """Truncate string with ellipsis if needed."""
    if len(s) <= max_len:
        return s
    return s[:max_len - 3] + "..."

def format_system_prompt(data: dict) -> Text:
    """SystemPromptEvent: Show source and prompt snippet."""
    source = data.get("source", "?")
    prompt = data.get("system_prompt", {})
    text = prompt.get("text", "") if isinstance(prompt, dict) else str(prompt)
    text = text.replace("\n", " ")

    result = Text()
    result.append("[", style=BRACKET)
    result.append("SysPrompt", style=LABEL_SYSPROMPT)
    result.append("]", style=BRACKET)
    result.append("  ", style=BRACKET)
    result.append(source, style=TOOL)
    result.append("  ", style=SECONDARY)
    result.append(truncate(text, 60), style=SECONDARY)
    return result

def format_message(data: dict, truncate_text: bool=True) -> Text:
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

    result = Text()
    result.append("[", style=BRACKET)
    result.append("Message  ", style=LABEL_MESSAGE)  # Padded to 9 chars
    result.append("]", style=BRACKET)
    result.append("  ", style=BRACKET)
    result.append(role, style=TOOL)
    result.append("  ", style=SECONDARY)

    if truncate_text:
        text = text.replace("\n", " ")
        result.append(truncate(text, 70), style=SECONDARY)
    else:
        result.append(text, style=SECONDARY)

    return result

def format_action(data: dict) -> Text:
    """ActionEvent: Show tool name and action details."""
    tool = data.get("tool_name", "?")
    action = data.get("action", {})

    result = Text()
    result.append("[", style=BRACKET)
    result.append("Action   ", style=LABEL_ACTION)  # Padded to 9 chars
    result.append("]", style=BRACKET)
    result.append("  ", style=BRACKET)

    if tool == "file_editor":
        cmd = action.get("command", "?")
        path = action.get("path", "")
        path = path.split("/")[-1] if path else "?"
        result.append(tool, style=TOOL)
        result.append(":", style=BRACKET)
        result.append(cmd, style=COMMAND)
        result.append("  ", style=SECONDARY)
        result.append(path, style=PATH)
    elif tool == "terminal":
        cmd = action.get("command", "").replace("\n", " ")
        result.append("terminal", style=TOOL)
        result.append("  ", style=SECONDARY)
        result.append(truncate(cmd, 75), style=COMMAND)
    else:
        thought = data.get("thought", [])
        thought_text = ""
        if thought and isinstance(thought[0], dict):
            thought_text = thought[0].get("text", "")
        thought_text = thought_text.replace("\n", " ")
        result.append(tool, style=TOOL)
        result.append("  ", style=SECONDARY)
        result.append(truncate(thought_text, 70), style=SECONDARY)

    return result

def format_observation(data: dict) -> Text:
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

    result = Text()
    result.append("[", style=BRACKET)
    result.append("Obs      ", style=LABEL_OBS)  # Padded to 9 chars
    result.append("]", style=BRACKET)
    result.append("  ", style=BRACKET)
    result.append(tool, style=TOOL)
    result.append("  ", style=SECONDARY)

    if is_error:
        result.append("[ERR] ", style=ERROR)
        result.append(truncate(text, 65), style=SECONDARY)
    else:
        result.append(truncate(text, 70), style=SECONDARY)

    return result

def format_unknown(event_type: str, data: dict) -> Text:
    """Unknown event type: Show type and first few keys."""
    keys = list(data.keys())[:5]
    result = Text()
    result.append("[", style=BRACKET)
    result.append(event_type, style=WARNING)
    result.append("]", style=BRACKET)
    result.append(" keys=", style=SECONDARY)
    result.append(str(keys), style=SECONDARY)
    return result

class MinimalVisualizer(ConversationVisualizerBase):
    """A minimal visualizer that print the raw events as they occur."""

    def on_event(self, event: Event) -> None:
        """Handle events for minimal progress visualization."""
        if isinstance(event, MessageEvent):
            console.print(format_message(event.model_dump()))
        elif isinstance(event, SystemPromptEvent):
            console.print(format_system_prompt(event.model_dump()))
        elif isinstance(event, ActionEvent):
            console.print(format_action(event.model_dump()))
        elif isinstance(event, ObservationEvent):
            console.print(format_observation(event.model_dump()))
        else:
            console.print(format_unknown(type(event).__name__, event.model_dump()))
            event_text = Text()
            event_text.append("\n\n[EVENT] ", style=SECONDARY)
            event_text.append(type(event).__name__, style=WARNING)
            event_text.append(": ", style=SECONDARY)
            event_text.append(event.model_dump_json()[:200] + "...", style=SECONDARY)
            console.print(event_text)

class InteractiveVisualizer(ConversationVisualizerBase):
    """A minimal visualizer that print the raw events as they occur."""

    def on_event(self, event: Event) -> None:
        """Handle events for minimal progress visualization."""
        if isinstance(event, MessageEvent):
            if event.llm_message.role == 'user':
                pass
            else:
                console.print(format_message(event.model_dump(), False))
        elif isinstance(event, SystemPromptEvent):
            console.print(format_system_prompt(event.model_dump()))
        elif isinstance(event, ActionEvent):
            console.print(format_action(event.model_dump()))
        elif isinstance(event, ObservationEvent):
            console.print(format_observation(event.model_dump()))
        else:
            console.print(format_unknown(type(event).__name__, event.model_dump()))
            event_text = Text()
            event_text.append("\n\n[EVENT] ", style=SECONDARY)
            event_text.append(type(event).__name__, style=WARNING)
            event_text.append(": ", style=SECONDARY)
            event_text.append(event.model_dump_json()[:200] + "...", style=SECONDARY)
            console.print(event_text)

def run_agent(llm: LLM, prompt: str):
    agent = Agent(
        llm=llm,
        tools=[
            Tool(name=TerminalTool.name),
            Tool(name=FileEditorTool.name),
            Tool(name=TaskTrackerTool.name),
            Tool(name=BrowserToolSet.name),
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


def ticket_help() -> str:
    """Simple git checkout with minimal error handling."""
    result = subprocess.run(['./ticket'], check=True, capture_output=True)
    return str(result.stdout, "utf-8")

def next_ticket() -> str:
    """Simple git checkout with minimal error handling."""
    result = subprocess.run(['./ticket', 'ready'], check=True, capture_output=True)
    lines = result.stdout.splitlines()
    if not lines:
        return ""
    return str(lines[0], "utf-8").split()[0]

def parse_args():
    parser = argparse.ArgumentParser(prog='hans', description='OpenHands powered code helper')
    parser.add_argument('-i', '--interactive', action='store_true', help='Interactive (good for creating tickets)')
    parser.add_argument('-n', '--num-iterations', default=10, help='Max times to call agent')
    return parser.parse_args()

def run_interactive(llm: LLM, args):
    """An agent that takes user input and manages the tickets."""

    agent = Agent(
        llm=llm,
        tools=[
            Tool(name=TerminalTool.name),
            Tool(name=GlobTool.name),
            Tool(name=GrepTool.name),
        ]
    )

    cwd = os.getcwd()
    conversation = Conversation(
        agent=agent,
        workspace=cwd,
        visualizer=InteractiveVisualizer(),
    )

    conversation.send_message(f"""
        # Purpose
        You are mainly here to help create tickets and manage the open ones.
        Help gather context and scope out tasks for an agent to do autonomously.
        # Constraints
        Show me yout proposed tickets for confirmation before creating.
        # CLI
        Call this like `./ticket`

        {ticket_help()}
    """)
    print("-" * 50)
    while True:
        try:
            user_input = input("\nYou: ").strip()

            if not user_input:
                print("exit or go?")
                continue

            if user_input.lower() in ('quit', 'exit'):
                print("Goodbye!")
                break
            if user_input.lower() == ('go'):
                run_ticket_agent(llm, args)
                return

            conversation.send_message(user_input)
            conversation.run()

        except KeyboardInterrupt:
            print("\nGoodbye!")
            break
        except EOFError:
            print("\nGoodbye!")
            break

def run_ticket_agent(llm, args):
    for i in range(20):
        ticket_id = next_ticket()
        if not ticket_id:
            print("No work on the hopper! Try -i")
            break
        ticket_file = f'.tickets/{ticket_id}.md'
        print(f"═══ Iteration {i+1} ═══")
        prompt = f"""
        * Do only the task in `{ticket_file}`.
        * Record your progress and obstacles with `./ticket add-note {ticket_id} "..."`
        * Update AGENTS.md (short) and docs/ with learnings if needed.
        * If complete use `./ticket close {ticket_id}` to mark it complete
        * Commit progress if tests pass
        * Commit .tickets updates if needed
        """
        run_agent(llm=llm, prompt=prompt)

def run():
    llm = LLM(
        model=os.getenv("LLM_MODEL", "anthropic/claude-sonnet-4-5-20250929"),
        api_key=os.getenv("LLM_API_KEY"),
        base_url=os.getenv("LLM_BASE_URL", None),
    )
    args = parse_args()

    if args.interactive:
        run_interactive(llm, args)
    else:
        run_ticket_agent(llm, args)




print("🚀 Starting Hans")
run()
