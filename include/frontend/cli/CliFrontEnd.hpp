#pragma once

#include "backend/debug.h"

#include <cstddef>
#include <string>

#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_RESET "\x1b[0m"
#define ANSI_COL_GRAY "\x1b[90m"

class CliFrontEnd {
public:
  void run(const char* code, SimulationState* state);

private:
  std::string currentCode;

  void printState(SimulationState* state, size_t inspecting,
                  bool codeOnly = false);
  void initCode(const char* code);
};
