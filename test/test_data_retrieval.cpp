#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "common.h"
#include "utils_test.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <vector>

class DataRetrievalTest : public testing::Test {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    loadFromFile("classical-storage");
  }

protected:
  DDSimulationState ddState;
  SimulationState* state = nullptr;

  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }

  void forwardTo(size_t instruction) {
    size_t currentInstruction = state->getCurrentInstruction(state);
    while (currentInstruction < instruction) {
      state->stepForward(state);
      currentInstruction = state->getCurrentInstruction(state);
    }
  }
};

TEST_F(DataRetrievalTest, GetNumQubits) {
  ASSERT_EQ(state->getNumQubits(state), 4);
}

TEST_F(DataRetrievalTest, GetNumClassicalVariables) {
  ASSERT_EQ(state->getNumClassicalVariables(state), 4);
}

TEST_F(DataRetrievalTest, GetAmplitudes) {
  Complex result;

  forwardTo(6);
  ASSERT_EQ(state->getAmplitudeIndex(state, 0, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, 3, &result), OK);
  ASSERT_TRUE(complexEquality(result, 1.0, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0011", &result), OK);
  ASSERT_TRUE(complexEquality(result, 1.0, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));

  forwardTo(12);
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &result), OK);
  ASSERT_TRUE(complexEquality(result, -0.707, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.707, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1010", &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, 3, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));

  forwardTo(13);
  Complex c1;
  Complex c2;
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &c1), OK);
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &c2), OK);
  ASSERT_TRUE(
      (complexEquality(c1, 0.0, 0.0) && complexEquality(c2, -1.0, 0.0)) ||
      (complexEquality(c1, 1.0, 0.0) && complexEquality(c2, 0.0, 0.0)));
  const size_t baseIndex = complexEquality(c1, 1.0, 0.0) ? 2 : 11;

  forwardTo(14);
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex + 4, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, baseIndex == 2 ? 1 : -1.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));

  forwardTo(15);
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex + 2, &result), OK);
  ASSERT_TRUE(complexEquality(result, baseIndex == 2 ? 1.0 : -1.0, 0.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));
}

TEST_F(DataRetrievalTest, GetClassicalVariableNames) {
  std::array<char, 256> name = {0};
  std::vector<std::string> expectedNames = {"c[0]", "c[1]", "c[2]", "hello[0]"};
  for (size_t i = 0; i < expectedNames.size(); i++) {
    ASSERT_EQ(state->getClassicalVariableName(state, i, name.data()), OK);
    ASSERT_STREQ(name.data(), expectedNames[i].c_str());
  }
}

TEST_F(DataRetrievalTest, GetClassicalVariable) {
  Variable v;

  forwardTo(6);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, false));

  forwardTo(7);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, true));
  ASSERT_EQ(state->getClassicalVariable(state, "c[1]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, true));

  forwardTo(10);
  ASSERT_EQ(state->getClassicalVariable(state, "c[2]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, true));

  forwardTo(13);
  ASSERT_EQ(state->getClassicalVariable(state, "hello[0]", &v), OK);
  const bool entangledValue = v.value.boolValue;
  if (entangledValue) {
    Complex c;
    ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &c), OK);
    ASSERT_TRUE(c.real == -1 || c.real == 1);
  } else {
    Complex c;
    ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &c), OK);
    ASSERT_TRUE(c.real == -1 || c.real == 1);
  }

  forwardTo(16);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, entangledValue));
}

TEST_F(DataRetrievalTest, GetStateVectorFull) {
  std::array<Complex, 16> amplitudes{};
  Statevector sv{4, 16, amplitudes.data()};

  ASSERT_EQ(state->getStateVectorFull(state, &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[0], 1.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], 0.0, 0.0));

  forwardTo(12);
  ASSERT_EQ(state->getStateVectorFull(state, &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[2], 0.707, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[11], -0.707, 0.0));
}

TEST_F(DataRetrievalTest, GetStateVectorSub) {
  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};

  forwardTo(6);
  std::array<size_t, 2> qubits = {0, 1};
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[3], 1, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[0], 0.0, 0.0));

  qubits[1] = 2;
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[3], 0.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], 1.0, 0.0));

  forwardTo(11);
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[0], 0.707, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], -0.707, 0.0));

  qubits[0] = 1;
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[0], 0.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], 1.0,
                              0.0)); // 0 because of destructive interference
  ASSERT_TRUE(complexEquality(amplitudes[2], 0.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[3], 0.0, 0.0));
}
