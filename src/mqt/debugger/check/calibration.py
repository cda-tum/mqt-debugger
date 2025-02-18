"""Provides device calibration information for the runtime check."""

from __future__ import annotations

from dataclasses import dataclass

from qiskit import QuantumCircuit


@dataclass
class Calibration:
    """Represents the calibration data for a quantum device."""

    error_rate_1q: float
    error_rate_2q: float
    error_rate_measurement: float

    def get_expected_success_probability(self, code: str) -> float:
        """Get the expected success probability of the device for some program.

        Currently, expected success probability is computed in terms of measurement and gate fidelities.

        Args:
            code (str): The program to check.

        Returns:
            float: The expected success probability.
        """
        fidelity_measurement = 1 - self.error_rate_measurement
        fidelity_1q = 1 - self.error_rate_1q
        fidelity_2q = 1 - self.error_rate_2q

        gate_fidelity = 1.0
        qc = QuantumCircuit.from_qasm_str(code)
        for instruction in qc.data:
            gate_type = instruction.name
            if gate_type == "barrier":
                continue
            if len(instruction.qubits) == 1:
                if gate_type == "measure":
                    gate_fidelity *= fidelity_measurement
                else:
                    gate_fidelity *= fidelity_1q
            else:
                gate_fidelity *= fidelity_2q

        return gate_fidelity

    @classmethod
    def example(cls) -> Calibration:
        """Get an example calibration."""
        return cls(0.01, 0.01, 0.01)
