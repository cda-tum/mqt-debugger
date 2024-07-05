//
// Created by damian on 7/2/24.
//

#include "frontend/cli/CliFrontEnd.hpp"

#include "common/parsing/Utils.hpp"

#include <array>
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
  if (result == ERROR) {
    std::cout << "Error loading code\n";
    return;
  }
  bool wasError = false;
  bool wasGet = false;

  while (command != "exit") {
    clearScreen();
    if (wasError) {
      std::cout << "Invalid command. Choose one of:\n";
      std::cout << "run\t";
      std::cout << "step\t";
      std::cout << "back\t";
      std::cout << "get <variable>\t";
      std::cout << "reset\t";
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
    printState(state);
    std::cout << "Enter command: ";
    std::getline(std::cin, command);
    if (command == "run") {
      state->runSimulation(state);
    } else if (command == "step" || command.empty()) {
      state->stepForward(state);
    } else if (command == "back") {
      state->stepBackward(state);
    } else if (command == "reset") {
      state->resetSimulation(state);
    } else if (command.length() >= 5 && command.substr(0, 4) == "get ") {
      wasGet = true;
    } else {
      wasError = true;
    }
  }
}

void CliFrontEnd::printState(SimulationState* state) {
  size_t currentStart = 0;
  size_t currentEnd = 0;
  const Result result =
      state->getCurrentInstructionPosition(state, &currentStart, &currentEnd);

  std::cout << currentCode.substr(0, currentStart);
  if (result == OK) {
    std::cout << ANSI_BG_YELLOW
              << currentCode.substr(currentStart,
                                    currentEnd - currentStart + 1);
    std::cout << ANSI_BG_RESET
              << currentCode.substr(currentEnd + 1,
                                    currentCode.length() - currentEnd);
  } else {
    std::cout << ANSI_BG_RESET << currentCode;
  }
  std::cout << "\n";

  const std::array<const char*, 8> bitStrings = {"000", "001", "010", "011",
                                                 "100", "101", "110", "111"};
  Complex c;
  for (const auto* bitString : bitStrings) {
    state->getAmplitudeBitstring(state, bitString, &c);
    std::cout << bitString << " " << c.real << "\t||\t";
  }
  std::cout << "\n";
  if (state->didAssertionFail(state)) {
    std::cout << "THE PREVIOUS LINE FAILED AN ASSERTION\n";
  }
}
