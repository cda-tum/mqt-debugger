#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-reinterpret-cast"

#include "backend/dd/DDSimDebug.hpp"

#include "backend/debug.h"
#include "backend/parsing/AssertionParsing.hpp"
#include "common.h"

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
  self->interface.runSimulation = ddsimRunSimulation;
  self->interface.resetSimulation = ddsimResetSimulation;
  self->interface.canStepForward = ddsimCanStepForward;
  self->interface.canStepBackward = ddsimCanStepBackward;
  self->interface.isFinished = ddsimIsFinished;
  self->interface.didAssertionFail = ddsimDidAssertionFail;

  self->interface.getCurrentLine = ddsimGetCurrentLine;
  self->interface.getAmplitudeIndex = ddsimGetAmplitudeIndex;
  self->interface.getAmplitudeBitstring = ddsimGetAmplitudeBitstring;
  self->interface.getClassicalVariable = ddsimGetClassicalVariable;
  self->interface.getStateVectorFull = ddsimGetStateVectorFull;
  self->interface.getStateVectorSub = ddsimGetStateVectorSub;

  return self->interface.init(reinterpret_cast<SimulationState*>(self));
}
#pragma clang diagnostic pop

void resetSimulationState(DDSimulationState* ddsim) {
  if (ddsim->simulationState.p != nullptr) {
    ddsim->dd->decRef(ddsim->simulationState);
  }
  ddsim->simulationState = ddsim->dd->makeZeroState(ddsim->qc->getNqubits());
  ddsim->dd->incRef(ddsim->simulationState);
}

Result ddsimInit(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);

  ddsim->simulationState.p = nullptr;
  ddsim->qc = std::make_unique<qc::QuantumComputation>();
  ddsim->dd = std::make_unique<dd::Package<>>(1);
  ddsim->iterator = ddsim->qc->begin();
  ddsim->currentLine = 0;
  ddsim->assertionFailed = false;
  ddsim->lastIrreversibleStep = 0;

  resetSimulationState(ddsim);

  return OK;
}

Result ddsimLoadCode(SimulationState* self, const char* code) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  ddsim->currentLine = 0;
  std::stringstream ss{preprocessAssertionCode(code, ddsim)};
  ddsim->qc->import(ss, qc::Format::OpenQASM3);

  ddsim->iterator = ddsim->qc->begin();
  ddsim->dd->resize(ddsim->qc->getNqubits());
  ddsim->assertionFailed = false;

  resetSimulationState(ddsim);

  return OK;
}

Result ddsimStepForward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  if (!self->canStepForward(self)) {
    return ERROR;
  }
  ddsim->currentLine++;

  if (ddsim->instructionTypes[ddsim->currentLine - 1] == ASSERTION) {
    auto& assertion = ddsim->assertionInstructions[ddsim->currentLine - 1];
    ddsim->assertionFailed = !checkAssertion(ddsim, assertion);
    return OK;
  }

  ddsim->assertionFailed = false;
  if (ddsim->instructionTypes[ddsim->currentLine - 1] != SIMULATE) {
    return OK;
  }

  qc::MatrixDD currDD{};
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
        const Variable newVariable{
            name.c_str(), VariableType::VarBool, {!result}};
        ddsim->variables.insert({name, newVariable});
      }
    }

    ddsim->iterator++;
    ddsim->lastIrreversibleStep = ddsim->currentLine;
    return OK;
  }
  if ((*ddsim->iterator)->getType() == qc::Reset) {
    ddsim->iterator++;
    return ERROR; // TODO
  }
  if ((*ddsim->iterator)->getType() == qc::Barrier) {
    ddsim->iterator++;
    return OK;
  }
  if ((*ddsim->iterator)->isClassicControlledOperation()) {
    // TODO this is for later
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
  ddsim->currentLine--;

  if (ddsim->instructionTypes[ddsim->currentLine] == SIMULATE) {

    ddsim->iterator--;
    qc::MatrixDD currDD{};
    if ((*ddsim->iterator)->isClassicControlledOperation()) {
      // TODO this is for later
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
  }

  if (ddsim->currentLine > 0 &&
      ddsim->instructionTypes[ddsim->currentLine - 1] == ASSERTION) {
    auto& assertion = ddsim->assertionInstructions[ddsim->currentLine - 1];
    ddsim->assertionFailed = !checkAssertion(ddsim, assertion);
  } else {
    ddsim->assertionFailed = false;
  }

  return OK;
}

Result ddsimRunSimulation(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  while (!self->isFinished(self) && !ddsim->assertionFailed) {
    self->stepForward(self);
  }
  return OK;
}

Result ddsimResetSimulation(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  ddsim->currentLine = 0;

  ddsim->iterator = ddsim->qc->begin();
  ddsim->assertionFailed = false;
  ddsim->lastIrreversibleStep = 0;
  ddsim->variables.clear();

  resetSimulationState(ddsim);
  return OK;
}

bool ddsimCanStepForward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentLine < ddsim->instructionTypes.size();
}

bool ddsimCanStepBackward(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentLine > ddsim->lastIrreversibleStep;
}

bool ddsimIsFinished(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentLine == ddsim->instructionTypes.size();
}

bool ddsimDidAssertionFail(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->assertionFailed;
}

size_t ddsimGetCurrentLine(SimulationState* self) {
  auto* ddsim = reinterpret_cast<DDSimulationState*>(self);
  return ddsim->currentLine;
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

Result destroyDDSimulationState([[maybe_unused]] DDSimulationState* self) {
  return OK;
}

//-----------------------------------------------------------------------------------------

std::string replaceAll(std::string str, const std::string& from,
                       const std::string& to) {
  size_t startPos = 0;
  while ((startPos = str.find(from, startPos)) != std::string::npos) {
    str.replace(startPos, from.length(), to);
    startPos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}

std::vector<std::string> split(std::string& text, char delimiter) {
  std::vector<std::string> result;
  std::istringstream iss(text);
  for (std::string s; std::getline(iss, s, delimiter);) {
    result.push_back(s);
  }
  return result;
}

size_t variableToQubit(DDSimulationState* ddsim, std::string& variable) {
  auto declaration = replaceAll(variable, " ", "");
  declaration = replaceAll(declaration, "\t", "");
  auto parts = split(declaration, '[');
  auto var = parts[0];
  const size_t idx = std::stoul(parts[1].substr(0, parts[1].size() - 1));

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

[[noreturn]] bool checkAssertionEqualityCircuit(
    [[maybe_unused]] DDSimulationState* ddsim,
    [[maybe_unused]] std::unique_ptr<CircuitEqualityAssertion>& assertion) {
  throw std::runtime_error("Span assertions are not implemented");
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
    auto result = checkAssertionSpan(ddsim, spanAssertion);
    assertion = std::move(spanAssertion);
    return result;
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

std::string preprocessAssertionCode(const char* code,
                                    DDSimulationState* ddsim) {
  std::vector<std::string> lines;
  std::string token;
  std::istringstream tokenStream(code);
  while (std::getline(tokenStream, token, ';')) {
    lines.push_back(token);
  }

  std::vector<std::string> correctLines;
  size_t i = 0;
  for (const auto& line : lines) {
    if (line.find("qreg") != std::string::npos) {
      auto declaration = replaceAll(line, "qreg", "");
      declaration = replaceAll(declaration, " ", "");
      declaration = replaceAll(declaration, "\t", "");
      declaration = replaceAll(declaration, "\n", "");
      declaration = replaceAll(declaration, ";", "");
      auto parts = split(declaration, '[');
      auto name = parts[0];
      const size_t size = std::stoul(parts[1].substr(0, parts[1].size() - 1));

      const size_t index = ddsim->qubitRegisters.empty()
                               ? 0
                               : ddsim->qubitRegisters.back().index +
                                     ddsim->qubitRegisters.back().size;
      const QubitRegisterDefinition reg{name, index, size};
      ddsim->qubitRegisters.push_back(reg);

      correctLines.push_back(replaceAll(line, "\n", ""));
      ddsim->instructionTypes.push_back(NOP);
    } else if (line.find("creg") != std::string::npos) {
      auto declaration = replaceAll(line, "creg", "");
      declaration = replaceAll(declaration, " ", "");
      declaration = replaceAll(declaration, "\t", "");
      declaration = replaceAll(declaration, ";", "");
      auto parts = split(declaration, '[');
      auto name = parts[0];
      const size_t size = std::stoul(parts[1].substr(0, parts[1].size() - 1));

      const size_t index = ddsim->classicalRegisters.empty()
                               ? 0
                               : ddsim->classicalRegisters.back().index +
                                     ddsim->classicalRegisters.back().size;
      const ClassicalRegisterDefinition reg{name, index, size};
      ddsim->classicalRegisters.push_back(reg);

      correctLines.push_back(replaceAll(line, "\n", ""));
      ddsim->instructionTypes.push_back(NOP);
    } else if (isAssertion(line)) {
      ddsim->instructionTypes.push_back(ASSERTION);
      ddsim->assertionInstructions.insert({i, parseAssertion(line)});
    } else {
      correctLines.push_back(replaceAll(line, "\n", ""));
      ddsim->instructionTypes.push_back(SIMULATE);
    }
    i++;
  }

  return std::accumulate(
      correctLines.begin(), correctLines.end(), std::string(),
      [](const std::string& a, const std::string& b) { return a + b + ";"; });
}

std::string getClassicalBitName(DDSimulationState* ddsim, size_t index) {
  for (auto& reg : ddsim->classicalRegisters) {
    if (index >= reg.index && index < reg.index + reg.size) {
      return reg.name + "[" + std::to_string(index - reg.index) + "]";
    }
  }
  return "UNKNOWN";
}
