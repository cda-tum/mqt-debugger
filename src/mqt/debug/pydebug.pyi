"""Type stubs for python bindings of the debug module."""

# Enums

class VariableType:
    VarBool: VariableType
    VarInt: VariableType
    VarFloat: VariableType

# Classes
class VariableValue:
    bool_value: bool
    int_value: int
    float_value: float

    def __init__(self) -> None: ...

class Variable:
    name: str
    type: VariableType
    value: VariableValue

    def __init__(self) -> None: ...

class Complex:
    real: float
    imaginary: float

    def __init__(self) -> None: ...

class Statevector:
    num_qubits: int
    num_states: int
    amplitudes: list[Complex]

    def __init__(self) -> None: ...

class SimulationState:
    def __init__(self) -> None: ...
    def init(self) -> None: ...
    def load_code(self, code: str) -> None: ...
    def step_forward(self) -> None: ...
    def step_over_forward(self) -> None: ...
    def step_out_forward(self) -> None: ...
    def step_backward(self) -> None: ...
    def step_over_backward(self) -> None: ...
    def step_out_backward(self) -> None: ...
    def run_all(self) -> int: ...
    def run_simulation(self) -> None: ...
    def run_simulation_backward(self) -> None: ...
    def reset_simulation(self) -> None: ...
    def pause_simulation(self) -> None: ...
    def can_step_forward(self) -> bool: ...
    def can_step_backward(self) -> bool: ...
    def is_finished(self) -> bool: ...
    def did_assertion_fail(self) -> bool: ...
    def was_breakpoint_hit(self) -> bool: ...
    def get_current_instruction(self) -> int: ...
    def get_previous_instruction(self) -> int: ...
    def get_instruction_count(self) -> int: ...
    def get_instruction_position(self, instruction: int) -> tuple[int, int]: ...
    def get_num_qubits(self) -> int: ...
    def get_amplitude_index(self, qubit: int) -> Complex: ...
    def get_amplitude_bitstring(self, bitstring: str) -> Complex: ...
    def get_classical_variable(self, name: str) -> Variable: ...
    def get_num_classical_variables(self) -> int: ...
    def get_classical_variable_name(self, index: int) -> str: ...
    def get_state_vector_full(self) -> Statevector: ...
    def get_state_vector_sub(self, sub_state_size: int, qubits: list[int]) -> Statevector: ...
    def set_breakpoint(self, desired_position: int) -> int: ...
    def clear_breakpoints(self) -> None: ...
    def get_stack_depth(self) -> int: ...
    def get_stack_trace(self, max_depth: int) -> list[int]: ...
    def get_diagnostics(self) -> Diagnostics: ...

class ErrorCauseType:
    Unknown: ErrorCauseType
    MissingInteraction: ErrorCauseType
    ControlAlwaysZero: ErrorCauseType

class ErrorCause:
    instruction: int
    type: ErrorCauseType

    def __init__(self) -> None: ...

class Diagnostics:
    def __init__(self) -> None: ...
    def init(self) -> None: ...
    def get_num_qubits(self) -> int: ...
    def get_instruction_count(self) -> int: ...
    def get_data_dependencies(self, instruction: int) -> list[int]: ...
    def get_interactions(self, before_instruction: int, qubit: int) -> list[int]: ...
    def potential_error_causes(self) -> list[ErrorCause]: ...

def create_ddsim_simulation_state() -> SimulationState: ...
def destroy_ddsim_simulation_state(state: SimulationState) -> None: ...
