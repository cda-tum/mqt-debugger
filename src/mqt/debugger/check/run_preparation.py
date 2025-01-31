"""Module responsible for the preparation of running programs on real hardware."""

from __future__ import annotations

from typing import TYPE_CHECKING

import mqt.debugger as dbg

if TYPE_CHECKING:
    from pathlib import Path


def estimate_required_shots() -> int:
    """Compute an estimate of the required shots for the current slice, based on the given device information.

    Returns:
        int: The estimated number of shots required.
    """
    return 100


def start_compilation(code: Path, output_dir: Path) -> None:
    """Start the compilation process."""
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
        print(f"Compiled slice {i}. Required shots: {estimate_required_shots()}")  # noqa: T201
    dbg.destroy_ddsim_simulation_state(state)
