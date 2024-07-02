#pragma once

#include <vector>
#include <string>
#include "backend/debug.h"

#define ANSI_BG_YELLOW    "\x1b[43m"
#define ANSI_BG_RESET     "\x1b[0m"


class CliFrontEnd {
public:
    void run(const char *code, SimulationState* state);
private:
    std::vector<std::string> lines;

    void printState(SimulationState* state);
    void initCode(const char* code);

};
