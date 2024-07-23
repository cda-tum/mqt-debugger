#include "python/InterfaceBindings.hpp"

#include "backend/dd/DDSimDiagnostics.hpp"

#include <backend/debug.h>
#include <common.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

void checkOrThrow(Result result) {
  if (result != OK) {
    throw std::runtime_error("An error occurred while executing the operation");
  }
}

struct StatevectorCPP {
  size_t numQubits;
  size_t numStates;
  std::vector<Complex> amplitudes;
};

void bindFramework(py::module& m) {
  // Bind the VariableType enum
  py::enum_<VariableType>(m, "VariableType")
      .value("VarBool", VarBool)
      .value("VarInt", VarInt)
      .value("VarFloat", VarFloat)
      .export_values();

  // Bind the VariableValue union
  py::class_<VariableValue>(m, "VariableValue")
      .def(py::init<>())
      .def_readwrite("bool_value", &VariableValue::boolValue)
      .def_readwrite("int_value", &VariableValue::intValue)
      .def_readwrite("float_value", &VariableValue::floatValue);

  // Bind the Variable struct
  py::class_<Variable>(m, "Variable")
      .def(py::init<>())
      .def_readwrite("name", &Variable::name)
      .def_readwrite("type", &Variable::type)
      .def_readwrite("value", &Variable::value);

  // Bind the Complex struct
  py::class_<Complex>(m, "Complex")
      .def(py::init<>())
      .def_readwrite("real", &Complex::real)
      .def_readwrite("imaginary", &Complex::imaginary)
      .def("__str__",
           [](const Complex& self) {
             return std::to_string(self.real) + " + " +
                    std::to_string(self.imaginary) + "i";
           })
      .def("__repr__", [](const Complex& self) {
        return "Complex(" + std::to_string(self.real) + ", " +
               std::to_string(self.imaginary) + ")";
      });

  // Bind the Statevector struct
  py::class_<StatevectorCPP>(m, "Statevector")
      .def(py::init<>())
      .def_readwrite("num_qubits", &StatevectorCPP::numQubits)
      .def_readwrite("num_states", &StatevectorCPP::numStates)
      .def_readwrite("amplitudes", &StatevectorCPP::amplitudes);

  py::class_<SimulationState>(m, "SimulationState")
      .def(py::init<>())
      .def("init",
           [](SimulationState* self) { checkOrThrow(self->init(self)); })
      .def("load_code",
           [](SimulationState* self, const char* code) {
             checkOrThrow(self->loadCode(self, code));
           })
      .def("step_forward",
           [](SimulationState* self) { checkOrThrow(self->stepForward(self)); })
      .def("step_over_forward",
           [](SimulationState* self) {
             checkOrThrow(self->stepOverForward(self));
           })
      .def("step_out_forward",
           [](SimulationState* self) {
             checkOrThrow(self->stepOutForward(self));
           })
      .def(
          "step_backward",
          [](SimulationState* self) { checkOrThrow(self->stepBackward(self)); })
      .def("step_over_backward",
           [](SimulationState* self) {
             checkOrThrow(self->stepOverBackward(self));
           })
      .def("step_out_backward",
           [](SimulationState* self) {
             checkOrThrow(self->stepOutBackward(self));
           })
      .def("run_all",
           [](SimulationState* self) {
             size_t errors = 0;
             checkOrThrow(self->runAll(self, &errors));
             return errors;
           })
      .def("run_simulation",
           [](SimulationState* self) {
             checkOrThrow(self->runSimulation(self));
           })
      .def("run_simulation_backward",
           [](SimulationState* self) {
             checkOrThrow(self->runSimulationBackward(self));
           })
      .def("reset_simulation",
           [](SimulationState* self) {
             checkOrThrow(self->resetSimulation(self));
           })
      .def("pause_simulation",
           [](SimulationState* self) {
             checkOrThrow(self->pauseSimulation(self));
           })
      .def("can_step_forward",
           [](SimulationState* self) { return self->canStepForward(self); })
      .def("can_step_backward",
           [](SimulationState* self) { return self->canStepBackward(self); })
      .def("is_finished",
           [](SimulationState* self) { return self->isFinished(self); })
      .def("did_assertion_fail",
           [](SimulationState* self) { return self->didAssertionFail(self); })
      .def("was_breakpoint_hit",
           [](SimulationState* self) { return self->wasBreakpointHit(self); })
      .def("get_current_instruction",
           [](SimulationState* self) {
             return self->getCurrentInstruction(self);
           })
      .def(
          "get_instruction_count",
          [](SimulationState* self) { return self->getInstructionCount(self); })
      .def("get_instruction_position",
           [](SimulationState* self, size_t instruction) {
             size_t start = 0;
             size_t end = 0;
             checkOrThrow(
                 self->getInstructionPosition(self, instruction, &start, &end));
             return std::make_pair(start, end);
           })
      .def("get_num_qubits",
           [](SimulationState* self) { return self->getNumQubits(self); })
      .def("get_amplitude_index",
           [](SimulationState* self, size_t qubit) {
             Complex output;
             checkOrThrow(self->getAmplitudeIndex(self, qubit, &output));
             return output;
           })
      .def("get_amplitude_bitstring",
           [](SimulationState* self, const char* bitstring) {
             Complex output;
             checkOrThrow(
                 self->getAmplitudeBitstring(self, bitstring, &output));
             return output;
           })
      .def("get_classical_variable",
           [](SimulationState* self, const char* name) {
             Variable output;
             checkOrThrow(self->getClassicalVariable(self, name, &output));
             return output;
           })
      .def("get_num_classical_variables",
           [](SimulationState* self) {
             return self->getNumClassicalVariables(self);
           })
      .def("get_classical_variable_name",
           [](SimulationState* self, size_t variableIndex) {
             std::string output(255, '\0');
             checkOrThrow(self->getClassicalVariableName(self, variableIndex,
                                                         output.data()));
             const std::size_t pos = output.find_first_of('\0');
             if (pos != std::string::npos) {
               output = output.substr(0, pos);
             }
             return output;
           })
      .def("get_state_vector_full",
           [](SimulationState* self) {
             const size_t numQubits = self->getNumQubits(self);
             const std::vector<Complex> amplitudes(1 << numQubits);
             StatevectorCPP result{numQubits, 1ULL << numQubits, amplitudes};
             Statevector output{numQubits, result.numStates,
                                result.amplitudes.data()};
             checkOrThrow(self->getStateVectorFull(self, &output));
             return result;
           })
      .def("get_state_vector_sub",
           [](SimulationState* self, std::vector<size_t> qubits) {
             const size_t numQubits = qubits.size();
             const std::vector<Complex> amplitudes(1 << numQubits);
             StatevectorCPP result{numQubits, 1ULL << numQubits, amplitudes};
             Statevector output{numQubits, result.numStates,
                                result.amplitudes.data()};
             checkOrThrow(self->getStateVectorSub(self, numQubits,
                                                  qubits.data(), &output));
             return result;
           })
      .def("set_breakpoint",
           [](SimulationState* self, size_t desiredPosition) {
             size_t actualPosition;
             checkOrThrow(
                 self->setBreakpoint(self, desiredPosition, &actualPosition));
             return actualPosition;
           })
      .def("clear_breakpoints",
           [](SimulationState* self) {
             checkOrThrow(self->clearBreakpoints(self));
           })
      .def("get_stack_depth",
           [](SimulationState* self) {
             size_t depth = 0;
             checkOrThrow(self->getStackDepth(self, &depth));
             return depth;
           })
      .def("get_stack_trace",
           [](SimulationState* self, size_t maxDepth) {
             size_t trueSize = 0;
             checkOrThrow(self->getStackDepth(self, &trueSize));
             const size_t stackSize = std::min(maxDepth, trueSize);
             std::vector<size_t> stackTrace(stackSize);
             checkOrThrow(
                 self->getStackTrace(self, maxDepth, stackTrace.data()));
             return stackTrace;
           })
      .def(
          "get_diagnostics",
          [](SimulationState* self) { return self->getDiagnostics(self); },
          py::return_value_policy::reference_internal);
}

void bindDiagnostics(py::module& m) {
  // Bind the ErrorCauseType enum
  py::enum_<ErrorCauseType>(m, "ErrorCauseType")
      .value("Unknown", Unknown)
      .value("MissingInteraction", MissingInteraction)
      .value("ControlAlwaysZero", ControlAlwaysZero)
      .export_values();

  // Bind the ErrorCause struct
  py::class_<ErrorCause>(m, "ErrorCause")
      .def(py::init<>())
      .def_readwrite("instruction", &ErrorCause ::instruction)
      .def_readwrite("type", &ErrorCause ::type);

  py::class_<Diagnostics>(m, "Diagnostics")
      .def(py::init<>())
      .def("init", [](Diagnostics* self) { checkOrThrow(self->init(self)); })
      .def("get_num_qubits",
           [](Diagnostics* self) { return self->getNumQubits(self); })
      .def("get_instruction_count",
           [](Diagnostics* self) { return self->getInstructionCount(self); })
      .def("get_data_dependencies",
           [](Diagnostics* self, size_t instruction) {
             std::vector<uint8_t> instructions(self->getInstructionCount(self));
             checkOrThrow(self->getDataDependencies(
                 self, instruction,
                 reinterpret_cast<bool*>(instructions.data())));
             std::vector<size_t> result;
             for (size_t i = 0; i < instructions.size(); i++) {
               if (instructions[i] != 0) {
                 result.push_back(i);
               }
             }
             return result;
           })
      .def("get_interactions",
           [](Diagnostics* self, size_t beforeInstruction, size_t qubit) {
             std::vector<uint8_t> qubits(self->getNumQubits(self));
             checkOrThrow(
                 self->getInteractions(self, beforeInstruction, qubit,
                                       reinterpret_cast<bool*>(qubits.data())));
             std::vector<size_t> result;
             for (size_t i = 0; i < qubits.size(); i++) {
               if (qubits[i] != 0) {
                 result.push_back(i);
               }
             }
             return result;
           })
      .def("potential_error_causes", [](Diagnostics* self) {
        size_t nextSize = 10;
        while (true) {
          std::vector<ErrorCause> output(nextSize);
          const auto actualSize =
              self->potentialErrorCauses(self, output.data(), nextSize);
          if (actualSize <= nextSize) {
            output.resize(actualSize);
            return output;
          }
          nextSize = nextSize * 2;
        }
      });
}
