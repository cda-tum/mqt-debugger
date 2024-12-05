/**
 * @file AssertionParsing.cpp
 * @brief Implementation of the assertion parsing functionality.
 */

#include "common/parsing/AssertionParsing.hpp"

#include "common.h"
#include "common/parsing/ParsingError.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
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

void Assertion::setTargetQubits(std::vector<std::string> newTargetQubits) {
  targetQubits = std::move(newTargetQubits);
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
      similarityThreshold(inputSimilarityThreshold) {}

void EqualityAssertion::validate() {
  if (similarityThreshold < 0 || similarityThreshold > 1) {
    throw ParsingError("Similarity threshold must be between 0 and 1");
  }
  Assertion::validate();
}

double EqualityAssertion::getSimilarityThreshold() const {
  return similarityThreshold;
}

StatevectorEqualityAssertion::StatevectorEqualityAssertion(
    Statevector inputTargetStatevector, double inputSimilarityThreshold,
    std::vector<std::string> inputTargetQubits)
    : EqualityAssertion(inputSimilarityThreshold, std::move(inputTargetQubits),
                        AssertionType::StatevectorEquality),
      targetStatevector(inputTargetStatevector) {}

void StatevectorEqualityAssertion::validate() {
  if (targetStatevector.numQubits != getTargetQubits().size()) {
    throw ParsingError(
        "Number of target qubits must match number of qubits in statevector");
  }
  EqualityAssertion::validate();
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

/**
 * @brief Extract the targets of the assertion from its string representation.
 * @param targetPart The string representation of the full targets of the
 * assertion.
 * @return The targets of the assertion.
 */
std::vector<std::string> extractTargetQubits(const std::string& targetPart) {
  const auto parts = splitString(targetPart, ',');
  std::vector<std::string> trimmedParts(parts.size());
  std::transform(parts.begin(), parts.end(), trimmedParts.begin(), trim);
  return trimmedParts;
}

/**
 * @brief Parse a complex number from a string.
 * @param complexString The string representation of the complex number.
 * @return The parsed complex number.
 */
Complex parseComplex(std::string complexString) {
  complexString = removeWhitespace(complexString);
  auto parts = splitString(complexString, '-');
  bool negativeSplit = true;
  if (parts[0].empty()) {
    parts.erase(parts.begin());
    parts[0] = "-" + parts[0];
  }
  if (parts.size() == 1) {
    negativeSplit = false;
    parts = splitString(complexString, '+');
  }
  double real = 0;
  double imaginary = 0;
  bool first = true;
  for (auto& part : parts) {
    if (part.find('i') != std::string::npos ||
        part.find('j') != std::string::npos) {
      imaginary +=
          std::stod(replaceString(replaceString(part, "i", ""), "j", "")) *
          (negativeSplit && !first ? -1 : 1);
    } else {
      real += std::stod(part) * (negativeSplit && !first ? -1 : 1);
    }
    first = false;
  }
  return {real, imaginary};
}

/**
 * @brief Parse a statevector from a string.
 * @param statevectorString The string representation of the statevector.
 * @return The parsed statevector.
 */
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

/**
 * @brief Check if the given expression is an assertion.
 *
 * This is done by checking if it starts with `assert-ent`, `assert-sup`, or
 * `assert-eq`.
 * @param expression The expression to check.
 * @return True if the expression is an assertion, false otherwise.
 */
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
      throw ParsingError(
          "Similarity threshold out of range. It must be between 0 and 1");
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
