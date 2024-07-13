"""Represents the 'stackTrace' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class StackTraceDAPMessage(DAPMessage):
    """Represents the 'stackTrace' DAP request."""

    message_type_name: str = "stackTrace"

    start_frame: int
    levels: int

    parameters: bool
    parameter_types: bool
    parameter_names: bool
    parameter_values: bool
    line: bool
    include_all: bool

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'StackTraceDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'stackTrace' request.
        """
        super().__init__(message)
        self.start_frame = message["arguments"]["startFrame"]
        self.levels = message["arguments"]["levels"]

        if "format" not in message["arguments"]:
            self.parameters = False
            self.parameter_types = False
            self.parameter_names = False
            self.parameter_values = False
            self.line = True
            self.include_all = False
            return
        self.parameters = message["arguments"]["format"]["parameters"]
        self.parameter_types = message["arguments"]["format"]["parameterTypes"]
        self.parameter_names = message["arguments"]["format"]["parameterNames"]
        self.parameter_values = message["arguments"]["format"]["parameterValues"]
        self.line = message["arguments"]["format"]["line"]
        self.include_all = message["arguments"]["format"]["includeAll"]

    def validate(self) -> None:
        """Validates the 'StackTraceDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'stackTrace' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        # TODO implement for debugger
        d = super().handle(server)

        current = server.simulation_state.get_current_instruction()
        (start, end) = server.simulation_state.get_instruction_position(current)
        start_line, start_col = server.code_pos_to_coordinates(start)
        end_line, end_col = server.code_pos_to_coordinates(end)

        d["body"] = {
            "stackFrames": [
                {
                    "id": 1,
                    "name": "main",
                    "line": start_line,
                    "endLine": end_line,
                    "column": start_col,
                    "endColumn": end_col,
                    "source": server.source_file,
                }
            ],
            "totalFrames": 1,
        }
        return d
