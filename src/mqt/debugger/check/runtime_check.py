"""This module handles running assertion programs on real hardware and using statistical tests to check the results."""

from __future__ import annotations

from pathlib import Path

from . import result_checker

compiled_code_1 = """// ASSERT: (test_q0,test_q1) {superposition}
creg test_q0[1];
creg test_q1[1];
qreg q[2];
h q[0];
measure q[0] -> test_q0[0];
measure q[1] -> test_q1[0];
"""

compiled_code_2 = """// ASSERT: (test_q0,test_q1) {0.707,0,0,0.707}
creg test_q0[1];
creg test_q1[1];
qreg q[2];
h q[0];
cx q[0], q[1];
measure q[0] -> test_q0[0];
measure q[1] -> test_q1[0];
"""


def main() -> None:
    """The main function."""
    # result_checker.check_result(compiled_code_2, Path("experiments/assertion-compilation/bell-pair.json"))
    result_checker.check_result(compiled_code_1, Path("experiments/assertion-compilation/zeros.json"))


if __name__ == "__main__":
    main()
