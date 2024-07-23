#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Diagnostics Diagnostics;

struct Diagnostics {
  Result (*init)(Diagnostics* self);

  size_t (*getNumQubits)(Diagnostics* self);
  size_t (*getInstructionCount)(Diagnostics* self);
  Result (*getDataDependencies)(Diagnostics* self, size_t instruction,
                                bool* instructions);
  Result (*getInteractions)(Diagnostics* self, size_t beforeInstruction,
                            size_t qubit, bool* qubitsAreInteracting);
};

#ifdef __cplusplus
}
#endif
