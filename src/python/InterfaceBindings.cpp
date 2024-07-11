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
      .def_readwrite("init", &SimulationState::init)
      .def_readwrite("load_code", &SimulationState::loadCode)
      .def_readwrite("step_forward", &SimulationState::stepForward)
      .def_readwrite("step_over_forward", &SimulationState::stepOverForward)
      .def_readwrite("step_backward", &SimulationState::stepBackward)
      .def_readwrite("step_over_backward", &SimulationState::stepOverBackward)
      .def_readwrite("run_simulation", &SimulationState::runSimulation)
      .def_readwrite("reset_simulation", &SimulationState::resetSimulation)
      .def_readwrite("can_step_forward", &SimulationState::canStepForward)
      .def_readwrite("can_step_backward", &SimulationState::canStepBackward)
      .def_readwrite("is_finished", &SimulationState::isFinished)
      .def_readwrite("did_assertion_fail", &SimulationState::didAssertionFail)
      .def_readwrite("get_current_instruction",
                     &SimulationState::getCurrentInstruction)
      .def_readwrite("get_previous_instruction",
                     &SimulationState::getPreviousInstruction)
      .def_readwrite("get_instruction_count",
                     &SimulationState::getInstructionCount)
      .def_readwrite("get_instruction_position",
                     &SimulationState::getInstructionPosition)
      .def_readwrite("get_num_qubits", &SimulationState::getNumQubits)
      .def_readwrite("get_amplitude_index", &SimulationState::getAmplitudeIndex)
      .def_readwrite("get_amplitude_bitstring",
                     &SimulationState::getAmplitudeBitstring)
      .def_readwrite("get_classical_variable",
                     &SimulationState::getClassicalVariable)
      .def_readwrite("get_state_vector_full",
                     &SimulationState::getStateVectorFull)
      .def_readwrite("get_state_vector_sub",
                     &SimulationState::getStateVectorSub)
      .def_readwrite("get_data_dependencies",
                     &SimulationState::getDataDependencies)
      .def("call_init", [](SimulationState* self) { return self->init(self); })
      .def("call_load_code",
           [](SimulationState* self, const char* code) {
             return self->loadCode(self, code);
           })
      .def("call_step_forward",
           [](SimulationState* self) { return self->stepForward(self); })
      .def("call_step_over_forward",
           [](SimulationState* self) { return self->stepOverForward(self); })
      .def("call_step_backward",
           [](SimulationState* self) { return self->stepBackward(self); })
      .def("call_step_over_backward",
           [](SimulationState* self) { return self->stepOverBackward(self); })
      .def("call_run_simulation",
           [](SimulationState* self) { return self->runSimulation(self); })
      .def("call_reset_simulation",
           [](SimulationState* self) { return self->resetSimulation(self); })
      .def("call_can_step_forward",
           [](SimulationState* self) { return self->canStepForward(self); })
      .def("call_can_step_backward",
           [](SimulationState* self) { return self->canStepBackward(self); })
      .def("call_is_finished",
           [](SimulationState* self) { return self->isFinished(self); })
      .def("call_did_assertion_fail",
           [](SimulationState* self) { return self->didAssertionFail(self); })
      .def("call_get_current_instruction",
           [](SimulationState* self) {
             return self->getCurrentInstruction(self);
           })
      .def("call_get_previous_instruction",
           [](SimulationState* self) {
             return self->getPreviousInstruction(self);
           })
      .def(
          "call_get_instruction_count",
          [](SimulationState* self) { return self->getInstructionCount(self); })
      .def("call_get_instruction_position",
           [](SimulationState* self, size_t instruction, size_t* start,
              size_t* end) {
             return self->getInstructionPosition(self, instruction, start, end);
           })
      .def("call_get_num_qubits",
           [](SimulationState* self) { return self->getNumQubits(self); })
      .def("call_get_amplitude_index",
           [](SimulationState* self, size_t qubit, Complex* output) {
             return self->getAmplitudeIndex(self, qubit, output);
           })
      .def("call_get_amplitude_bitstring",
           [](SimulationState* self, const char* bitstring, Complex* output) {
             return self->getAmplitudeBitstring(self, bitstring, output);
           })
      .def("call_get_classical_variable",
           [](SimulationState* self, const char* name, Variable* output) {
             return self->getClassicalVariable(self, name, output);
           })
      .def("call_get_state_vector_full",
           [](SimulationState* self, Statevector* output) {
             return self->getStateVectorFull(self, output);
           })
      .def("call_get_state_vector_sub",
           [](SimulationState* self, size_t subStateSize, const size_t* qubits,
              Statevector* output) {
             return self->getStateVectorSub(self, subStateSize, qubits, output);
           })
      .def("call_get_data_dependencies",
           [](SimulationState* self, size_t instruction, bool* instructions) {
             return self->getDataDependencies(self, instruction, instructions);
           });
}
