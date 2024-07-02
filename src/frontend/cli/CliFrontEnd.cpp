//
// Created by damian on 7/2/24.
//

#include "frontend/cli/CliFrontEnd.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>

void clearScreen() {
  // Clear the screen using an ANSI escape sequence
  std::cout << "\033[2J\033[1;1H";
}

void CliFrontEnd::initCode(const char* code) {
  lines.clear();
  std::string token;
  std::istringstream tokenStream(code);
  while (std::getline(tokenStream, token, ';')) {
    lines.push_back(token);
  }
  lines.emplace_back("END");
}

void CliFrontEnd::run(const char* code, SimulationState* state) {
  initCode(code);

  std::string command;
  state->loadCode(state, code);
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
    } else if (command == "step") {
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
  for (size_t i = 0; i < lines.size(); i++) {
    if (i == state->getCurrentLine(state)) {
      std::cout << ANSI_BG_YELLOW << " > ";
    } else {
      std::cout << ANSI_BG_RESET << " > ";
    }
    std::cout << lines[i].c_str() << "\t\t\t\t" << ANSI_BG_RESET << "\n";
  }
  std::cout << "\n";

  const char* bitStrings[] = {"000", "001", "010", "011",
                              "100", "101", "110", "111"};
  Complex c;
  for (auto& bitString : bitStrings) {
    state->getAmplitudeBitstring(state, bitString, &c);
    std::cout << bitString << " " << c.real << "\t||\t";
  }
  std::cout << "\n";
  if (state->didAssertionFail(state)) {
    std::cout << "THE PREVIOUS LINE FAILED AN ASSERTION\n";
  }
}
