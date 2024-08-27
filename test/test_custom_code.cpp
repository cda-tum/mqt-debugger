#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "common.h"
#include "utils_test.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

class CustomCodeTest : public testing::Test {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
  }

protected:
  DDSimulationState ddState;
  SimulationState* state = nullptr;

  std::string fullCode;
  std::string userCode;
  size_t offset = 0;

  void loadCode(size_t numQubits, size_t numClassics, const char* code) {
    if (numQubits < 1) {
      numQubits = 1;
    }
    if (numClassics < 1) {
      numClassics = 1;
    }
    std::ostringstream ss;

    ss << "qreg q[" << numQubits << "];\n";
    ss << "creg c[" << numClassics << "];\n";

    offset = ss.str().size();

    ss << code;

    userCode = code;
    fullCode = ss.str();
    state->loadCode(state, fullCode.c_str());
  }

  void forwardTo(size_t instruction) {
    instruction += 2; // Skip the qreg and creg declarations
    size_t currentInstruction = state->getCurrentInstruction(state);
    while (currentInstruction < instruction) {
      state->stepForward(state);
      currentInstruction = state->getCurrentInstruction(state);
    }
  }
};

TEST_F(CustomCodeTest, ClassicControlledOperation) {
  loadCode(2, 1,
           "h q[0];"
           "cx q[0], q[1];"
           "measure q[0] -> c[0];"
           "if(c==1) x q[1];"
           "if(c==0) z q[1];");
  ASSERT_EQ(state->runSimulation(state), OK);

  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};
  state->getStateVectorFull(state, &sv);
  ASSERT_TRUE(complexEquality(amplitudes[0], 1, 0.0) ||
              complexEquality(amplitudes[1], 1, 0.0));
}

TEST_F(CustomCodeTest, ResetGate) {
  loadCode(1, 1,
           "x q[0];"
           "z q[0];"
           "reset q[0];"
           "barrier;");
  Complex result;

  forwardTo(2);
  ASSERT_EQ(state->getAmplitudeIndex(state, 1, &result), OK);
  ASSERT_TRUE(complexEquality(result, -1.0, 0.0));

  forwardTo(3);
  ASSERT_EQ(state->getAmplitudeIndex(state, 0, &result), OK);
  ASSERT_TRUE(complexEquality(result, -1.0, 0.0));
}

TEST_F(CustomCodeTest, GateInGateName) {
  loadCode(1, 1,
           "gate my_gate q0 {"
           "  x q0;"
           "}"
           "my_gate q[0];"
           "measure q[0] -> c[0];"
           "assert-eq q[0] { 0, 1 }");
  state->runSimulation(state);
}

TEST_F(CustomCodeTest, EqualityAssertion) {
  loadCode(2, 0,
           "h q[0];"
           "cx q[0], q[1];"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }"
           "assert-eq q[0], q[1] { qreg q[2]; h q[1]; cx q[1], q[0]; }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

TEST_F(CustomCodeTest, DestructiveInterference) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-sup q[1], q[2];"
           "assert-ent q[1], q[2];");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}
