#pragma once

#include "QuantumComputation.hpp"
#include "backend/debug.h"
#include "common.h"
#include "common/parsing/AssertionParsing.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"

#include <string>

enum InstructionType { NOP, SIMULATE, ASSERTION, CALL, RETURN };

struct QubitRegisterDefinition {
  std::string name;
  size_t index;
  size_t size;
};

struct ClassicalRegisterDefinition {
  std::string name;
  size_t index;
  size_t size;
};

struct DDSimulationState {
  SimulationState interface;
  size_t currentInstruction;

  std::unique_ptr<qc::QuantumComputation> qc;
  std::unique_ptr<dd::Package<>> dd;
  std::vector<std::unique_ptr<qc::Operation>>::iterator iterator;
  qc::VectorDD simulationState;
  std::vector<InstructionType> instructionTypes;
  std::vector<size_t> instructionStarts;
  std::vector<size_t> instructionEnds;
  std::map<size_t, std::unique_ptr<Assertion>> assertionInstructions;
  std::map<size_t, size_t> successorInstructions;
  std::vector<QubitRegisterDefinition> qubitRegisters;
  std::vector<ClassicalRegisterDefinition> classicalRegisters;
  std::map<std::string, Variable> variables;
  std::vector<size_t> previousInstructionStack;
  std::vector<size_t> callReturnStack;
  std::map<size_t, std::map<std::string, std::string>> callSubstitutions;
  std::vector<std::pair<size_t, size_t>> restoreCallReturnStack;
  std::map<size_t, std::vector<size_t>> dataDependencies;

  bool assertionFailed;
};

Result ddsimInit(SimulationState* self);

Result ddsimLoadCode(SimulationState* self, const char* code);
Result ddsimStepForward(SimulationState* self);
Result ddsimStepBackward(SimulationState* self);
Result ddsimRunSimulation(SimulationState* self);
Result ddsimResetSimulation(SimulationState* self);
bool ddsimCanStepForward(SimulationState* self);
bool ddsimCanStepBackward(SimulationState* self);
bool ddsimIsFinished(SimulationState* self);
bool ddsimDidAssertionFail(SimulationState* self);

size_t ddsimGetCurrentInstruction(SimulationState* self);
size_t ddsimGetPreviousInstruction(SimulationState* self);
size_t ddsimGetInstructionCount(SimulationState* self);
Result ddsimGetInstructionPosition(SimulationState* self, size_t instruction,
                                   size_t* start, size_t* end);

size_t ddsimGetNumQubits(SimulationState* self);
Result ddsimGetAmplitudeIndex(SimulationState* self, size_t qubit,
                              Complex* output);
Result ddsimGetAmplitudeBitstring(SimulationState* self, const char* bitstring,
                                  Complex* output);
Result ddsimGetClassicalVariable(SimulationState* self, const char* name,
                                 Variable* output);
Result ddsimGetStateVectorFull(SimulationState* self, Statevector* output);
Result ddsimGetStateVectorSub(SimulationState* self, size_t subStateSize,
                              const size_t* qubits, Statevector* output);

Result ddsimGetDataDependencies(SimulationState* self, size_t instruction,
                                bool* instructions);

Result createDDSimulationState(DDSimulationState* self);
Result destroyDDSimulationState([[maybe_unused]] DDSimulationState* self);

std::string preprocessAssertionCode(const char* code, DDSimulationState* ddsim);
bool checkAssertion(DDSimulationState* ddsim,
                    std::unique_ptr<Assertion>& assertion);
std::string getClassicalBitName(DDSimulationState* ddsim, size_t index);
