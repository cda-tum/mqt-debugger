"""A simple file to test the functionality of mqt-debug python bindings."""

from __future__ import annotations

from mqt import debug

code = """qreg q[3];
creg c[3];
h q[0];
cx q[0], q[1];
cx q[2], q[0];
assert-ent q[0], q[1];
assert-ent q[2], q[0];
assert-ent q[2], q[1];
"""


def main() -> None:
    """A simple test for the mqt-debug library."""
    s = debug.create_ddsim_simulation_state()
    s.load_code(code)
    s.run_simulation()
    if s.did_assertion_fail():
        print("Assertion failed")
        current = s.get_current_instruction()
        print(current)
        (start, end) = s.get_instruction_position(current)
        print(start, end)
        print(s.get_amplitude_index(0).real)
        print(s.get_state_vector_full().amplitudes)


if __name__ == "__main__":
    main()
