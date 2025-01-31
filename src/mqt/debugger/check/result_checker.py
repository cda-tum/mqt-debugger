"""Handles the checking of the results of the quantum program with assertions."""

from __future__ import annotations

import json
from dataclasses import dataclass
from typing import TYPE_CHECKING

from scipy.stats import chisquare  # type: ignore[import-not-found]

if TYPE_CHECKING:
    from pathlib import Path

COLOR_GREEN = "\033[92m"
COLOR_RED = "\033[91m"
COLOR_RESET = "\033[0m"

EXPECTED_FIDELITY = 0.8


@dataclass
class Result:
    """Represents the results of a quantum program execution over several shots."""

    num_samples: int
    distribution: dict[tuple[str, ...], list[int]]

    @classmethod
    def load(cls, path: Path, distributions: list[tuple[str, ...]]) -> Result:
        """Load the results from a file.

        Args:
            path (Path): The path to the file containing the results.
            distributions (list[tuple[str, ...]]): The set of distributions that should be grouped.

        Returns:
            Result: The result of the quantum program.
        """
        with path.open("r") as f:
            data = json.load(f)
        filled_distribution = {key: [0 for _ in range(2 ** len(key))] for key in distributions}
        for entry in data:
            indices = dict.fromkeys(distributions, 0)
            for key in entry:
                if entry[key] == 0:
                    continue
                parent_distribution = next(x for x in distributions if key in x)
                index = parent_distribution.index(key)
                indices[parent_distribution] += 2 ** (len(parent_distribution) - index - 1)
            for key, value in indices.items():
                filled_distribution[key][value] += 1
        return Result(len(data), filled_distribution)


def distrbituion_equal_under_noise(distribution: list[int], expected_probs: list[float], num_samples: int) -> bool:
    """Check if the distribution is equal to the expected distribution under noise.

    Args:
        distribution (list[int]): The observed distribution.
        expected_probs (list[float]): The expected probabilities.
        num_samples (int): The number of samples.

    Returns:
        bool: True if the distribution is equal to the expected distribution under noise, False otherwise.
    """
    expected_with_noise = [
        EXPECTED_FIDELITY * x + (1 - EXPECTED_FIDELITY) / len(expected_probs) for x in expected_probs
    ]
    expected_counts = [x * num_samples for x in expected_with_noise]
    result: tuple[float, float] = chisquare(distribution, expected_counts)
    (_chi2, p) = result
    return p > 0.05


def check_assertion_superposition(distribution: list[int], num_samples: int) -> bool:
    """Check a superposition assertion based on the distribution.

    Args:
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    to_test = [0.0 for _ in distribution]
    for i in range(len(distribution)):
        to_test[i] = 1.0
        if distrbituion_equal_under_noise(distribution, to_test, num_samples):
            return False
        to_test[i] = 0
    return True


def check_assertion_equality(assertion: str, distribution: list[int], num_samples: int) -> bool:
    """Check an equality assertion based on the distribution.

    Args:
        assertion (str): The assertion to check.
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    expected_statevector = [complex(x) for x in assertion[1:].split("}")[0].split(",")]
    magnitude = sum(abs(x) ** 2 for x in expected_statevector) ** 0.5
    expected_statevector = [x / magnitude for x in expected_statevector]

    expected_distribution = [(x * x.conjugate()).real for x in expected_statevector]

    return distrbituion_equal_under_noise(distribution, expected_distribution, num_samples)


def check_assertion(assertion: str, distribution: list[int], num_samples: int) -> bool:
    """Check an assertion based on the distribution.

    Args:
        assertion (str): The assertion to check.
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    if assertion.startswith("{superposition}"):
        return check_assertion_superposition(distribution, num_samples)
    return check_assertion_equality(assertion, distribution, num_samples)


def check_result(compiled_code: str, result_path: Path) -> None:
    """Check the result of a quantum program against assertions.

    Args:
        compiled_code (str): The compiled code of the quantum program.
        result_path (Path): The path to the file containing the results.
    """
    lines = compiled_code.splitlines()
    distributions: list[tuple[str, ...]] = []
    assertions: dict[tuple[str, ...], str] = {}
    for line in lines:
        if not line.startswith("// ASSERT:"):
            break
        var_list = tuple(line.split("(")[1].split(")")[0].split(","))
        distributions.append(var_list)
        assertions[var_list] = f"{{{line.split("{")[1]}"
    result = Result.load(result_path, distributions)

    for key, value in assertions.items():
        res = check_assertion(value, result.distribution[key], result.num_samples)
        if not res:
            print(f"{COLOR_RED}Assertion {key} failed.{COLOR_RESET}")  # noqa: T201
        else:
            print(f"{COLOR_GREEN}Assertion {key} passed.{COLOR_RESET}")  # noqa: T201
