"""Provides device calibration information for the runtime check."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class Calibration:
    """Represents the calibration data for a quantum device."""

    error_rate: float

    def get_expected_success_probability(self, code: str) -> float:
        """Get the expected success probability of the device for some program.

        Args:
            code (str): The program to check.

        Returns:
            float: The expected success probability.
        """
        instructions = code.count(";")
        return (1 - self.error_rate) ** instructions

    @classmethod
    def example(cls) -> Calibration:
        """Get an example calibration."""
        return cls(0.01)
