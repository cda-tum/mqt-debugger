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
        "exceptionBreakpointFilters": get_exception_filters(),
        "supportsStepBack": True,
        "supportsSetVariable": False,
        "supportsRestartFrame": True,
        "supportsTerminateRequest": True,
        "supportsRestartRequest": True,
        "supportsVariableType": True,
        "supportsDelayedStackTraceLoading": False,
        "supportsVariablePaging": True,
    }


def get_exception_filters() -> list[dict[str, Any]]:
    """Returns the exception filters supported by DAP server.

    Returns:
        list[dict[str, Any]]: The exception filters of the DAP server.
    """
    return [
        {
            "filter": "all",
            "label": "All Assertions",
            "description": "Filters for all assertion failures.",
            "default": True,
        },
        {
            "filter": "ent",
            "label": "Entanglement Assertions",
            "description": "Filters for entanglement assertion failures.",
            "default": False,
        },
        {
            "filter": "sup",
            "label": "Superposition Assertions",
            "description": "Filters for superposition assertion failures.",
            "default": False,
        },
        {
            "filter": "eq",
            "label": "Equality Assertions",
            "description": "Filters for equality assertion failures.",
            "default": False,
        },
    ]
