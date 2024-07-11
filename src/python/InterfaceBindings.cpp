#include "python/InterfaceBindings.hpp"

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
      .def(
          "step_backward",
          [](SimulationState* self) { checkOrThrow(self->stepBackward(self)); })
      .def("step_over_backward",
           [](SimulationState* self) {
             checkOrThrow(self->stepOverBackward(self));
           })
      .def("run_simulation",
           [](SimulationState* self) {
             checkOrThrow(self->runSimulation(self));
           })
      .def("reset_simulation",
           [](SimulationState* self) {
             checkOrThrow(self->resetSimulation(self));
           })
      .def("can_step_forward",
           [](SimulationState* self) { return self->canStepForward(self); })
      .def("can_step_backward",
           [](SimulationState* self) { return self->canStepBackward(self); })
      .def("is_finished",
           [](SimulationState* self) { return self->isFinished(self); })
      .def("did_assertion_fail",
           [](SimulationState* self) { return self->didAssertionFail(self); })
      .def("get_current_instruction",
           [](SimulationState* self) {
             return self->getCurrentInstruction(self);
           })
      .def("get_previous_instruction",
           [](SimulationState* self) {
             return self->getPreviousInstruction(self);
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
      .def("get_state_vector_full",
           [](SimulationState* self) {
             size_t numQubits = self->getNumQubits(self);
             std::vector<Complex> amplitudes(1 << numQubits);
             StatevectorCPP result{numQubits, 1ULL << numQubits, amplitudes};
             Statevector output{numQubits, result.numStates,
                                result.amplitudes.data()};
             checkOrThrow(self->getStateVectorFull(self, &output));
             return result;
           })
      .def("get_state_vector_sub",
           [](SimulationState* self, std::vector<size_t> qubits) {
             size_t numQubits = qubits.size();
             std::vector<Complex> amplitudes(1 << numQubits);
             StatevectorCPP result{numQubits, 1ULL << numQubits, amplitudes};
             Statevector output{numQubits, result.numStates,
                                result.amplitudes.data()};
             checkOrThrow(self->getStateVectorSub(self, numQubits,
                                                  qubits.data(), &output));
             return result;
           })
      .def("get_data_dependencies", [](SimulationState* self,
                                       size_t instruction,
                                       std::vector<uint8_t>& instructions) {
        checkOrThrow(self->getDataDependencies(
            self, instruction, reinterpret_cast<bool*>(instructions.data())));
      });
}
