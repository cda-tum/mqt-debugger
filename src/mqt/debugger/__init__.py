"""A module for interfacing with the mqt-debugger library."""

from __future__ import annotations

from . import check, dap
from ._version import version as __version__
from .pydebugger import (
    CompilationMode,
    CompilationSettings,
    Complex,
    Diagnostics,
    ErrorCause,
    ErrorCauseType,
    SimulationState,
    Statevector,
    Variable,
    VariableType,
    VariableValue,
    create_ddsim_simulation_state,
    destroy_ddsim_simulation_state,
)

__all__ = [
    "CompilationMode",
    "CompilationSettings",
    "Complex",
    "Diagnostics",
    "ErrorCause",
    "ErrorCauseType",
    "SimulationState",
    "Statevector",
    "Variable",
    "VariableType",
    "VariableValue",
    "__version__",
    "check",
    "check",
    "create_ddsim_simulation_state",
    "create_ddsim_simulation_state",
    "dap",
    "destroy_ddsim_simulation_state",
]
