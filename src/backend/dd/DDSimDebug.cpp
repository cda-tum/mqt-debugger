#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-reinterpret-cast"

#include "backend/dd/DDSimDebug.hpp"

#include "CircuitOptimizer.hpp"
#include "backend/debug.h"
#include "common.h"
#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/CodePreprocessing.hpp"
#include "common/parsing/ParsingError.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <utility>

double generateRandomNumber() {
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(0.0, 1.0);

  return dis(gen);
}

#pragma clang diagnostic push
Result createDDSimulationState(DDSimulationState* self) {
  self->interface.init = ddsimInit;

  self->interface.loadCode = ddsimLoadCode;
  self->interface.stepForward = ddsimStepForward;
  self->interface.stepBackward = ddsimStepBackward;
  self->interface.stepOverForward = ddsimStepOverForward;
  self->interface.stepOverBackward = ddsimStepOverBackward;
  self->interface.stepOutForward = ddsimStepOutForward;
  self->interface.stepOutBackward = ddsimStepOutBackward;
  self->interface.runSimulation = ddsimRunSimulation;
  self->interface.runSimulationBackward = ddsimRunSimulationBackward;
  self->interface.resetSimulation = ddsimResetSimulation;
  self->interface.pauseSimulation = ddsimPauseSimulation;
  self->interface.canStepForward = ddsimCanStepForward;
  self->interface.canStepBackward = ddsimCanStepBackward;
  self->interface.isFinished = ddsimIsFinished;
  self->interface.didAssertionFail = ddsimDidAssertionFail;
  self->interface.wasBreakpointHit = ddsimWasBreakpointHit;

  self->interface.getCurrentInstruction = ddsimGetCurrentInstruction;
  self->interface.getPreviousInstruction = ddsimGetPreviousInstruction;
  self->interface.getInstructionCount = ddsimGetInstructionCount;
  self->interface.getInstructionPosition = ddsimGetInstructionPosition;
  self->interface.getNumQubits = ddsimGetNumQubits;
  self->interface.getAmplitudeIndex = ddsimGetAmplitudeIndex;
  self->interface.getAmplitudeBitstring = ddsimGetAmplitudeBitstring;
  self->interface.getClassicalVariable = ddsimGetClassicalVariable;
  self->interface.getNumClassicalVariables = ddsimGetNumClassicalVariables;
  self->interface.getClassicalVariableName = ddsimGetClassicalVariableName;
  self->interface.getStateVectorFull = ddsimGetStateVectorFull;
  self->interface.getStateVectorSub = ddsimGetStateVectorSub;
  self->interface.getDataDependencies = ddsimGetDataDependencies;
  self->interface.setBreakpoint = ddsimSetBreakpoint;
  self->interface.clearBreakpoints = ddsimClearBreakpoints;

  return self->interface.init(reinterpret_cast<SimulationState*>(self));
}
#pragma clang diagnostic pop

void resetSimulationState(DDSimulationState* ddsim) {
  if (ddsim->simulationState.p != nullptr) {
    ddsim->dd->decRef(ddsim->simulationState);
  }
  ddsim->simulationState = ddsim->dd->makeZeroState(ddsim->qc->getNqubits());
  ddsim->dd->incRef(ddsim->simulationState);
  ddsim->breakpoints.clear();
  ddsim->paused = false;
}

Result ddsimInit(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);

  ddsim->simulationState.p = nullptr;
  ddsim->qc = std::make_unique<qc::QuantumComputation>();
  ddsim->dd = std::make_unique<dd::Package<>>(1);
  ddsim->iterator = ddsim->qc->begin();
  ddsim->currentInstruction = 0;
  ddsim->previousInstructionStack.clear();
  ddsim->callReturnStack.clear();
  ddsim->callSubstitutions.clear();
  ddsim->restoreCallReturnStack.clear();
  ddsim->breakpoints.clear();
  ddsim->lastFailedAssertion = -1ULL;
  ddsim->lastMetBreakpoint = -1ULL;

  resetSimulationState(ddsim);

  return OK;
}

Result ddsimLoadCode(SimulationState* self, const char* code) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  ddsim->currentInstruction = 0;
  ddsim->previousInstructionStack.clear();
  ddsim->callReturnStack.clear();
  ddsim->callSubstitutions.clear();
  ddsim->restoreCallReturnStack.clear();
  ddsim->code = code;

  try {
    std::stringstream ss{preprocessAssertionCode(code, ddsim)};
    ddsim->qc->import(ss, qc::Format::OpenQASM3);
    qc::CircuitOptimizer::flattenOperations(*ddsim->qc);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return ERROR;
  }

  ddsim->iterator = ddsim->qc->begin();
  ddsim->dd->resize(ddsim->qc->getNqubits());
  ddsim->lastFailedAssertion = -1ULL;
  ddsim->lastMetBreakpoint = -1ULL;

  resetSimulationState(ddsim);

  return OK;
}

Result ddsimStepOverForward(SimulationState* self) {
  if (!self->canStepForward(self)) {
    return ERROR;
  }
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (ddsim->instructionTypes[ddsim->currentInstruction] != CALL) {
    return self->stepForward(self);
  }

  Result res = OK;
  const auto currentInstruction = ddsim->currentInstruction;
  bool done = false;
  while ((res == OK) && !done) {
    if (ddsim->paused) {
      ddsim->paused = false;
      return OK;
    }
    if (ddsim->instructionTypes[ddsim->currentInstruction] == RETURN &&
        ddsim->callReturnStack.back() == currentInstruction) {
      done = true;
    }
    res = self->stepForward(self);
    if (self->didAssertionFail(self) || self->wasBreakpointHit(self)) {
      break;
    }
  }
  return res;
}

Result ddsimStepOverBackward(SimulationState* self) {
  if (!self->canStepBackward(self)) {
    return ERROR;
  }
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  const auto prev = self->getPreviousInstruction(self);
  if (ddsim->instructionTypes[prev] != RETURN) {
    return self->stepBackward(self);
  }

  Result res = OK;
  const auto stackSize = ddsim->callReturnStack.size();
  while (res == OK) {
    if (ddsim->paused) {
      ddsim->paused = false;
      return OK;
    }
    res = self->stepBackward(self);
    if (ddsim->instructionTypes[ddsim->currentInstruction] == CALL &&
        ddsim->callReturnStack.size() == stackSize) {
      break;
    }
    if (self->wasBreakpointHit(self)) {
      break;
    }
  }
  return res;
}

Result ddsimStepOutForward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (ddsim->callReturnStack.empty()) {
    return self->runSimulation(self);
  }

  const auto size = ddsim->callReturnStack.size();
  Result res = OK;
  while ((res = self->stepForward(self)) == OK) {
    if (self->didAssertionFail(self) || self->wasBreakpointHit(self)) {
      break;
    }
    if (ddsim->paused) {
      ddsim->paused = false;
      return OK;
    }
    if (ddsim->callReturnStack.size() == size - 1) {
      break;
    }
  }
  return res;
}

Result ddsimStepOutBackward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (ddsim->callReturnStack.empty()) {
    return self->runSimulationBackward(self);
  }

  const auto size = ddsim->callReturnStack.size();
  Result res = OK;
  while ((res = self->stepBackward(self)) == OK) {
    if (self->wasBreakpointHit(self)) {
      break;
    }
    if (self->wasBreakpointHit(self)) {
      break;
    }
    if (ddsim->paused) {
      ddsim->paused = false;
      return OK;
    }
    if (ddsim->callReturnStack.size() == size - 1) {
      break;
    }
  }
  return res;
}

Result ddsimStepForward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (!self->canStepForward(self)) {
    return ERROR;
  }
  ddsim->lastMetBreakpoint = -1ULL;
  const auto currentInstruction = ddsim->currentInstruction;
  ddsim->currentInstruction = ddsim->successorInstructions[currentInstruction];
  if (ddsim->breakpoints.contains(ddsim->currentInstruction)) {
    ddsim->lastMetBreakpoint = ddsim->currentInstruction;
  }

  if (ddsim->currentInstruction == 0) {
    ddsim->currentInstruction = ddsim->callReturnStack.back() + 1;
    ddsim->restoreCallReturnStack.emplace_back(ddsim->currentInstruction,
                                               ddsim->callReturnStack.back());
    ddsim->callReturnStack.pop_back();
  }
  if (ddsim->instructionTypes[currentInstruction] == CALL) {
    ddsim->callReturnStack.push_back(currentInstruction);
  }
  ddsim->previousInstructionStack.emplace_back(currentInstruction);

  if (ddsim->instructionTypes[currentInstruction] == ASSERTION) {
    auto& assertion = ddsim->assertionInstructions[currentInstruction];
    const auto failed = !checkAssertion(ddsim, assertion);
    if (failed && ddsim->lastFailedAssertion != currentInstruction) {
      ddsim->lastFailedAssertion = currentInstruction;
      self->stepBackward(self);
    }
    return OK;
  }

  ddsim->lastFailedAssertion = -1ULL;
  if (ddsim->instructionTypes[currentInstruction] != SIMULATE) {
    return OK;
  }

  qc::MatrixDD currDD;
  if ((*ddsim->iterator)->getType() == qc::Measure) {
    auto qubitsToMeasure = (*ddsim->iterator)->getTargets();
    auto classicalBits =
        dynamic_cast<qc::NonUnitaryOperation*>(ddsim->iterator->get())
            ->getClassics();
    for (size_t i = 0; i < qubitsToMeasure.size(); i++) {
      auto qubit = qubitsToMeasure[i];
      auto classicalBit = classicalBits[i];

      auto [pZero, pOne] = ddsim->dd->determineMeasurementProbabilities(
          ddsim->simulationState, static_cast<dd::Qubit>(qubit), true);
      auto rnd = generateRandomNumber();
      auto result = rnd < pZero;
      ddsim->dd->performCollapsingMeasurement(ddsim->simulationState,
                                              static_cast<dd::Qubit>(qubit),
                                              result ? pZero : pOne, result);
      auto name = getClassicalBitName(ddsim, classicalBit);
      if (ddsim->variables.find(name) != ddsim->variables.end()) {
        VariableValue value;
        value.boolValue = !result;
        ddsim->variables[name].value = value;
      } else {
        ddsim->variableNames.push_back(std::make_unique<std::string>(name));
        const Variable newVariable{ddsim->variableNames.back()->data(),
                                   VariableType::VarBool,
                                   {!result}};
        ddsim->variables.insert({name, newVariable});
      }
    }

    ddsim->iterator++;
    ddsim->previousInstructionStack.clear();
    ddsim->restoreCallReturnStack.clear();
    return OK;
  }
  if ((*ddsim->iterator)->getType() == qc::Reset) {
    auto qubitsToMeasure = (*ddsim->iterator)->getTargets();
    ddsim->iterator++;
    ddsim->previousInstructionStack.clear();
    ddsim->restoreCallReturnStack.clear();

    for (const auto qubit : qubitsToMeasure) {
      auto [pZero, pOne] = ddsim->dd->determineMeasurementProbabilities(
          ddsim->simulationState, static_cast<dd::Qubit>(qubit), true);
      auto rnd = generateRandomNumber();
      auto result = rnd < pZero;
      ddsim->dd->performCollapsingMeasurement(ddsim->simulationState,
                                              static_cast<dd::Qubit>(qubit),
                                              result ? pZero : pOne, result);
      if (!result) {
        const auto x = qc::StandardOperation(qubit, qc::X);
        auto tmp = ddsim->dd->multiply(dd::getDD(&x, *ddsim->dd),
                                       ddsim->simulationState);
        ddsim->dd->incRef(tmp);
        ddsim->dd->decRef(ddsim->simulationState);
        ddsim->simulationState = tmp;
      }
    }
    return OK;
  }
  if ((*ddsim->iterator)->getType() == qc::Barrier) {
    ddsim->iterator++;
    return OK;
  }
  if ((*ddsim->iterator)->isClassicControlledOperation()) {
    const auto* op =
        dynamic_cast<qc::ClassicControlledOperation*>((*ddsim->iterator).get());
    const auto& controls = op->getControlRegister();
    const auto& exp = op->getExpectedValue();
    size_t registerValue = 0;
    for (size_t i = 0; i < controls.second; i++) {
      const auto name = getClassicalBitName(ddsim, controls.first + i);
      const auto& value = ddsim->variables[name].value.boolValue;
      registerValue |= (value ? 1ULL : 0ULL) << i;
    }
    if (registerValue == exp) {
      currDD = dd::getDD(ddsim->iterator->get(), *ddsim->dd);
    } else {
      currDD = ddsim->dd->makeIdent();
    }
  } else {
    currDD = dd::getDD(ddsim->iterator->get(),
                       *ddsim->dd); // retrieve the "new" current operation
  }

  auto temp = ddsim->dd->multiply(currDD, ddsim->simulationState);
  ddsim->dd->incRef(temp);
  ddsim->dd->decRef(ddsim->simulationState);
  ddsim->simulationState = temp;
  ddsim->dd->garbageCollect();

  ddsim->iterator++;
  return OK;
}

Result ddsimStepBackward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (!self->canStepBackward(self)) {
    return ERROR;
  }

  ddsim->lastMetBreakpoint = -1ULL;
  if (!ddsim->restoreCallReturnStack.empty() &&
      ddsim->currentInstruction == ddsim->restoreCallReturnStack.back().first) {
    ddsim->callReturnStack.push_back(
        ddsim->restoreCallReturnStack.back().second);
    ddsim->restoreCallReturnStack.pop_back();
  }

  ddsim->currentInstruction = ddsim->previousInstructionStack.back();
  ddsim->previousInstructionStack.pop_back();

  if (!ddsim->callReturnStack.empty() &&
      ddsim->currentInstruction == ddsim->callReturnStack.back()) {
    ddsim->callReturnStack.pop_back();
  }

  // When going backwards, we still run the instruction that hits the breakpoint
  // because we want to stop *before* it.
  if (ddsim->breakpoints.contains(ddsim->currentInstruction)) {
    ddsim->lastMetBreakpoint = ddsim->currentInstruction;
  }

  if (ddsim->lastFailedAssertion != ddsim->currentInstruction) {
    ddsim->lastFailedAssertion = -1ULL;
  }

  if (ddsim->instructionTypes[ddsim->currentInstruction] != SIMULATE) {
    return OK;
  }

  ddsim->iterator--;
  qc::MatrixDD currDD;

  if ((*ddsim->iterator)->getType() == qc::Barrier) {
    return OK;
  }
  if ((*ddsim->iterator)->isClassicControlledOperation()) {
    const auto* op =
        dynamic_cast<qc::ClassicControlledOperation*>((*ddsim->iterator).get());
    const auto& controls = op->getControlRegister();
    const auto& exp = op->getExpectedValue();
    size_t registerValue = 0;
    for (size_t i = 0; i < controls.second; i++) {
      const auto name = getClassicalBitName(ddsim, controls.first + i);
      const auto& value = ddsim->variables[name].value.boolValue;
      registerValue |= (value ? 1ULL : 0ULL) << i;
    }
    if (registerValue == exp) {
      currDD = dd::getInverseDD(ddsim->iterator->get(), *ddsim->dd);
    } else {
      currDD = ddsim->dd->makeIdent();
    }
  } else {
    currDD = dd::getInverseDD(
        ddsim->iterator->get(),
        *ddsim->dd); // get the inverse of the current operation
  }

  auto temp = ddsim->dd->multiply(currDD, ddsim->simulationState);
  ddsim->dd->incRef(temp);
  ddsim->dd->decRef(ddsim->simulationState);
  ddsim->simulationState = temp;
  ddsim->dd->garbageCollect();

  return OK;
}

Result ddsimRunSimulation(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  while (!self->isFinished(self)) {
    if (ddsim->paused) {
      ddsim->paused = false;
      return OK;
    }
    Result res = self->stepForward(self);
    if (res != OK) {
      return res;
    }
    if (self->didAssertionFail(self) || self->wasBreakpointHit(self)) {
      break;
    }
  }
  return OK;
}

Result ddsimRunSimulationBackward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  while (self->canStepBackward(self)) {
    if (ddsim->paused) {
      ddsim->paused = false;
      return OK;
    }
    Result res = self->stepBackward(self);
    if (res != OK) {
      return res;
    }
    if (self->didAssertionFail(self) || self->wasBreakpointHit(self)) {
      break;
    }
  }
  return OK;
}

Result ddsimResetSimulation(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  ddsim->currentInstruction = 0;
  ddsim->previousInstructionStack.clear();
  ddsim->callReturnStack.clear();
  ddsim->callSubstitutions.clear();
  ddsim->restoreCallReturnStack.clear();

  ddsim->iterator = ddsim->qc->begin();
  ddsim->lastFailedAssertion = -1ULL;
  ddsim->lastMetBreakpoint = -1ULL;
  ddsim->variables.clear();
  ddsim->variableNames.clear();

  resetSimulationState(ddsim);
  return OK;
}

Result ddsimPauseSimulation(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  ddsim->paused = true;
  return OK;
}

bool ddsimCanStepForward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentInstruction < ddsim->instructionTypes.size();
}

bool ddsimCanStepBackward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return !ddsim->previousInstructionStack.empty();
}

bool ddsimIsFinished(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentInstruction == ddsim->instructionTypes.size();
}

bool ddsimDidAssertionFail(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->lastFailedAssertion == ddsim->currentInstruction;
}

bool ddsimWasBreakpointHit(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->lastMetBreakpoint == ddsim->currentInstruction;
}

size_t ddsimGetCurrentInstruction(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentInstruction;
}

size_t ddsimGetPreviousInstruction(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->previousInstructionStack.back();
}

size_t ddsimGetInstructionCount(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->instructionTypes.size();
}

Result ddsimGetInstructionPosition(SimulationState* self, size_t instruction,
                                   size_t* start, size_t* end) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (instruction >= ddsim->instructionStarts.size()) {
    return ERROR;
  }
  size_t start_index = ddsim->instructionStarts[instruction];
  size_t end_index = ddsim->instructionEnds[instruction];

  while (ddsim->code[start_index] == ' ' || ddsim->code[start_index] == '\n' ||
         ddsim->code[start_index] == '\r' || ddsim->code[start_index] == '\t') {
    start_index++;
  }
  while (ddsim->code[end_index] == ' ' || ddsim->code[end_index] == '\n' ||
         ddsim->code[end_index] == '\r' || ddsim->code[end_index] == '\t') {
    end_index++;
  }
  *start = start_index;
  *end = end_index;
  return OK;
}

size_t ddsimGetNumQubits(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->qc->getNqubits();
}

Result ddsimGetAmplitudeIndex(SimulationState* self, size_t qubit,
                              Complex* output) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  auto result = ddsim->simulationState.getValueByIndex(qubit);
  output->real = result.real();
  output->imaginary = result.imag();
  return OK;
}

Result ddsimGetAmplitudeBitstring(SimulationState* self, const char* bitstring,
                                  Complex* output) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  auto path = std::string(bitstring);
  std::reverse(path.begin(), path.end());
  auto result =
      ddsim->simulationState.getValueByPath(ddsim->qc->getNqubits(), path);
  output->real = result.real();
  output->imaginary = result.imag();
  return OK;
}

Result ddsimGetClassicalVariable(SimulationState* self, const char* name,
                                 Variable* output) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (ddsim->variables.find(name) != ddsim->variables.end()) {
    *output = ddsim->variables[name];
    return OK;
  }
  return ERROR;
}
size_t ddsimGetNumClassicalVariables(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->variables.size();
}
Result ddsimGetClassicalVariableName(SimulationState* self,
                                     size_t variableIndex, char* output) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);

  if (variableIndex >= ddsim->variables.size()) {
    return ERROR;
  }

  const auto name = getClassicalBitName(ddsim, variableIndex);
  strcpy(output, name.c_str());
  return OK;
}

Result ddsimGetStateVectorFull(SimulationState* self, Statevector* output) {
  const std::span<Complex> amplitudes(output->amplitudes, output->numStates);
  for (size_t i = 0; i < output->numStates; i++) {
    self->getAmplitudeIndex(self, i, &amplitudes[i]);
  }
  return OK;
}

Result ddsimGetStateVectorSub(SimulationState* self, size_t subStateSize,
                              const size_t* qubits, Statevector* output) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  Statevector fullState;
  fullState.numQubits = ddsim->qc->getNqubits();
  fullState.numStates = 1 << fullState.numQubits;
  std::vector<Complex> amplitudes(fullState.numStates);
  const std::span<Complex> outAmplitudes(output->amplitudes, output->numStates);
  const std::span<const size_t> qubitsSpan(qubits, subStateSize);
  fullState.amplitudes = amplitudes.data();

  self->getStateVectorFull(self, &fullState);

  for (auto& amplitude : outAmplitudes) {
    amplitude.real = 0;
    amplitude.imaginary = 0;
  }

  for (size_t i = 0; i < fullState.numStates; i++) {
    size_t outputIndex = 0;
    for (size_t j = 0; j < subStateSize; j++) {
      outputIndex |= ((i >> qubitsSpan[j]) & 1) << j;
    }
    outAmplitudes[outputIndex].real += amplitudes[i].real;
    outAmplitudes[outputIndex].imaginary += amplitudes[i].imaginary;
  }

  return OK;
}

Result ddsimGetDataDependencies(SimulationState* self, size_t instruction,
                                bool* instructions) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  const std::span<bool> isDependency(
      instructions, ddsim->interface.getInstructionCount(&ddsim->interface));
  std::set<size_t> toVisit{instruction};
  std::set<size_t> visited;
  while (!toVisit.empty()) {
    auto current = *toVisit.begin();
    isDependency[current] = true;
    toVisit.erase(toVisit.begin());
    visited.insert(current);
    for (auto dep : ddsim->dataDependencies[current]) {
      if (visited.find(dep) == visited.end()) {
        toVisit.insert(dep);
      }
    }
  }

  return OK;
}

Result ddsimSetBreakpoint(SimulationState* self, size_t desiredPosition,
                          size_t* targetInstruction) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  for (auto i = 0ULL; i < ddsim->instructionTypes.size(); i++) {
    size_t start = ddsim->instructionStarts[i];
    size_t end = ddsim->instructionEnds[i];
    if (desiredPosition >= start && desiredPosition <= end) {
      *targetInstruction = i;
      ddsim->breakpoints.insert(i);
      return OK;
    }
  }
  return ERROR;
}

Result ddsimClearBreakpoints(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  ddsim->breakpoints.clear();
  return OK;
}

Result destroyDDSimulationState([[maybe_unused]] DDSimulationState* self) {
  return OK;
}

//-----------------------------------------------------------------------------------------
size_t variableToQubit(DDSimulationState* ddsim, std::string& variable) {
  auto declaration = replaceString(variable, " ", "");
  declaration = replaceString(declaration, "\t", "");
  std::string var;
  size_t idx = 0;
  if (declaration.find('[') != std::string::npos) {
    auto parts = splitString(declaration, '[');
    var = parts[0];
    idx = std::stoul(parts[1].substr(0, parts[1].size() - 1));
  } else {
    var = declaration;
  }

  for (size_t i = ddsim->callReturnStack.size() - 1; i != -1ULL; i--) {
    const auto call = ddsim->callReturnStack[i];
    if (ddsim->callSubstitutions[call].find(var) ==
        ddsim->callSubstitutions[call].end()) {
      continue;
    }
    var = ddsim->callSubstitutions[call][var];
    if (var.find('[') != std::string::npos) {
      auto parts = splitString(var, '[');
      var = parts[0];
      idx = std::stoul(parts[1].substr(0, parts[1].size() - 1));
    }
  }

  for (auto& reg : ddsim->qubitRegisters) {
    if (reg.name == var) {
      if (idx >= reg.size) {
        throw std::runtime_error("Index out of bounds");
      }
      return reg.index + idx;
    }
  }
  throw std::runtime_error("Unknown variable name " + var);
}

double complexMagnitude(Complex& c) {
  return std::sqrt(c.real * c.real + c.imaginary * c.imaginary);
}

double dotProduct(const Statevector& sv1, const Statevector& sv2) {
  double resultReal = 0;
  double resultImag = 0;

  const std::span<Complex> amplitudes1(sv1.amplitudes, sv1.numStates);
  const std::span<Complex> amplitudes2(sv2.amplitudes, sv2.numStates);

  for (size_t i = 0; i < sv1.numStates; i++) {
    resultReal += amplitudes1[i].real * amplitudes2[i].real -
                  amplitudes1[i].imaginary * amplitudes2[i].imaginary;
    resultImag += amplitudes1[i].real * amplitudes2[i].imaginary +
                  amplitudes1[i].imaginary * amplitudes2[i].real;
  }
  Complex result{resultReal, resultImag};
  return complexMagnitude(result);
}

bool areQubitsEntangled(Statevector* sv) {
  const double epsilon = 0.0001;
  const std::span<Complex> amplitudes(sv->amplitudes, sv->numStates);
  const bool canBe00 = complexMagnitude(amplitudes[0]) > epsilon;
  const bool canBe01 = complexMagnitude(amplitudes[1]) > epsilon;
  const bool canBe10 = complexMagnitude(amplitudes[2]) > epsilon;
  const bool canBe11 = complexMagnitude(amplitudes[3]) > epsilon;

  return (canBe00 && canBe11 && !(canBe01 && canBe10)) ||
         (canBe01 && canBe10 && !(canBe00 && canBe11));
}

bool checkAssertionEntangled(
    DDSimulationState* ddsim,
    std::unique_ptr<EntanglementAssertion>& assertion) {
  std::vector<size_t> qubits;
  for (auto variable : assertion->getTargetQubits()) {
    qubits.push_back(variableToQubit(ddsim, variable));
  }

  Statevector sv;
  sv.numQubits = 2;
  sv.numStates = 4;
  std::vector<Complex> amplitudes(4);
  sv.amplitudes = amplitudes.data();
  std::array<size_t, 2> qubitsStep = {0, 0};

  bool result = true;
  for (size_t i = 0; i < qubits.size() && result; i++) {
    for (size_t j = 0; j < qubits.size(); j++) {
      if (i == j) {
        continue;
      }
      qubitsStep[0] = qubits[i];
      qubitsStep[1] = qubits[j];
      ddsim->interface.getStateVectorSub(&ddsim->interface, 2,
                                         qubitsStep.data(), &sv);
      if (!areQubitsEntangled(&sv)) {
        result = false;
        break;
      }
    }
  }

  return result;
}

bool checkAssertionSuperposition(
    DDSimulationState* ddsim,
    std::unique_ptr<SuperpositionAssertion>& assertion) {
  std::vector<size_t> qubits;
  for (auto variable : assertion->getTargetQubits()) {
    qubits.push_back(variableToQubit(ddsim, variable));
  }

  Statevector sv;
  sv.numQubits = qubits.size();
  sv.numStates = 1 << sv.numQubits;
  std::vector<Complex> amplitudes(sv.numStates);
  sv.amplitudes = amplitudes.data();

  ddsim->interface.getStateVectorSub(&ddsim->interface, sv.numQubits,
                                     qubits.data(), &sv);

  int found = 0;
  const double epsilon = 0.00000001;
  for (size_t i = 0; i < sv.numStates && found < 2; i++) {
    if (complexMagnitude(amplitudes[i]) > epsilon) {
      found++;
    }
  }

  return found > 1;
}

[[noreturn]] bool
checkAssertionSpan([[maybe_unused]] DDSimulationState* ddsim,
                   [[maybe_unused]] std::unique_ptr<SpanAssertion>& assertion) {
  throw std::runtime_error("Span assertions are not implemented");
}

bool checkAssertionEqualityStatevector(
    DDSimulationState* ddsim,
    std::unique_ptr<StatevectorEqualityAssertion>& assertion) {
  std::vector<size_t> qubits;
  for (auto variable : assertion->getTargetQubits()) {
    qubits.push_back(variableToQubit(ddsim, variable));
  }

  Statevector sv;
  sv.numQubits = qubits.size();
  sv.numStates = 1 << sv.numQubits;
  std::vector<Complex> amplitudes(sv.numStates);
  sv.amplitudes = amplitudes.data();

  ddsim->interface.getStateVectorSub(&ddsim->interface, sv.numQubits,
                                     qubits.data(), &sv);

  const double similarityThreshold = assertion->getSimilarityThreshold();

  const double similarity = dotProduct(sv, assertion->getTargetStatevector());

  return similarity >= similarityThreshold;
}

bool checkAssertionEqualityCircuit(
    [[maybe_unused]] DDSimulationState* ddsim,
    [[maybe_unused]] std::unique_ptr<CircuitEqualityAssertion>& assertion) {
  DDSimulationState secondSimulation;
  createDDSimulationState(&secondSimulation);
  secondSimulation.interface.loadCode(&secondSimulation.interface,
                                      assertion->getCircuitCode().c_str());
  if (!secondSimulation.assertionInstructions.empty()) {
    destroyDDSimulationState(&secondSimulation);
    throw std::runtime_error(
        "Circuit equality assertions cannot contain nested assertions");
  }
  secondSimulation.interface.runSimulation(&secondSimulation.interface);

  Statevector sv2;
  sv2.numQubits =
      secondSimulation.interface.getNumQubits(&secondSimulation.interface);
  sv2.numStates = 1 << sv2.numQubits;
  std::vector<Complex> amplitudes2(sv2.numStates);
  sv2.amplitudes = amplitudes2.data();
  secondSimulation.interface.getStateVectorFull(&secondSimulation.interface,
                                                &sv2);
  destroyDDSimulationState(&secondSimulation);

  std::vector<size_t> qubits;
  for (auto variable : assertion->getTargetQubits()) {
    qubits.push_back(variableToQubit(ddsim, variable));
  }
  Statevector sv;
  sv.numQubits = qubits.size();
  sv.numStates = 1 << sv.numQubits;
  std::vector<Complex> amplitudes(sv.numStates);
  sv.amplitudes = amplitudes.data();
  ddsim->interface.getStateVectorSub(&ddsim->interface, sv.numQubits,
                                     qubits.data(), &sv);

  const double similarityThreshold = assertion->getSimilarityThreshold();

  const double similarity = dotProduct(sv, sv2);

  return similarity >= similarityThreshold;
}

bool checkAssertion(DDSimulationState* ddsim,
                    std::unique_ptr<Assertion>& assertion) {
  if (assertion->getType() == AssertionType::Entanglement) {
    std::unique_ptr<EntanglementAssertion> entanglementAssertion(
        dynamic_cast<EntanglementAssertion*>(assertion.release()));
    auto result = checkAssertionEntangled(ddsim, entanglementAssertion);
    assertion = std::move(entanglementAssertion);
    return result;
  }
  if (assertion->getType() == AssertionType::Superposition) {
    std::unique_ptr<SuperpositionAssertion> superpositionAssertion(
        dynamic_cast<SuperpositionAssertion*>(assertion.release()));
    auto result = checkAssertionSuperposition(ddsim, superpositionAssertion);
    assertion = std::move(superpositionAssertion);
    return result;
  }
  if (assertion->getType() == AssertionType::Span) {
    std::unique_ptr<SpanAssertion> spanAssertion(
        dynamic_cast<SpanAssertion*>(assertion.release()));
    checkAssertionSpan(ddsim, spanAssertion);
  }
  if (assertion->getType() == AssertionType::StatevectorEquality) {
    std::unique_ptr<StatevectorEqualityAssertion> svEqualityAssertion(
        dynamic_cast<StatevectorEqualityAssertion*>(assertion.release()));
    auto result = checkAssertionEqualityStatevector(ddsim, svEqualityAssertion);
    assertion = std::move(svEqualityAssertion);
    return result;
  }
  if (assertion->getType() == AssertionType::CircuitEquality) {
    std::unique_ptr<CircuitEqualityAssertion> circuitEqualityAssertion(
        dynamic_cast<CircuitEqualityAssertion*>(assertion.release()));
    auto result =
        checkAssertionEqualityCircuit(ddsim, circuitEqualityAssertion);
    assertion = std::move(circuitEqualityAssertion);
    return result;
  }
  throw std::runtime_error("Unknown assertion type");
}

std::string validCodeFromChildren(const Instruction& parent,
                                  std::vector<Instruction>& allInstructions) {
  std::string code = parent.code;
  if (!parent.block.valid) {
    return code;
  }
  code += " { ";
  for (auto child : parent.childInstructions) {
    const auto& childInstruction = allInstructions[child];
    if (childInstruction.assertion != nullptr) {
      continue;
    }
    code += validCodeFromChildren(childInstruction, allInstructions);
  }
  code += " } ";
  return code;
}

std::string preprocessAssertionCode(const char* code,
                                    DDSimulationState* ddsim) {

  auto instructions = preprocessCode(code);
  std::vector<std::string> correctLines;
  ddsim->instructionTypes.clear();
  ddsim->instructionStarts.clear();
  ddsim->instructionEnds.clear();
  ddsim->callSubstitutions.clear();
  ddsim->classicalRegisters.clear();
  ddsim->qubitRegisters.clear();
  ddsim->successorInstructions.clear();
  ddsim->dataDependencies.clear();
  ddsim->breakpoints.clear();

  for (auto& instruction : instructions) {
    ddsim->successorInstructions.insert(
        {instruction.lineNumber, instruction.successorIndex});
    ddsim->instructionStarts.push_back(instruction.originalCodeStartPosition);
    ddsim->instructionEnds.push_back(instruction.originalCodeEndPosition);
    ddsim->dataDependencies.insert({instruction.lineNumber, {}});
    for (const auto& dependency : instruction.dataDependencies) {
      ddsim->dataDependencies[instruction.lineNumber].push_back(dependency);
    }
    if (instruction.code == "RETURN") {
      ddsim->instructionTypes.push_back(RETURN);
    } else if (instruction.assertion != nullptr) {
      ddsim->instructionTypes.push_back(ASSERTION);
      ddsim->assertionInstructions.insert(
          {instruction.lineNumber, std::move(instruction.assertion)});
    } else if (instruction.code.find("gate") != std::string::npos) {
      if (!instruction.inFunctionDefinition) {
        correctLines.push_back(
            validCodeFromChildren(instruction, instructions));
      }
      ddsim->instructionTypes.push_back(NOP);
    } else if (instruction.isFunctionCall) {
      if (!instruction.inFunctionDefinition) {
        correctLines.push_back(instruction.code);
      }
      ddsim->callSubstitutions.insert(
          {instruction.lineNumber, instruction.callSubstitution});
      ddsim->instructionTypes.push_back(CALL);
    } else if (instruction.code.find("qreg") != std::string::npos) {
      auto declaration = replaceString(instruction.code, "qreg", "");
      declaration = replaceString(declaration, " ", "");
      declaration = replaceString(declaration, "\t", "");
      declaration = replaceString(declaration, ";", "");
      auto parts = splitString(declaration, '[');
      auto name = parts[0];
      const size_t size = std::stoul(parts[1].substr(0, parts[1].size() - 1));

      const size_t index = ddsim->qubitRegisters.empty()
                               ? 0
                               : ddsim->qubitRegisters.back().index +
                                     ddsim->qubitRegisters.back().size;
      const QubitRegisterDefinition reg{name, index, size};
      ddsim->qubitRegisters.push_back(reg);

      if (!instruction.inFunctionDefinition) {
        correctLines.push_back(instruction.code);
      }
      ddsim->instructionTypes.push_back(NOP);
    } else if (instruction.code.find("creg") != std::string::npos) {
      auto declaration = replaceString(instruction.code, "creg", "");
      declaration = replaceString(declaration, " ", "");
      declaration = replaceString(declaration, "\t", "");
      declaration = replaceString(declaration, ";", "");
      auto parts = splitString(declaration, '[');
      auto name = parts[0];
      const size_t size = std::stoul(parts[1].substr(0, parts[1].size() - 1));

      const size_t index = ddsim->classicalRegisters.empty()
                               ? 0
                               : ddsim->classicalRegisters.back().index +
                                     ddsim->classicalRegisters.back().size;
      const ClassicalRegisterDefinition reg{removeWhitespace(name), index,
                                            size};
      ddsim->classicalRegisters.push_back(reg);
      for (auto i = 0ULL; i < size; i++) {
        const auto variableName =
            removeWhitespace(name) + "[" + std::to_string(i) + "]";
        ddsim->variableNames.push_back(
            std::make_unique<std::string>(variableName));
        const Variable newVariable{ddsim->variableNames.back()->data(),
                                   VariableType::VarBool,
                                   {false}};
        ddsim->variables.insert({newVariable.name, newVariable});
      }

      if (!instruction.inFunctionDefinition) {
        correctLines.push_back(instruction.code);
      }
      ddsim->instructionTypes.push_back(NOP);
    } else {
      if (!instruction.inFunctionDefinition) {
        correctLines.push_back(
            validCodeFromChildren(instruction, instructions));
      }
      ddsim->instructionTypes.push_back(SIMULATE);
    }
  }

  return std::accumulate(
      correctLines.begin(), correctLines.end(), std::string(),
      [](const std::string& a, const std::string& b) { return a + b; });
}

std::string getClassicalBitName(DDSimulationState* ddsim, size_t index) {
  for (auto& reg : ddsim->classicalRegisters) {
    if (index >= reg.index && index < reg.index + reg.size) {
      return reg.name + "[" + std::to_string(index - reg.index) + "]";
    }
  }
  return "UNKNOWN";
}
