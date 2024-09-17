#pragma once

#include "DDSimDiagnostics.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common/parsing/AssertionParsing.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Operation.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

enum InstructionType : uint8_t { NOP, SIMULATE, ASSERTION, CALL, RETURN };

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
  std::string code;
  std::string processedCode;
  bool ready;

  std::unique_ptr<qc::QuantumComputation> qc;
  std::unique_ptr<dd::Package<>> dd;
  std::vector<std::unique_ptr<qc::Operation>>::iterator iterator;
  qc::VectorDD simulationState;
  std::vector<InstructionType> instructionTypes;
  std::vector<size_t> instructionStarts;
  std::vector<size_t> instructionEnds;
  std::set<size_t> functionDefinitions;
  std::map<size_t, std::unique_ptr<Assertion>> assertionInstructions;
  std::map<size_t, size_t> successorInstructions;
  std::vector<QubitRegisterDefinition> qubitRegisters;
  std::vector<ClassicalRegisterDefinition> classicalRegisters;
  std::map<std::string, Variable> variables;
  std::vector<std::unique_ptr<std::string>> variableNames;
  std::vector<size_t> previousInstructionStack;
  std::vector<size_t> callReturnStack;
  std::map<size_t, std::map<std::string, std::string>> callSubstitutions;
  std::vector<std::pair<size_t, size_t>> restoreCallReturnStack;
  std::map<size_t, std::vector<std::pair<size_t, size_t>>> dataDependencies;
  std::set<size_t> breakpoints;
  std::vector<std::vector<std::string>> targetQubits;

  bool paused;

  size_t lastFailedAssertion;
  size_t lastMetBreakpoint;

  DDDiagnostics diagnostics;
};

Result ddsimInit(SimulationState* self);

Result ddsimLoadCode(SimulationState* self, const char* code);
Result ddsimStepForward(SimulationState* self);
Result ddsimStepBackward(SimulationState* self);
Result ddsimStepOverForward(SimulationState* self);
Result ddsimStepOverBackward(SimulationState* self);
Result ddsimStepOutForward(SimulationState* self);
Result ddsimStepOutBackward(SimulationState* self);
Result ddsimRunAll(SimulationState* self, size_t* failedAssertions);
Result ddsimRunSimulation(SimulationState* self);
Result ddsimRunSimulationBackward(SimulationState* self);
Result ddsimResetSimulation(SimulationState* self);
Result ddsimPauseSimulation(SimulationState* self);
bool ddsimCanStepForward(SimulationState* self);
bool ddsimCanStepBackward(SimulationState* self);
bool ddsimIsFinished(SimulationState* self);
bool ddsimDidAssertionFail(SimulationState* self);
bool ddsimWasBreakpointHit(SimulationState* self);

size_t ddsimGetCurrentInstruction(SimulationState* self);
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
size_t ddsimGetNumClassicalVariables(SimulationState* self);
Result ddsimGetClassicalVariableName(SimulationState* self,
                                     size_t variableIndex, char* output);

Result ddsimGetStateVectorFull(SimulationState* self, Statevector* output);
Result ddsimGetStateVectorSub(SimulationState* self, size_t subStateSize,
                              const size_t* qubits, Statevector* output);

Result ddsimSetBreakpoint(SimulationState* self, size_t desiredPosition,
                          size_t* targetInstruction);
Result ddsimClearBreakpoints(SimulationState* self);
Result ddsimGetStackDepth(SimulationState* self, size_t* depth);
Result ddsimGetStackTrace(SimulationState* self, size_t maxDepth,
                          size_t* output);

Diagnostics* ddsimGetDiagnostics(SimulationState* self);

Result createDDSimulationState(DDSimulationState* self);
Result destroyDDSimulationState(DDSimulationState* self);

std::string preprocessAssertionCode(const char* code, DDSimulationState* ddsim);
bool checkAssertion(DDSimulationState* ddsim,
                    std::unique_ptr<Assertion>& assertion);
std::string getClassicalBitName(DDSimulationState* ddsim, size_t index);
size_t variableToQubit(DDSimulationState* ddsim, const std::string& variable);
bool isSubStateVectorLegal(const Statevector& full,
                           std::vector<size_t>& targetQubits);
std::vector<std::vector<Complex>>
getPartialTraceFromStateVector(const Statevector& sv,
                               const std::vector<size_t>& traceOut);
