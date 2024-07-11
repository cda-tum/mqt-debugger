#include "python/InterfaceBindings.hpp"

#include <backend/debug.h>
#include <common.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

void bindFramework(py::module& m) {
  // Bind the Result enum
  py::enum_<Result>(m, "Result")
      .value("OK", OK)
      .value("ERROR", ERROR)
      .export_values();

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
      .def_readwrite("imaginary", &Complex::imaginary);

  // Bind the Statevector struct
  py::class_<Statevector>(m, "Statevector")
      .def(py::init<>())
      .def_readwrite("num_qubits", &Statevector::numQubits)
      .def_readwrite("num_states", &Statevector::numStates)
      .def_readwrite("amplitudes", &Statevector::amplitudes);

  py::class_<SimulationState>(m, "SimulationState")
      .def(py::init<>())
      .def("init", [](SimulationState* self) { return self->init(self); })
      .def("load_code",
           [](SimulationState* self, const char* code) {
             return self->loadCode(self, code);
           })
      .def("step_forward",
           [](SimulationState* self) { return self->stepForward(self); })
      .def("step_over_forward",
           [](SimulationState* self) { return self->stepOverForward(self); })
      .def("step_backward",
           [](SimulationState* self) { return self->stepBackward(self); })
      .def("step_over_backward",
           [](SimulationState* self) { return self->stepOverBackward(self); })
      .def("run_simulation",
           [](SimulationState* self) { return self->runSimulation(self); })
      .def("reset_simulation",
           [](SimulationState* self) { return self->resetSimulation(self); })
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
           [](SimulationState* self, size_t instruction, size_t* start,
              size_t* end) {
             return self->getInstructionPosition(self, instruction, start, end);
           })
      .def("get_num_qubits",
           [](SimulationState* self) { return self->getNumQubits(self); })
      .def("get_amplitude_index",
           [](SimulationState* self, size_t qubit, Complex* output) {
             return self->getAmplitudeIndex(self, qubit, output);
           })
      .def("get_amplitude_bitstring",
           [](SimulationState* self, const char* bitstring, Complex* output) {
             return self->getAmplitudeBitstring(self, bitstring, output);
           })
      .def("get_classical_variable",
           [](SimulationState* self, const char* name, Variable* output) {
             return self->getClassicalVariable(self, name, output);
           })
      .def("get_state_vector_full",
           [](SimulationState* self, Statevector* output) {
             return self->getStateVectorFull(self, output);
           })
      .def("get_state_vector_sub",
           [](SimulationState* self, size_t subStateSize, const size_t* qubits,
              Statevector* output) {
             return self->getStateVectorSub(self, subStateSize, qubits, output);
           })
      .def("get_data_dependencies",
           [](SimulationState* self, size_t instruction, bool* instructions) {
             return self->getDataDependencies(self, instruction, instructions);
           });
}
