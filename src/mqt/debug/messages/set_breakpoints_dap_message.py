"""Represents the 'setBreakpoints' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer

# TODO this fails if another file also has breakpoints


class SetBreakpointsDAPMessage(DAPMessage):
    """Represents the 'setBreakpoints' DAP request."""

    message_type_name: str = "setBreakpoints"

    breakpoints: list[tuple[str, int]]
    source: dict[str, Any]

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'SetBreakpointsDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'setBreakpoints' request.
        """
        self.breakpoints = [("line", int(x["line"])) for x in message["arguments"]["breakpoints"]]
        self.source = message["arguments"]["source"]
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'SetBreakpointsDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'setBreakpoints' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        # TODO this can be expanded on later
        d = super().handle(server)
        bpts = [
            {
                "id": i,
                "verified": True,
                "source": self.source,
                "line": b[1],
                "column": 1,
                "endLine": b[1],
                "endColumn": 5,
            }
            for i, b in enumerate(self.breakpoints)
        ]
        d["body"] = {"breakpoints": bpts}
        return d
