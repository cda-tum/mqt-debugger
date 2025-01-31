"""Module responsible for the preparation of running programs on real hardware."""

from __future__ import annotations

import math
from typing import TYPE_CHECKING

import numpy as np

import mqt.debugger as dbg

if TYPE_CHECKING:
    from pathlib import Path

    from .calibration import Calibration


def extract_assertions_from_code(code: str) -> list[str]:
    """Extract the assertions from the given code.

    Args:
        code (str): The code to extract the assertions from.

    Returns:
        list[str]: The extracted assertions.
    """
    return [x[11:] for x in code.splitlines() if x.startswith("// ASSERT: ")]


def estimate_required_shots(compiled_program: str, calibration: Calibration) -> int:
    """Compute an estimate of the required shots for the current slice, based on the given device information.

    Args:
        compiled_program (str): The compiled program slice.
        calibration (Calibration): The calibration data for the device.

    Returns:
        int: The estimated number of shots required.
    """
    expected_success_probability = calibration.get_expected_success_probability(compiled_program)
    assertions = extract_assertions_from_code(compiled_program)
    max_variables = max(a.split("}")[0].split("{")[1].count(",") + 1 for a in assertions)
    k = 2**max_variables

    # Compute the number of samples required so that each bin contains at least 5 samples.
    min_expectation = (1 - expected_success_probability) / k
    min_samples_high_fidelity = int(math.ceil(5 / min_expectation))

    min_samples_low_fidelity = int(math.ceil(k / expected_success_probability**2 * np.log(1 / 0.05)))
    return max(min_samples_high_fidelity, min_samples_low_fidelity)


def start_compilation(code: Path, output_dir: Path, calibration: Calibration) -> None:
    """Start the compilation of the assertion program.

    Args:
        code (Path): The path to the assertion program in extended OpenQASM format.
        output_dir (Path): The directory to store the compiled slices.
        calibration (Calibration): The calibration data for the device.
    """
    state = dbg.create_ddsim_simulation_state()
    with code.open("r") as f:
        code_str = f.read()
    state.load_code(code_str)
    i = 0
    while True:
        i += 1
        settings = dbg.CompilationSettings(
            dbg.CompilationMode.StatisticalSlices,
            opt=0,
            slice_index=i - 1,
        )
        compiled = state.compile(settings)
        if not compiled:
            break
        with (output_dir / f"slice_{i}.qasm").open("w") as f:
            f.write(compiled)
        print(f"Compiled slice {i}. Required shots: {estimate_required_shots(compiled, calibration)}")  # noqa: T201
    dbg.destroy_ddsim_simulation_state(state)
