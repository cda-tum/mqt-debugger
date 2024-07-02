#pragma once

#include "common.h"
#include "backend/debug.h"
#include "dd/Operations.hpp"
#include "QuantumComputation.hpp"
#include "dd/Package.hpp"

#include <string>

enum InstructionType
{
    NOP,
    SIMULATE,
    ASSERTION
};

typedef struct
{
    std::string name;
    size_t index;
    size_t size;
} QubitRegisterDefinition;

typedef struct
{
    std::string name;
    size_t index;
    size_t size;
} ClassicalRegisterDefinition;

typedef struct
{
  SimulationState interface;
  size_t currentLine;

  std::unique_ptr<qc::QuantumComputation> qc;
  std::unique_ptr<dd::Package<>> dd;
  std::vector<std::unique_ptr<qc::Operation>>::iterator iterator;
  qc::VectorDD simulationState;
  std::vector<InstructionType> instructionTypes;
  std::map<size_t, std::string> assertionInstructions;
  std::vector<QubitRegisterDefinition> qubitRegisters;
  std::vector<ClassicalRegisterDefinition > classicalRegisters;
  std::map<std::string, Variable> variables;
  size_t lastIrreversibleStep;

  bool assertionFailed;
} DDSimulationState;

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

size_t ddsimGetCurrentLine(SimulationState* self);
Result ddsimGetAmplitudeIndex(SimulationState* self, size_t qubit, Complex* output);
Result ddsimGetAmplitudeBitstring(SimulationState* self, const char* bitstring, Complex* output);
Result ddsimGetClassicalVariable(SimulationState* self, const char* name, Variable* output);
Result ddsimGetStateVectorFull(SimulationState* self, Statevector* output);
Result ddsimGetStateVectorSub(SimulationState* self, size_t subStateSize, const size_t* qubits, Statevector* output);

Result createDDSimulationState(DDSimulationState* self);
Result destroyDDSimulationState([[maybe_unused]] DDSimulationState* self);

std::string preprocessAssertionCode(const char* code, DDSimulationState* ddsim);
bool checkAssertion(DDSimulationState* ddsim, std::string& assertion);
std::string getClassicalBitName(DDSimulationState* ddsim, size_t index);

