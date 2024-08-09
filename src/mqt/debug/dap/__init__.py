"""This module handles DAP capabilities of the debugger."""

from __future__ import annotations

from . import messages
from .dap_server import DAPServer

__all__ = ["DAPServer", "messages"]
