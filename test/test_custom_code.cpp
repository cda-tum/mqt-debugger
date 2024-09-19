#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
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

    offset = ss.str().size();

    ss << code;

    userCode = code;
    fullCode = ss.str();
    ASSERT_EQ(state->loadCode(state, fullCode.c_str()),
              shouldFail ? ERROR : OK);
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

TEST_F(CustomCodeTest, ClassicControlledOperationFalse) {
  loadCode(2, 1,
           "z q[0];"
           "cx q[0], q[1];"
           "measure q[0] -> c[0];"
           "if(c==1) x q[1];"
           "if(c==0) z q[1];");
  ASSERT_EQ(state->runSimulation(state), OK);

  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};
  state->getStateVectorFull(state, &sv);
  ASSERT_TRUE(complexEquality(amplitudes[0], 1, 0.0));

  ASSERT_EQ(state->stepBackward(state), OK);
}

TEST_F(CustomCodeTest, ClassicControlledOperationTrue) {
  loadCode(2, 1,
           "x q[0];"
           "cx q[0], q[1];"
           "measure q[0] -> c[0];"
           "if(c==1) x q[1];"
           "if(c==0) z q[1];");
  ASSERT_EQ(state->runSimulation(state), OK);

  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};
  state->getStateVectorFull(state, &sv);
  ASSERT_TRUE(complexEquality(amplitudes[1], 1, 0.0));

  ASSERT_EQ(state->stepBackward(state), OK);
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

TEST_F(CustomCodeTest, IllegalSubstateSVEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[0], q[1] { 0.5, 0.5, 0.5, 0.5 }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), ERROR);
}

TEST_F(CustomCodeTest, LegalSubstateSVEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[1], q[2] { 0.707, 0, 0, 0.707 }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

TEST_F(CustomCodeTest, IllegalSubstateCircuitEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[0], q[1] { qreg q[2]; h q[0]; h q[1]; }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), ERROR);
}

TEST_F(CustomCodeTest, LegalSubstateCircuitEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[2], q[1] { qreg q[2]; h q[0]; cx q[0], q[1]; }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

TEST_F(CustomCodeTest, ErrorInCode) {
  loadCode(3, 0, "x f[0];", true);
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), ERROR);
  ASSERT_EQ(numErrors, 0);
  ASSERT_EQ(state->runSimulation(state), ERROR);
  ASSERT_EQ(state->runSimulationBackward(state), ERROR);
}

TEST_F(CustomCodeTest, StackTraceErrorInCode) {
  loadCode(3, 0, "x f[0];", true);
  size_t depth = 0;
  ASSERT_EQ(state->getStackDepth(state, &depth), ERROR);
  ASSERT_EQ(state->getStackTrace(state, 10, nullptr), ERROR);
}

TEST_F(CustomCodeTest, ErrorAssertionInCircuitEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "assert-eq q[0], q[1], q[2] { qreg q[3]; assert-sup q[0]; }");
  ASSERT_EQ(state->runAll(state, nullptr), ERROR);
}

TEST_F(CustomCodeTest, BarrierInstruction) {
  loadCode(1, 0,
           "barrier;"
           "x q[0];");
  ASSERT_EQ(state->stepForward(state), OK);
  ASSERT_EQ(state->stepForward(state), OK);

  ASSERT_EQ(state->stepForward(state), OK);
  ASSERT_EQ(state->stepBackward(state), OK);
  ASSERT_EQ(state->stepOverForward(state), OK);
  ASSERT_EQ(state->stepOverBackward(state), OK);
}

TEST_F(CustomCodeTest, ErrorAssertionInvalidIndex) {
  loadCode(3, 0,
           "x q[0];"
           "assert-sup q[3];");
  ASSERT_EQ(state->runAll(state, nullptr), ERROR);
}

TEST_F(CustomCodeTest, ErrorAssertionInvalidQubit) {
  loadCode(3, 0,
           "x q[0];"
           "assert-sup f[3];");
  ASSERT_EQ(state->runAll(state, nullptr), ERROR);
}

TEST_F(CustomCodeTest, AssertionInCustomGate) {
  loadCode(3, 0,
           "gate test q0 {"
           "h q0;"
           "assert-sup q0;"
           "}"
           "test q[0];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

TEST_F(CustomCodeTest, AssertionInCustomGateShadowing) {
  loadCode(3, 0,
           "gate test q {"
           "h q;"
           "assert-sup q;"
           "}"
           "test q[0];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

TEST_F(CustomCodeTest, CommentAtEnd) {
  loadCode(3, 0, "x q[0]; // Comment");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
  ASSERT_EQ(state->getCurrentInstruction(state), 3);
}

TEST_F(CustomCodeTest, QASMPreamble) {
  loadCode(3, 0, "x q[0]; // Comment", false,
           "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
  ASSERT_EQ(state->getCurrentInstruction(state), 5);
}

TEST_F(CustomCodeTest, LargeProgram) {
  loadCode(20, 0, "x q[0]; cx q[0], q[1];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
  ASSERT_EQ(state->getCurrentInstruction(state), 4);
}

TEST_F(CustomCodeTest, CollectiveGateAsDependency) {
  loadCode(2, 0, "x q; barrier q[0];");
  auto* diagnosis = state->getDiagnostics(state);
  std::array<bool, 4> dependencies{};
  ASSERT_EQ(
      diagnosis->getDataDependencies(diagnosis, 3, false, dependencies.data()),
      OK);
  ASSERT_EQ(dependencies[0], false);
  ASSERT_EQ(dependencies[1], false);
  ASSERT_EQ(dependencies[2], true);
  ASSERT_EQ(dependencies[3], true);
}

TEST_F(CustomCodeTest, CollectiveGateAsInteraction) {
  loadCode(1, 0, "qreg p[1]; cx q, p; assert-ent q[0], p[0];");
  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_TRUE(state->didAssertionFail(state));

  auto* diagnosis = state->getDiagnostics(state);

  std::array<bool, 2> interactions{};
  ASSERT_EQ(diagnosis->getInteractions(diagnosis, 4, 0, interactions.data()),
            OK);

  ASSERT_TRUE(interactions[0]);
  ASSERT_TRUE(interactions[1]);

  std::array<ErrorCause, 1> causes{};
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      1);
  ASSERT_EQ(causes[0].type, ControlAlwaysZero);
  ASSERT_EQ(causes[0].instruction, 3);
}

TEST_F(CustomCodeTest, NonZeroControlsInErrorSearch) {
  loadCode(2, 0,
           "gate test q1, q2 { cx q1, q2; } x q[0]; test q[1], q[0]; test "
           "q[0], q[1]; assert-sup q[0];");
  auto* diagnosis = state->getDiagnostics(state);
  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_TRUE(state->didAssertionFail(state));
  std::array<ErrorCause, 5> errors{};
  ASSERT_TRUE(diagnosis->potentialErrorCauses(diagnosis, errors.data(),
                                              errors.size()) == 0);
}

TEST_F(CustomCodeTest, PaperExampleGrover) {
  loadCode(3, 3,
           "gate oracle q0, q1, q2, flag {"
           "assert-sup q0, q1, q2;"
           "ccz q1, q2, flag;"
           "assert-ent q0, q1, q2;"
           "}"
           "gate diffusion q0, q1, q2 {"
           "h q0; h q1; h q2;"
           "x q0; x q1; x q2;"
           "ccz q0, q1, q2;"
           "x q2; x q1; x q0;"
           "h q2; h q1; h q0;"
           "}"
           "qreg flag[1];"
           "x flag;"
           "oracle q[0], q[1], q[2], flag;"
           "diffusion q[0], q[1], q[2];"
           "assert-eq 0.8, q[0], q[1], q[2] { 0, 0, 0, 0, 0, 0, 0, 1 }"
           "oracle q[0], q[1], q[2], flag;"
           "diffusion q[0], q[1], q[2];"
           "assert-eq 0.9, q[0], q[1], q[2] { 0, 0, 0, 0, 0, 0, 0, 1 }",
           false, "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n");

  auto* diagnosis = state->getDiagnostics(state);
  std::array<ErrorCause, 10> causes{};

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 5);
  // We expect no potential errors yet:
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      0);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);
  // We expect three potential errors:
  //   2 missing interactions: q0 <-> q1 and q0 <-> q2
  //   1 control always zero: q1 & q2 in instruction 6
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      3);
  ASSERT_EQ(causes[0].type, MissingInteraction);
  ASSERT_EQ(causes[0].instruction, 7);
  ASSERT_EQ(causes[1].type, MissingInteraction);
  ASSERT_EQ(causes[1].instruction, 7);
  ASSERT_EQ(causes[2].type, ControlAlwaysZero);
  ASSERT_EQ(causes[2].instruction, 6);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 28);
  // We expect one potential error: Control always zero in instruction 6
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      1);
  ASSERT_EQ(causes[0].type, ControlAlwaysZero);
  ASSERT_EQ(causes[0].instruction, 6);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 31);
  // We expect no potential errors, as instruction 6 is no longer always 0
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      0);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), false);
  ASSERT_EQ(state->isFinished(state), true);
}
