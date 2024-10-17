/**
 * @file common_fixtures.cpp
 * @brief Provides common fixture base classes for other tests.
 */

#include "backend/dd/DDSimDebug.hpp"
#include "backend/dd/DDSimDiagnostics.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "utils_test.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

/**
 * @brief Fixture that loads custom code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class CustomCodeFixture : public testing::Test {
protected:
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    diagnostics = state->getDiagnostics(state);
  }

  /**
   * @brief The DDSimulationState to use for testing.
   */
  DDSimulationState ddState;
  /**
   * @brief The DDSimulationState to use for testing.
   */
  Diagnostics* diagnostics;
  /**
   * @brief A reference to the SimulationState interface for easier access.
   */
  SimulationState* state = nullptr;

  /**
   * @brief The full code to load into the state.
   */
  std::string fullCode;

  /**
   * @brief The code provided by the user explicitly.
   */
  std::string userCode;

  /*
   * @brief Load custom code into the state.
   *
   * Classical and Quantum registers of the given size are created automatically
   * with the names `c` and `q`. Therefore, the first instruction in the
   * provided code will have instruction index 2, as 0 and 1 are reserved for
   * the registers.\n\n
   *
   * At least one classical and quantum bit will always be created, even if the
   * given number is less than 1.
   *
   * @param numQubits The number of qubits to create.
   * @param numClassics The number of classical bits to create.
   * @param code The code to load into the state.
   * @param shouldFail Asserts whether the code should fail to load (i.e., due
   * to an expected error).
   * @param preamble The preamble to add to the code before loading declaring
   * the registers (e.g., library imports).
   */
  void loadCode(size_t numQubits, size_t numClassics, const char* code,
                bool shouldFail = false, const char* preamble = "") {
    if (numQubits < 1) {
      numQubits = 1;
    }
    if (numClassics < 1) {
      numClassics = 1;
    }
    std::ostringstream ss;

    ss << preamble;

    ss << "qreg q[" << numQubits << "];\n";
    ss << "creg c[" << numClassics << "];\n";

    ss << code;

    userCode = code;
    fullCode = ss.str();
    ASSERT_EQ(state->loadCode(state, fullCode.c_str()),
              shouldFail ? ERROR : OK);
  }

  /**
   * @brief Continue execution until the given instruction is reached.
   *
   * Instruction numbers start with 0 for the first instruction defined in the
   * user code.
   *
   * @param instruction The instruction to forward to.
   */
  void forwardTo(size_t instruction) {
    instruction += 2; // Skip the qreg and creg declarations
    size_t currentInstruction = state->getCurrentInstruction(state);
    while (currentInstruction < instruction) {
      state->stepForward(state);
      currentInstruction = state->getCurrentInstruction(state);
    }
  }
};

/**
 * @brief Fixture that allows code to be loaded from a specific file.
 *
 * The file must be located in the `circuits` directory.
 */
class LoadFromFileFixture : public virtual testing::Test {
protected:
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    diagnostics = state->getDiagnostics(state);
  }

  /**
   * @brief The DDSimulationState to use for testing.
   */
  DDSimulationState ddState;
  /**
   * @brief A reference to the SimulationState interface for easier access.
   */
  SimulationState* state = nullptr;
  /**
   * @brief A reference to the Diagnostics interface for easier access.
   */
  Diagnostics* diagnostics = nullptr;

  /**
   * @brief Load the code from the file with the given name.
   *
   * The given file should be located in the `circuits` directory and use the
   * `.qasm` extension.
   * @param testName The name of the file to load (not including the `circuits`
   * directory path and the extension).
   */
  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }

  /**
   * @brief Continue execution until the given instruction is reached.
   *
   * @param instruction The instruction to forward to.
   */
  void forwardTo(size_t instruction) {
    size_t currentInstruction = state->getCurrentInstruction(state);
    while (currentInstruction < instruction) {
      state->stepForward(state);
      currentInstruction = state->getCurrentInstruction(state);
    }
  }
};
