#pragma once

#include "backend/debug.h"

#include <string>
#include <vector>

#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_RESET "\x1b[0m"

class CliFrontEnd {
public:
  void run(const char* code, SimulationState* state);

private:
  std::string currentCode;

  void printState(SimulationState* state);
  void initCode(const char* code);
};
