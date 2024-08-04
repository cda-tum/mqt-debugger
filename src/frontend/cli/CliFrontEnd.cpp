//
// Created by damian on 7/2/24.
//

#include "frontend/cli/CliFrontEnd.hpp"

#include "backend/dd/DDSimDebug.hpp"
#include "common/parsing/Utils.hpp"

#include <array>
#include <cstdint>
#include <iostream>

void clearScreen() {
  // Clear the screen using an ANSI escape sequence
  std::cout << "\033[2J\033[1;1H";
}

void CliFrontEnd::initCode(const char* code) { currentCode = code; }

void CliFrontEnd::run(const char* code, SimulationState* state) {
  initCode(code);

  std::string command;
  const auto result = state->loadCode(state, code);
  // const auto* ddsim = reinterpret_cast<DDSimulationState*>(state);
  // std::cout << ddsim->processedCode;
  // std::cin >> command;
  state->resetSimulation(state);
  if (result == ERROR) {
    std::cout << "Error loading code\n";
    return;
  }

  bool wasError = false;
  bool wasGet = false;
  size_t inspecting = 23870;

  while (command != "exit") {
    clearScreen();
    if (wasError) {
      std::cout << "Invalid command. Choose one of:\n";
      std::cout << "run\t";
      std::cout << "run back [rb]\t";
      std::cout << "step [enter]\t";
      std::cout << "step over [o]\t";
      std::cout << "back [b]\t";
      std::cout << "back over [bo]\t";
      std::cout << "get <variable>\t";
      std::cout << "reset\t";
      std::cout << "inspect\t";
      std::cout << "exit\n\n";
      wasError = false;
    }
    if (wasGet) {
      Variable v;
      if (state->getClassicalVariable(
              state, command.substr(4, command.length() - 4).c_str(), &v) ==
          ERROR) {
        std::cout << "Variable " << command << " not found\n";
      } else {
        if (v.type == VarBool) {
          std::cout << command.substr(4, command.length() - 4) << " = "
                    << (v.value.boolValue ? "true" : "false") << "\n";
        } else if (v.type == VarInt) {
          std::cout << command.substr(4, command.length() - 4) << " = "
                    << v.value.intValue << "\n";
        } else if (v.type == VarFloat) {
          std::cout << command.substr(4, command.length() - 4) << " = "
                    << v.value.floatValue << "\n";
        }
      }
      wasGet = false;
    }
    printState(state, inspecting, true);

    std::cout << "Enter command: ";
    std::getline(std::cin, command);
    if (command == "run") {
      state->runSimulation(state);
    } else if (command == "run back" || command == "rb") {
      state->runSimulationBackward(state);
    } else if (command == "step" || command.empty()) {
      state->stepForward(state);
    } else if (command == "step over" || command == "o") {
      state->stepOverForward(state);
    } else if (command == "back" || command == "b") {
      state->stepBackward(state);
    } else if (command == "back over" || command == "bo") {
      state->stepOverBackward(state);
    } else if (command == "reset") {
      state->resetSimulation(state);
    } else if (command.length() >= 5 && command.substr(0, 4) == "get ") {
      wasGet = true;
    } else if (command == "inspect") {
      inspecting = state->getCurrentInstruction(state);
    } else if (command == "diagnose") {
      std::vector<ErrorCause> problems(10);
      const auto count = state->getDiagnostics(state)->potentialErrorCauses(
          state->getDiagnostics(state), problems.data(), problems.size());
      std::cout << count << " potential problems found\n";
    } else {
      wasError = true;
    }
  }
}

void CliFrontEnd::printState(SimulationState* state, size_t inspecting,
                             bool codeOnly) {
  std::vector<size_t> highlightIntervals;
  if (inspecting != -1ULL) {
    std::vector<uint8_t> inspectingDependencies(
        state->getInstructionCount(state));
    auto deps = inspectingDependencies.data();
    state->getDiagnostics(state)->getDataDependencies(
        state->getDiagnostics(state), inspecting,
        reinterpret_cast<bool*>(deps));
    uint8_t on = 0;
    for (size_t i = 0; i < inspectingDependencies.size(); i++) {
      if (inspectingDependencies[i] != on) {
        on = inspectingDependencies[i];
        size_t start = 0;
        size_t end = 0;
        state->getInstructionPosition(state, i, &start, &end);
        highlightIntervals.push_back(start);
      }
    }
  }
  if (highlightIntervals.empty()) {
    highlightIntervals.push_back(0);
  }
  highlightIntervals.push_back(currentCode.length() + 1);
  size_t currentStart = 0;
  size_t currentEnd = 0;
  const Result res = state->getInstructionPosition(
      state, state->getCurrentInstruction(state), &currentStart, &currentEnd);

  size_t currentPos = 0;
  bool on = false;
  for (const auto nextInterval : highlightIntervals) {
    const auto textColor = on ? ANSI_BG_RESET : ANSI_COL_GRAY;
    if (res == OK && currentStart >= currentPos &&
        currentStart < nextInterval) {
      std::cout << textColor
                << currentCode.substr(currentPos, currentStart - currentPos)
                << ANSI_BG_RESET;
      std::cout << ANSI_BG_YELLOW
                << currentCode.substr(currentStart,
                                      currentEnd - currentStart + 1)
                << ANSI_BG_RESET;
      std::cout << textColor
                << currentCode.substr(currentEnd + 1,
                                      nextInterval - currentEnd - 1)
                << ANSI_BG_RESET;
    } else {
      std::cout << textColor
                << currentCode.substr(currentPos, nextInterval - currentPos)
                << ANSI_BG_RESET;
    }
    on = !on;
    currentPos = nextInterval;
  }
  std::cout << "\n";

  if (!codeOnly) {
    const std::array<const char*, 8> bitStrings = {"000", "001", "010", "011",
                                                   "100", "101", "110", "111"};
    Complex c;
    for (const auto* bitString : bitStrings) {
      state->getAmplitudeBitstring(state, bitString, &c);
      std::cout << bitString << " " << c.real << "\t||\t";
    }
    std::cout << "\n";
  }
  if (state->didAssertionFail(state)) {
    std::cout << "THIS LINE FAILED AN ASSERTION\n";
  }
}
