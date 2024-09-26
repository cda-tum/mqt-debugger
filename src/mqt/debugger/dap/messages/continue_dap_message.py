"""Represents the 'continue' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class ContinueDAPMessage(DAPMessage):
    """Represents the 'continue' DAP request."""

    message_type_name: str = "continue"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'ContinueDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'continue' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'ContinueDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'continue' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        server.simulation_state.run_simulation()
        d = super().handle(server)
        d["body"] = {}
        return d
