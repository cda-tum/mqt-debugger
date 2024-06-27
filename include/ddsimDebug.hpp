#ifndef DDSIM_DEBUG_H
#define DDSIM_DEBUG_H

#include "common.h"
#include "debug.h"
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
  SimulationState interface;
  size_t currentLine;

  std::unique_ptr<qc::QuantumComputation> qc;
  std::unique_ptr<dd::Package<>> dd;
  std::vector<std::unique_ptr<qc::Operation>>::iterator iterator;
  qc::VectorDD simulationState;
  std::vector<InstructionType> instructionTypes;
  std::map<size_t, std::string> assertionInstructions;
  std::vector<QubitRegisterDefinition> qubitRegisters;

  bool assertionFailed;
} DDSimSimulationState;

Result ddsimInit(SimulationState* self);

Result ddsimLoadCode(SimulationState* self, const char* code);
Result ddsimStepForward(SimulationState* self);
Result ddsimStepBackward(SimulationState* self);
Result ddsimRunSimulation(SimulationState* self);
Result ddsimResetSimulation(SimulationState* self);
bool ddsimCanStepForward(SimulationState* self);
bool ddsimCanStepBackward(SimulationState* self);
bool ddsimIsFinished(SimulationState* self);

size_t ddsimGetCurrentLine(SimulationState* self);
Result ddsimGetAmplitudeIndex(SimulationState* self, size_t qubit, Complex* output);
Result ddsimGetAmplitudeBitstring(SimulationState* self, const char* bitstring, Complex* output);
Result ddsimGetClassicalVariable(SimulationState* self, const char* name, Variable* output);
Result ddsimGetStateVectorFull(SimulationState* self, Statevector* output);
Result ddsimGetStateVectorSub(SimulationState* self, size_t subStateSize, const size_t* qubits, Statevector* output);

Result createDDSimSimulationState(DDSimSimulationState* self);
Result destroyDDSimSimulationState([[maybe_unused]] DDSimSimulationState* self);

std::string preprocessAssertionCode(const char* code, DDSimSimulationState* ddsim);
bool checkAssertion(DDSimSimulationState* ddsim, std::string& assertion);

#endif // DDSIM_DEBUG_H