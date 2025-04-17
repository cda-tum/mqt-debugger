"""Tests the compilation procedures for assertion programs."""

from __future__ import annotations

import json
import locale
import random
import tempfile
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest

from mqt.debugger import check

if TYPE_CHECKING:
    import types
    from io import TextIOWrapper

CALIBRATION = check.Calibration(0.1, 0.1, 0.1)
BASE_PATH = Path("test/python/resources/compilation/")


class GeneratedOutput:
    """A context manager for generating output files with a given distribution."""

    file: TextIOWrapper

    variables: list[str]
    expected_distribution: list[float]
    num_samples: int
    success_probability: float

    def generate_output(self) -> list[dict[str, Any]]:
        """Generate a list of sample output dictionaries.

        Returns:
            list[dict[str, any]]: The sample output dictionaries.
        """
        samples: list[dict[str, Any]] = []

        for _ in range(self.num_samples):
            r = random.random()  # noqa: S311
            found_index = -1
            for i, likelihood in enumerate(self.expected_distribution):
                if r < likelihood:
                    found_index = i
                    break
                r -= likelihood

            if random.random() > self.success_probability:  # noqa: S311
                found_index = random.randrange(0, len(self.expected_distribution))  # noqa: S311

            result = {}
            for v in self.variables:
                result[v] = found_index & 1
                found_index >>= 1
            samples.append(result)
        return samples

    def __init__(
        self, variables: list[str], expected_distribution: list[float], num_samples: int, success_probability: float
    ) -> None:
        """Create a new GeneratedOutput context manager.

        Args:
            variables (list[str]): The variables in the output.
            expected_distribution (list[float]): The expected distribution of the output.
            num_samples (int): The number of samples to generate.
            success_probability (float): The success probability of the output.
        """
        self.variables = variables
        self.expected_distribution = expected_distribution
        self.num_samples = num_samples
        self.success_probability = success_probability

    def __enter__(self) -> TextIOWrapper:
        """The context manager entry point.

        Returns:
            TextIOWrapper: The file containing the generated output.
        """
        with tempfile.NamedTemporaryFile(mode="w", delete=False, encoding=locale.getpreferredencoding(False)) as f:
            name = f.name
            generated = self.generate_output()
            json.dump(generated, f)

        self.file = Path(name).open("r", encoding=locale.getpreferredencoding(False))
        return self.file

    def __exit__(
        self,
        _exc_type: type[BaseException] | None,
        _exc_value: BaseException | None,
        _traceback: types.TracebackType | None,
    ) -> None:
        """The context manager exit point.

        Args:
            _exc_type (_type_): The type of the exception.
            _exc_value (_type_): The exception value.
            _traceback (_type_): The traceback.
        """
        name = self.file.name
        self.file.close()
        Path(name).unlink()


@pytest.fixture(scope="module")
def compiled_slice_1() -> str:
    """A fixture for slice 1 of the compiled program.

    Returns:
        str: The compiled program slice code.
    """
    with (BASE_PATH / "test_program_compiled" / "slice_1.qasm").open("r") as f:
        return f.read()


def test_correct_good_sample_size(compiled_slice_1: str) -> None:
    """Test the correctness of a run result with a large enough sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.5, 0, 0, 0.5], 250, expected_success_probability) as o:
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    assert errors <= 5


def test_correct_bad_sample_size(compiled_slice_1: str) -> None:
    """Test the correctness of a run result with a bad sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.5, 0, 0, 0.5], 25, expected_success_probability) as o:
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    assert errors >= 5


def test_incorrect_bad_sample_size(compiled_slice_1: str) -> None:
    """Test the incorrectness of a run result with a bad sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.75, 0, 0, 0.25], 25, expected_success_probability) as o:
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    # Due to the low sample size, the checker does not find at least one out of 4 errors
    assert errors <= 75


def test_incorrect_good_sample_size(compiled_slice_1: str) -> None:
    """Test the incorrectness of a run result with a good sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.75, 0, 0, 0.25], 250, expected_success_probability) as o:
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    assert errors >= 75


def test_sample_estimate(compiled_slice_1: str) -> None:
    """Test the estimation of required shots.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    n = check.estimate_required_shots(compiled_slice_1, CALIBRATION)
    assert n == 100
