"""Represents the 'initialized' DAP event."""

from __future__ import annotations

from .dap_event import DAPEvent


class InitializedDAPEvent(DAPEvent):
    """Represents the 'initialized' DAP event."""

    event_name = "initialized"

    def __init__(self) -> None:
        """Create a new 'initialized' DAP event message."""
        super().__init__()

    def validate(self) -> None:
        """Validate the 'initialized' DAP event message after creation."""
