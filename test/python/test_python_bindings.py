from __future__ import annotations

import locale
from typing import Generator

import pytest

import mqt.debug

type SimulationInstance = tuple[mqt.debug.SimulationState, int]


@pytest.fixture(scope="module")
def simulation_instance_ghz() -> Generator[SimulationInstance, None, None]:
    simulation_state = mqt.debug.create_ddsim_simulation_state()
    with open("test/python/resources/bindings/ghz-incorrect.qasm", encoding=locale.getpreferredencoding(False)) as f:
        code = f.read()
        print(code)
        simulation_state.load_code(code)
    yield (simulation_state, 0)
    mqt.debug.destroy_ddsim_simulation_state(simulation_state)


@pytest.fixture(scope="module")
def simulation_instance_jumps() -> Generator[SimulationInstance, None, None]:
    simulation_state = mqt.debug.create_ddsim_simulation_state()
    with open("test/python/resources/bindings/jumps.qasm", encoding=locale.getpreferredencoding(False)) as f:
        code = f.read()
        print(code)
        simulation_state.load_code(code)
    yield (simulation_state, 1)
    mqt.debug.destroy_ddsim_simulation_state(simulation_state)


@pytest.fixture(scope="module")
def simulation_instance_classical() -> Generator[SimulationInstance, None, None]:
    simulation_state = mqt.debug.create_ddsim_simulation_state()
    with open("test/python/resources/bindings/classical.qasm", encoding=locale.getpreferredencoding(False)) as f:
        code = f.read()
        print(code)
        simulation_state.load_code(code)
    yield (simulation_state, 2)
    mqt.debug.destroy_ddsim_simulation_state(simulation_state)


@pytest.fixture(autouse=True)
def simulation_state_cleanup(
    simulation_instance_ghz: SimulationInstance,
    simulation_instance_jumps: SimulationInstance,
    simulation_instance_classical: SimulationInstance,
) -> Generator[None, None, None]:
    yield
    simulation_instance_ghz[0].reset_simulation()
    simulation_instance_jumps[0].reset_simulation()
    simulation_instance_classical[0].reset_simulation()


def load_fixture(request, name: str) -> tuple[mqt.debug.SimulationState, int]:
    return request.getfixturevalue(name)


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_run(simulation_instance: SimulationInstance, request) -> None:
    (simulation_state, id) = load_fixture(request, simulation_instance)
    simulation_state.run_simulation()
    print(simulation_state.get_instruction_count())
    assert simulation_state.is_finished() == (id != 0)


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_current_instruction(simulation_instance: SimulationInstance, request) -> None:
    (simulation_state, id) = load_fixture(request, simulation_instance)
    assert simulation_state.get_current_instruction() == 0
    simulation_state.step_forward()
    assert simulation_state.get_current_instruction() == 1
    simulation_state.step_forward()
    assert simulation_state.get_current_instruction() == (2 if id != 1 else 4)
    simulation_state.step_backward()
    assert simulation_state.get_current_instruction() == 1
    simulation_state.step_over_backward()
    assert simulation_state.get_current_instruction() == 0
    simulation_state.step_over_forward()
    assert simulation_state.get_current_instruction() == 1


def test_step_out(simulation_instance_jumps: SimulationInstance) -> None:
    (simulation_state, _id) = simulation_instance_jumps
    simulation_state.set_breakpoint(183)
    simulation_state.run_simulation()
    assert simulation_state.get_current_instruction() == 12
    simulation_state.step_out_backward()
    assert simulation_state.get_current_instruction() == 18
    simulation_state.run_simulation()
    assert simulation_state.get_current_instruction() == 12
    simulation_state.step_out_forward()
    assert simulation_state.get_current_instruction() == 19
