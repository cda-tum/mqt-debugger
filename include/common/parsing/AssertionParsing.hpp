#pragma once

#include "common.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class AssertionType : uint8_t {
  Entanglement,
  Superposition,
  StatevectorEquality,
  CircuitEquality
};

class Assertion {
  std::vector<std::string> targetQubits;
  AssertionType type;

public:
  explicit Assertion(std::vector<std::string> targetQubits, AssertionType type);
  [[nodiscard]] AssertionType getType() const;
  [[nodiscard]] const std::vector<std::string>& getTargetQubits() const;
  virtual ~Assertion() = default;
  void setTargetQubits(std::vector<std::string> newTargetQubits);
  virtual void validate() {};
};

class EntanglementAssertion : public Assertion {
public:
  explicit EntanglementAssertion(std::vector<std::string> targetQubits);
};

class SuperpositionAssertion : public Assertion {
public:
  explicit SuperpositionAssertion(std::vector<std::string> targetQubits);
};

class EqualityAssertion : public Assertion {
  double similarityThreshold;

public:
  EqualityAssertion(double similarityThreshold,
                    std::vector<std::string> targetQubits, AssertionType type);
  [[nodiscard]] double getSimilarityThreshold() const;
  void validate() override;
};

class StatevectorEqualityAssertion : public EqualityAssertion {
  Statevector targetStatevector;

public:
  StatevectorEqualityAssertion(Statevector targetStatevector,
                               double similarityThreshold,
                               std::vector<std::string> targetQubits);
  [[nodiscard]] const Statevector& getTargetStatevector() const;

  ~StatevectorEqualityAssertion() override;
  void validate() override;
};

class CircuitEqualityAssertion : public EqualityAssertion {
  std::string circuitCode;

public:
  CircuitEqualityAssertion(std::string circuitCode, double similarityThreshold,
                           std::vector<std::string> targetQubits);
  [[nodiscard]] const std::string& getCircuitCode() const;
};

std::unique_ptr<Assertion> parseAssertion(std::string assertionString,
                                          const std::string& blockContent);
bool isAssertion(std::string expression);
