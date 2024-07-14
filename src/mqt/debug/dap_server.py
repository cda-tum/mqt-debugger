"""Handles the responsibilities for a DAP server."""

from __future__ import annotations

import json
import socket
from typing import Any

import mqt.debug

from .messages import (
    ConfigurationDoneDAPMessage,
    ContinueDAPMessage,
    DisconnectDAPMessage,
    ExceptionInfoDAPMessage,
    InitializeDAPMessage,
    LaunchDAPMessage,
    NextDAPMessage,
    PauseDAPMessage,
    Request,
    RestartDAPMessage,
    ReverseContinueDAPMessage,
    ScopesDAPMessage,
    SetBreakpointsDAPMessage,
    SetExceptionBreakpointsDAPMessage,
    StackTraceDAPMessage,
    StepBackDAPMessage,
    StepInDAPMessage,
    StepOutDAPMessage,
    TerminateDAPMessage,
    ThreadsDAPMessage,
    VariablesDAPMessage,
)
from .pydebug import SimulationState

supported_messages: list[type[Request]] = [
    InitializeDAPMessage,
    DisconnectDAPMessage,
    LaunchDAPMessage,
    SetBreakpointsDAPMessage,
    ThreadsDAPMessage,
    StackTraceDAPMessage,
    ConfigurationDoneDAPMessage,
    NextDAPMessage,
    StepBackDAPMessage,
    StepInDAPMessage,
    ContinueDAPMessage,
    TerminateDAPMessage,
    RestartDAPMessage,
    ScopesDAPMessage,
    VariablesDAPMessage,
    ReverseContinueDAPMessage,
    StepOutDAPMessage,
    PauseDAPMessage,
    SetExceptionBreakpointsDAPMessage,
    ExceptionInfoDAPMessage,
]


def send_message(msg: str, client: socket.socket) -> None:
    """Send a message to the client according to the DAP messaging protocol.

    Args:
        msg (str): The message to send.
        client (socket.socket): The client socket to send the message to.
    """
    msg = msg.replace("\n", "\r\n")
    length = len(msg)
    header = f"Content-Length: {length}\r\n\r\n".encode("ascii")
    client.sendall(header + msg.encode("utf-8"))


class DAPServer:
    """The DAP server class."""

    host: str
    port: int

    simulation_state: SimulationState
    source_file: dict[str, Any]
    source_code: str
    can_step_back: bool
    exception_breakpoints: list[str]

    def __init__(self, host: str = "127.0.0.1", port: int = 4711) -> None:
        """Create a new DAP server instance.

        Args:
            host (str, optional): The host IP Address. Defaults to "0.0.0.0".
            port (int, optional): The port to run the server on. Defaults to 4711.
        """
        self.host = host
        self.port = port
        self.can_step_back = False
        self.simulation_state = SimulationState()

    def start(self) -> None:
        """Start the DAP server and listen for one connection."""
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # TODO remove
            s.bind((self.host, self.port))
            s.listen()

            try:
                conn, _addr = s.accept()
                with conn:
                    self.handle_client(conn)
            except RuntimeError:
                s.close()

    def handle_client(self, connection: socket.socket) -> None:
        """Handle incoming messages from the client.

        Args:
            connection (socket.socket): The client socket.
        """
        while True:
            data = connection.recv(1024)
            parts = data.decode().split("\n")
            if not parts or not data:
                break
            payload = json.loads(parts[-1])
            result, cmd = self.handle_command(payload)
            result_payload = json.dumps(result)
            send_message(result_payload, connection)

            e: mqt.debug.messages.DAPEvent | None = None
            if isinstance(cmd, mqt.debug.messages.LaunchDAPMessage):
                e = mqt.debug.messages.InitializedDAPEvent()
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if (
                isinstance(cmd, (mqt.debug.messages.LaunchDAPMessage, mqt.debug.messages.RestartDAPMessage))
                and cmd.stop_on_entry
            ):
                e = mqt.debug.messages.StoppedDAPEvent(mqt.debug.messages.StopReason.ENTRY, "Stopped on entry")
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if isinstance(
                cmd,
                (
                    mqt.debug.messages.NextDAPMessage,
                    mqt.debug.messages.StepBackDAPMessage,
                    mqt.debug.messages.StepInDAPMessage,
                    mqt.debug.messages.StepOutDAPMessage,
                    mqt.debug.messages.ContinueDAPMessage,
                    mqt.debug.messages.ReverseContinueDAPMessage,
                ),
            ):
                event = (
                    mqt.debug.messages.StopReason.EXCEPTION
                    if self.simulation_state.did_assertion_fail()
                    else mqt.debug.messages.StopReason.STEP
                )
                message = "An assertion failed" if self.simulation_state.did_assertion_fail() else "Stopped after step"
                e = mqt.debug.messages.StoppedDAPEvent(event, message)
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if isinstance(cmd, mqt.debug.messages.TerminateDAPMessage):
                e = mqt.debug.messages.TerminatedDAPEvent()
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
                e = mqt.debug.messages.ExitedDAPEvent(143)
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if isinstance(cmd, mqt.debug.messages.PauseDAPMessage):
                e = mqt.debug.messages.StoppedDAPEvent(mqt.debug.messages.StopReason.PAUSE, "Stopped after pause")
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            self.regular_checks(connection)

    def regular_checks(self, connection: socket.socket) -> None:
        """Perform regular checks and send events to the client if necessary.

        Args:
            connection (socket.socket): The client socket.
        """
        e: mqt.debug.messages.DAPEvent | None = None
        if self.simulation_state.is_finished() and self.simulation_state.get_instruction_count() != 0:
            e = mqt.debug.messages.ExitedDAPEvent(0)
            event_payload = json.dumps(e.encode())
            send_message(event_payload, connection)
        if self.can_step_back != self.simulation_state.can_step_backward():
            self.can_step_back = self.simulation_state.can_step_backward()
            e = mqt.debug.messages.CapabilitiesDAPEvent({"supportsStepBack": self.can_step_back})
            event_payload = json.dumps(e.encode())

    def handle_command(self, command: dict[str, Any]) -> tuple[dict[str, Any], mqt.debug.messages.DAPMessage]:
        """Handle an incoming command from the client and return the corresponding response.

        Args:
            command (dict[str, Any]): The command read from the client.

        Raises:
            RuntimeError: If the command is not supported.

        Returns:
            tuple[dict[str, Any], mqt.debug.messages.DAPMessage]: The response to the message as a dictionary and the message object.
        """
        for message_type in supported_messages:
            if message_type.message_type_name == command["command"]:
                message = message_type(command)
                return (message.handle(self), message)
        msg = f"Unsupported command: {command['command']}"
        raise RuntimeError(msg)

    def code_pos_to_coordinates(self, pos: int, start_col_at_1: bool = True) -> tuple[int, int]:
        """Helper method to convert a code position to line and column.

        Args:
            pos (int): The 0-indexed position in the code.
            start_col_at_1 (bool, optional): Indicates, whether columns start at index 1. Defaults to True.

        Returns:
            tuple[int, int]: The line and column. Lines are 1-indexed.
        """
        lines = self.source_code.split("\n")
        line = 0
        col = 0
        for i, line_code in enumerate(lines):
            if pos < len(line_code):
                line = i + 1
                col = pos
                break
            pos -= len(line_code) + 1
        if start_col_at_1:
            col += 1
        return (line, col)


if __name__ == "__main__":
    server = DAPServer()
    server.start()