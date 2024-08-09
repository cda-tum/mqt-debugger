"""Tests diagnosis methods of the debugger."""

from __future__ import annotations

import locale
from pathlib import Path

from mqt.debug import (
    ErrorCauseType,
    SimulationState,
    create_ddsim_simulation_state,
    destroy_ddsim_simulation_state,
)


def load_instance(name: str) -> SimulationState:
    """Loads a quantum simulation and debugging instance from a file.

    Args:
        name (str): The name of the instance to load.

    Returns:
        SimulationState: The generated simulation state.
    """
    state = create_ddsim_simulation_state()
    with Path(f"test/python/resources/diagnosis/{name}.qasm").open(encoding=locale.getpreferredencoding(False)) as f:
        state.load_code(f.read())
    return state


def test_data_dependencies() -> None:
    """Test the data dependency analysis."""
    s = load_instance("dependencies")
    dependencies = s.get_diagnostics().get_data_dependencies(6)
    assert dependencies == [1, 2, 3, 6]
    dependencies = s.get_diagnostics().get_data_dependencies(7)
    assert dependencies == [1, 2, 3, 4, 5, 7]
    destroy_ddsim_simulation_state(s)


def test_data_dependencies_jumps() -> None:
    """Test the data dependency analysis in code with jumps."""
    s = load_instance("dependencies-with-jumps")
    dependencies = s.get_diagnostics().get_data_dependencies(4)
    assert dependencies == [2, 3, 4]
    dependencies = s.get_diagnostics().get_data_dependencies(9)
    assert dependencies == [6, 7, 8, 9]
    destroy_ddsim_simulation_state(s)


def test_control_always_zero() -> None:
    """Test the control-always-zero error diagnosis."""
    s = load_instance("control-always-zero")
    s.run_simulation()
    causes = s.get_diagnostics().potential_error_causes()

    assert len(causes) == 1  # once diagnosis can step into jumps, this should be 2

    assert causes[0].type == ErrorCauseType.ControlAlwaysZero
    assert causes[0].instruction == 12


def test_missing_interaction() -> None:
    """Test the missing-interaction error diagnosis."""
    s = load_instance("missing-interaction")
    s.run_simulation()
    causes = [x for x in s.get_diagnostics().potential_error_causes() if x.type == ErrorCauseType.MissingInteraction]
    assert len(causes) == 0
    s.run_simulation()
    causes = [x for x in s.get_diagnostics().potential_error_causes() if x.type == ErrorCauseType.MissingInteraction]
    assert len(causes) == 1
    assert causes[0].instruction == 4
