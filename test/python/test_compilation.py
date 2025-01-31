"""Tests the compilation procedures for assertion programs."""

from __future__ import annotations

from pathlib import Path

import pytest

from mqt.debugger import check

CALIBRATION = check.Calibration(0.1)
BASE_PATH = Path("test/python/resources/compilation/")


@pytest.fixture(scope="module")
def compiled_slice_1() -> str:
    """A fixture for slice 1 of the compiled program.

    Returns:
        str: The compiled program slice code.
    """
    with (BASE_PATH / "test_program_compiled" / "slice_1.qasm").open("r") as f:
        return f.read()


def test_correct_good_sample_size(compiled_slice_1: str) -> None:
    """Test the correctness of a run result with a good sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    assert (
        check.check_result(compiled_slice_1, BASE_PATH / "bell-pair-equal-90-250.json", CALIBRATION, silent=True)
        is True
    )


def test_correct_bad_sample_size(compiled_slice_1: str) -> None:
    """Test the correctness of a run result with a bad sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    assert check.check_result(
        compiled_slice_1, BASE_PATH / "bell-pair-equal-90-100.json", CALIBRATION, silent=True
    ) in {True, False}


def test_incorrect_bad_sample_size(compiled_slice_1: str) -> None:
    """Test the incorrectness of a run result with a bad sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    assert check.check_result(
        compiled_slice_1, BASE_PATH / "bell-pair-unequal-90-100.json", CALIBRATION, silent=True
    ) in {True, False}


def test_incorrect_good_sample_size(compiled_slice_1: str) -> None:
    """Test the incorrectness of a run result with a good sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    assert (
        check.check_result(compiled_slice_1, BASE_PATH / "bell-pair-unequal-90-250.json", CALIBRATION, silent=True)
        is False
    )
