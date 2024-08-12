#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/ParsingError.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>

class ParsingTest : public testing::Test {
  void SetUp() override {}
};

TEST_F(ParsingTest, EqualityAssertion) {
  // With statevector
  const auto a1 = parseAssertion("assert-eq 0.5, q[0]", "1, 0");
  ASSERT_EQ(a1->getType(), AssertionType::StatevectorEquality);
  ASSERT_EQ(a1->getTargetQubits().size(), 1);
  ASSERT_EQ(a1->getTargetQubits()[0], "q[0]");
  const auto* sv = dynamic_cast<StatevectorEqualityAssertion*>(a1.get());
  ASSERT_EQ(sv->getSimilarityThreshold(), 0.5);
  ASSERT_EQ(sv->getTargetStatevector().numQubits, 1);
  ASSERT_EQ(sv->getTargetStatevector().numStates, 2);
  ASSERT_EQ(sv->getTargetStatevector().amplitudes->real, 1);

  // With circuit
  const auto a2 = parseAssertion("assert-eq 0.5, q[0]", "qreg q[1]; h q[0];");
  ASSERT_EQ(a2->getType(), AssertionType::CircuitEquality);
  ASSERT_EQ(a2->getTargetQubits().size(), 1);
  ASSERT_EQ(a2->getTargetQubits()[0], "q[0]");
  const auto* c = dynamic_cast<CircuitEqualityAssertion*>(a2.get());
  ASSERT_EQ(c->getSimilarityThreshold(), 0.5);
  ASSERT_EQ(c->getCircuitCode(), "qreg q[1]; h q[0];");
}

TEST_F(ParsingTest, EntanglementAssertion) {
  const auto a = parseAssertion("assert-ent q[0], q[1]", "");
  ASSERT_EQ(a->getType(), AssertionType::Entanglement);
  ASSERT_EQ(a->getTargetQubits().size(), 2);
  ASSERT_EQ(a->getTargetQubits()[0], "q[0]");
  ASSERT_EQ(a->getTargetQubits()[1], "q[1]");
}

TEST_F(ParsingTest, SuperpositionAssertion) {
  const auto a = parseAssertion("assert-sup q[0], q[1]", "");
  ASSERT_EQ(a->getType(), AssertionType::Superposition);
  ASSERT_EQ(a->getTargetQubits().size(), 2);
  ASSERT_EQ(a->getTargetQubits()[0], "q[0]");
  ASSERT_EQ(a->getTargetQubits()[1], "q[1]");
}

TEST_F(ParsingTest, ErrorStatevectorEqualityAssertion) {
  ASSERT_THROW(parseAssertion("assert-eq 1.5, q[0]", "1, 0"), ParsingError);
  ASSERT_THROW(parseAssertion("assert-eq 0.5, q[0]", "1, 0, 0"), ParsingError);
  ASSERT_THROW(parseAssertion("assert-eq 0.5, q[0]", "1, 0, 0, 0"),
               ParsingError);
}

TEST_F(ParsingTest, ErrorCircuitEqualityAssertion) {
  ASSERT_THROW(parseAssertion("assert-eq 1.5, q[0]", "qreg q[1]; h q[0];"),
               ParsingError);
}

TEST_F(ParsingTest, ErrorInvalidAssertion) {
  ASSERT_THROW(parseAssertion("assert-fake q[0]", ""), ParsingError);
}
