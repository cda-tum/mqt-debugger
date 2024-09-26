/**
 * @file CliFrontEnd.hpp
 * @brief Provides a CLI frontend for the debugger.
 *
 * This file contains the declaration of the CliFrontEnd class, which provides
 * a command-line interface for the debugger.
 */

#pragma once

#include "backend/debug.h"

#include <cstddef>
#include <string>

#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_RESET "\x1b[0m"
#define ANSI_COL_GRAY "\x1b[90m"

/**
 * @brief A command-line interface for the debugger.
 *
 * By creating an instance of this class and calling the `run` method, the user
 * can interact with the debugger using a command-line interface.
 */
class CliFrontEnd {
public:
  /**
   * @brief Runs the debugger with the given code and state.
   * @param code The code to run (compatible with the provided
   * `SimulationState`)
   * @param state The state to run the code on
   */
  void run(const char* code, SimulationState* state);

private:
  /**
   * @brief The current code being executed. Used to display the code in the
   * CLI.
   */
  std::string currentCode;

  /**
   * @brief Print the current state (state vector etc.) in the command line.
   */
  void printState(SimulationState* state, size_t inspecting,
                  bool codeOnly = false);

  /**
   * @brief Initialize the code for running it at a later time.
   */
  void initCode(const char* code);
};
