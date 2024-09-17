#pragma once

// NOLINTBEGIN(modernize-use-using, performance-enum-size)

#include "common.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#endif

typedef enum { Unknown, MissingInteraction, ControlAlwaysZero } ErrorCauseType;

typedef struct ErrorCause ErrorCause;
struct ErrorCause {
  ErrorCauseType type;
  size_t instruction;
};

typedef struct Diagnostics Diagnostics;
struct Diagnostics {
  Result (*init)(Diagnostics* self);

  size_t (*getNumQubits)(Diagnostics* self);
  size_t (*getInstructionCount)(Diagnostics* self);
  Result (*getDataDependencies)(Diagnostics* self, size_t instruction,
                                bool* instructions);
  Result (*getInteractions)(Diagnostics* self, size_t beforeInstruction,
                            size_t qubit, bool* qubitsAreInteracting);
  Result (*getZeroControlInstructions)(Diagnostics* self, bool* instructions);
  size_t (*potentialErrorCauses)(Diagnostics* self, ErrorCause* output,
                                 size_t count);
};

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-use-using, performance-enum-size)
