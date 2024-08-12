#include "common/parsing/AssertionParsing.hpp"

#include "common.h"
#include "common/parsing/ParsingError.hpp"
#include "common/parsing/Utils.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

Assertion::Assertion(std::vector<std::string> inputTargetQubits,
                     AssertionType assertionType)
    : targetQubits(std::move(inputTargetQubits)), type(assertionType) {}

AssertionType Assertion::getType() const { return type; }

const std::vector<std::string>& Assertion::getTargetQubits() const {
  return targetQubits;
}

EntanglementAssertion::EntanglementAssertion(
    std::vector<std::string> inputTargetQubits)
    : Assertion(std::move(inputTargetQubits), AssertionType::Entanglement) {}

SuperpositionAssertion::SuperpositionAssertion(
    std::vector<std::string> inputTargetQubits)
    : Assertion(std::move(inputTargetQubits), AssertionType::Superposition) {}

EqualityAssertion::EqualityAssertion(double inputSimilarityThreshold,
                                     std::vector<std::string> inputTargetQubits,
                                     AssertionType assertionType)
    : Assertion(std::move(inputTargetQubits), assertionType),
      similarityThreshold(inputSimilarityThreshold) {
  if (similarityThreshold < 0 || similarityThreshold > 1) {
    throw ParsingError("Similarity threshold must be between 0 and 1");
  }
}
double EqualityAssertion::getSimilarityThreshold() const {
  return similarityThreshold;
}

StatevectorEqualityAssertion::StatevectorEqualityAssertion(
    Statevector inputTargetStatevector, double inputSimilarityThreshold,
    std::vector<std::string> inputTargetQubits)
    : EqualityAssertion(inputSimilarityThreshold, std::move(inputTargetQubits),
                        AssertionType::StatevectorEquality),
      targetStatevector(inputTargetStatevector) {
  if (targetStatevector.numQubits != getTargetQubits().size()) {
    throw ParsingError(
        "Number of target qubits must match number of qubits in statevector");
  }
}
const Statevector& StatevectorEqualityAssertion::getTargetStatevector() const {
  return targetStatevector;
}
StatevectorEqualityAssertion::~StatevectorEqualityAssertion() {
  delete[] targetStatevector.amplitudes;
}

CircuitEqualityAssertion::CircuitEqualityAssertion(
    std::string inputCircuitCode, double inputSimilarityThreshold,
    std::vector<std::string> inputTargetQubits)
    : EqualityAssertion(inputSimilarityThreshold, std::move(inputTargetQubits),
                        AssertionType::CircuitEquality),
      circuitCode(std::move(inputCircuitCode)) {}
const std::string& CircuitEqualityAssertion::getCircuitCode() const {
  return circuitCode;
}

std::vector<std::string> extractTargetQubits(const std::string& targetPart) {
  return splitString(targetPart, ',');
}

Complex parseComplex(std::string complexString) {
  complexString = removeWhitespace(complexString);
  auto parts = splitString(complexString, '+');
  double real = 0;
  double imaginary = 0;
  for (auto& part : parts) {
    if (part.find('i') != std::string::npos &&
        part.find('j') != std::string::npos) {
      imaginary +=
          std::stod(replaceString(replaceString(part, "i", ""), "j", ""));
    } else {
      real += std::stod(part);
    }
  }
  return {real, imaginary};
}

Statevector parseStatevector(std::string statevectorString) {
  statevectorString = removeWhitespace(statevectorString);
  auto parts = splitString(statevectorString, ',');
  auto amplitudes = std::make_unique<std::vector<Complex>>();
  for (auto& part : parts) {
    amplitudes->push_back(parseComplex(part));
  }

  size_t numQubits = 0;
  size_t n = amplitudes->size();
  while (n > 1) {
    if ((n & 1) == 1) {
      throw ParsingError("Invalid statevector size");
    }
    n >>= 1;
    numQubits++;
  }

  Statevector sv;
  sv.numStates = amplitudes->size();
  sv.numQubits = numQubits;
  sv.amplitudes = amplitudes.release()->data();

  return sv;
}

bool isAssertion(std::string expression) {
  expression = trim(expression);
  return startsWith(expression, "assert-ent") ||
         startsWith(expression, "assert-sup") ||
         startsWith(expression, "assert-eq");
}

std::unique_ptr<Assertion> parseAssertion(std::string assertionString,
                                          const std::string& blockContent) {
  assertionString = trim(replaceString(assertionString, ";", ""));

  if (startsWith(assertionString, "assert-ent")) {
    auto targets = extractTargetQubits(assertionString.substr(11));
    return std::make_unique<EntanglementAssertion>(targets);
  }
  if (startsWith(assertionString, "assert-sup")) {
    auto targets = extractTargetQubits(assertionString.substr(11));
    return std::make_unique<SuperpositionAssertion>(targets);
  }
  if (startsWith(assertionString, "assert-eq")) {
    auto sub = assertionString.substr(10);
    auto targets = extractTargetQubits(sub);
    double similarityThreshold = 0;
    try {
      similarityThreshold = std::stod(targets[0]);
      targets.erase(targets.begin());
    } catch (const std::invalid_argument& e) {
      similarityThreshold = 1.0;
    } catch (const std::out_of_range& e) {
      similarityThreshold = 1.0;
    }

    if (blockContent.find(';') == std::string::npos) {
      auto statevector = parseStatevector(blockContent);
      return std::make_unique<StatevectorEqualityAssertion>(
          statevector, similarityThreshold, targets);
    }
    auto circuitCode = trim(blockContent);
    return std::make_unique<CircuitEqualityAssertion>(
        circuitCode, similarityThreshold, targets);
  }
  throw ParsingError("Expression is not a valid assertion");
}
