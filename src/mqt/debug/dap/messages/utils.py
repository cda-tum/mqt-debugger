"""Provides utility functions for the DAP messages."""

from __future__ import annotations

from typing import Any


def get_default_capabilities() -> dict[str, Any]:
    """Returns the default capabilities of the DAP server.

    Returns:
        dict[str, Any]: The default capabilities of the DAP server.
    """
    return {
        "supportsConfigurationDoneRequest": True,
        "supportsFunctionBreakpoints": False,
        "supportsConditionalBreakpoints": False,
        "supportsHitConditionalBreakpoints": False,
        "supportsEvaluateForHovers": False,
        "supportsExceptionInfoRequest": True,
        "exceptionBreakpointFilters": [],
        "supportsStepBack": True,
        "supportsSetVariable": False,
        "supportsRestartFrame": True,
        "supportsTerminateRequest": True,
        "supportsRestartRequest": True,
        "supportsVariableType": True,
        "supportsDelayedStackTraceLoading": False,
        "supportsVariablePaging": True,
    }
