"""Represents the 'terminate' DAP request."""

from __future__ import annotations

from typing import Any

from .. import DAPServer, destroy_ddsim_simulation_state
from .dap_message import DAPMessage


class TerminateDAPMessage(DAPMessage):
    """Represents the 'terminate' DAP request."""

    message_type_name: str = "terminate"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'TerminateDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'terminate' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'TerminateDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'terminate' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        destroy_ddsim_simulation_state(server.simulation_state)
        return super().handle(server)
