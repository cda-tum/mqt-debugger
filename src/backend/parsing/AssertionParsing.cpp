#include "backend/parsing/AssertionParsing.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <utility>

std::string trim(const std::string& str) {
  auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
  auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
  return (start < end) ? std::string(start, end) : std::string();
}

bool startsWith(const std::string& str, const std::string& prefix) {
  return str.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> splitString(std::string& text, char delimiter) {
  std::vector<std::string> result;
  std::istringstream iss(text);
  for (std::string s; std::getline(iss, s, delimiter);) {
    result.push_back(s);
  }
  return result;
}

std::string replaceString(std::string str, const std::string& from,
                          const std::string& to) {
  size_t startPos = 0;
  while ((startPos = str.find(from, startPos)) != std::string::npos) {
    str.replace(startPos, from.length(), to);
    startPos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}

std::string removeWhitespace(std::string str) {
  str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
  return str;
}

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

SpanAssertion::SpanAssertion(std::vector<Statevector> inputSpanVectors,
                             std::vector<std::string> inputTargetQubits)
    : Assertion(std::move(inputTargetQubits), AssertionType::Span),
      spanVectors(std::move(inputSpanVectors)) {}
const std::vector<Statevector>& SpanAssertion::getSpanVectors() const {
  return spanVectors;
}
SpanAssertion::~SpanAssertion() {
  for (auto& statevector : spanVectors) {
    delete[] statevector.amplitudes;
  }
}

EqualityAssertion::EqualityAssertion(double inputSimilarityThreshold,
                                     std::vector<std::string> inputTargetQubits,
                                     AssertionType assertionType)
    : Assertion(std::move(inputTargetQubits), assertionType),
      similarityThreshold(inputSimilarityThreshold) {}
double EqualityAssertion::getSimilarityThreshold() const {
  return similarityThreshold;
}

StatevectorEqualityAssertion::StatevectorEqualityAssertion(
    Statevector inputTargetStatevector, double inputSimilarityThreshold,
    std::vector<std::string> inputTargetQubits)
    : EqualityAssertion(inputSimilarityThreshold, std::move(inputTargetQubits),
                        AssertionType::StatevectorEquality),
      targetStatevector(inputTargetStatevector) {}
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

std::vector<std::string> extractTargetQubits(std::string targetPart) {
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
  while (n > 0) {
    if ((n & 1) == 1 && n != 1) {
      throw std::runtime_error("Invalid statevector size");
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
         startsWith(expression, "assert-span") ||
         startsWith(expression, "assert-eq");
}

std::unique_ptr<Assertion> parseAssertion(std::string assertionString) {
  assertionString = trim(replaceString(assertionString, ";", ""));

  if (startsWith(assertionString, "assert-ent")) {
    auto targets = extractTargetQubits(assertionString.substr(11));
    return std::make_unique<EntanglementAssertion>(targets);
  }
  if (startsWith(assertionString, "assert-sup")) {
    auto targets = extractTargetQubits(assertionString.substr(11));
    return std::make_unique<SuperpositionAssertion>(targets);
  }
  if (startsWith(assertionString, "assert-span")) {
    auto sub = assertionString.substr(12);
    auto parts = splitString(sub, '{');
    auto targets = extractTargetQubits(parts[0]);
    auto spanString = trim(splitString(parts[1], '}')[0]);
    auto statevectors = splitString(spanString, ';');
    std::vector<Statevector> statevectorList;
    for (auto& statevector : statevectors) {
      statevectorList.emplace_back(parseStatevector(statevector));
    }

    auto assertion = std::make_unique<SpanAssertion>(statevectorList, targets);
    auto svSize = statevectorList[0].numStates;
    for (auto& sv : statevectorList) {
      if (sv.numStates != svSize) {
        throw std::runtime_error(
            "Statevectors in span assertion must have the same size");
      }
    }
    return assertion;
  }
  if (startsWith(assertionString, "assert-eq")) {
    auto sub = assertionString.substr(10);
    auto parts = splitString(sub, '{');
    auto targets = extractTargetQubits(parts[0]);
    auto bodyString = trim(splitString(parts[1], '}')[0]);
    auto thresholdParts = splitString(parts[1], '}');
    auto threshold = thresholdParts.size() > 1
                         ? removeWhitespace(thresholdParts[1])
                         : std::string("1.0");
    auto similarityThreshold =
        threshold.length() > 0 ? std::stod(threshold) : 1.0;

    if (bodyString.find(';') == std::string::npos) {
      auto statevector = parseStatevector(bodyString);
      return std::make_unique<StatevectorEqualityAssertion>(
          statevector, similarityThreshold, targets);
    }
    auto circuitCode = trim(bodyString);
    return std::make_unique<CircuitEqualityAssertion>(
        circuitCode, similarityThreshold, targets);
  }
  throw std::runtime_error("Expression is not a valid assertion");
}
