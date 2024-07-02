//
// Created by damian on 7/2/24.
//

#include "frontend/cli/CliFrontEnd.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <iostream>
#include <memory>

void clearScreen() {
    // Clear the screen using an ANSI escape sequence
    std::cout << "\033[2J\033[1;1H";
}

void CliFrontEnd::initCode(const char *code) {
    lines.clear();
    std::string token;
    std::istringstream tokenStream(code);
    while (std::getline(tokenStream, token, ';')) {
        lines.push_back(token);
    }
    lines.emplace_back("END");
}

void CliFrontEnd::run(const char *code, SimulationState* state) {
    initCode(code);

    auto command = std::make_unique<char[]>(100);
    state->loadCode(state, code);
    bool wasError = false;
    bool wasGet = false;

    while(strcmp(command.get(), "exit") != 0) {
        clearScreen();
        if(wasError) {
            printf("Invalid command. Choose one of:\n");
            printf("run\t");
            printf("step\t");
            printf("back\t");
            printf("get <variable>\t");
            printf("reset\t");
            printf("exit\n\n");
            wasError = false;
        }
        if(wasGet) {
            Variable v;
            if(state->getClassicalVariable(state, command.get() + 4, &v) == ERROR) {
                printf("Variable %s not found\n", command.get() + 4);
            } else {
                if (v.type == VAR_BOOL) {
                    printf("%s = %s\n", command.get() + 4, v.value.bool_value ? "true" : "false");
                } else if (v.type == VAR_INT) {
                    printf("%s = %d\n", command.get() + 4, v.value.int_value);
                } else if (v.type == VAR_FLOAT) {
                    printf("%s = %f\n", command.get() + 4, v.value.float_value);
                }
            }
            wasGet = false;
        }
        printState(state);
        printf("Enter command: ");
        std::cin.getline(command.get(), 100);
        if(strcmp(command.get(), "run") == 0) {
            state->runSimulation(state);
        } else if(strcmp(command.get(), "step") == 0) {
            state->stepForward(state);
        } else if(strcmp(command.get(), "back") == 0) {
            state->stepBackward(state);
        } else if(strcmp(command.get(), "reset") == 0) {
            state->resetSimulation(state);
        } else if(strcmp(command.get(), "fix") == 0) {

        } else if(strlen(command.get()) >= 5 && strncmp(command.get(), "get ", 4) == 0) {
            wasGet = true;
        } else {
            wasError = true;
        }
    }
}

void CliFrontEnd::printState(SimulationState *state) {
    for(size_t i = 0; i < lines.size(); i++) {
        if(i == state->getCurrentLine(state)) {
            printf("%s > ", ANSI_BG_YELLOW);
        } else {
            printf("%s   ", ANSI_BG_RESET);
        }
        printf("%s\t\t\t\t%s\n", lines[i].c_str(), ANSI_BG_RESET);
    }
    printf("\n");

    const char* bitStrings[] = {"000", "001", "010", "011", "100", "101", "110", "111"};
    Complex c;
    for(auto & bitString : bitStrings) {
        state->getAmplitudeBitstring(state, bitString, &c);
        printf("%s %f\t||\t", bitString, c.real);
    }
    printf("\n");
    if(state->didAssertionFail(state)) {
        printf("THE PREVIOUS LINE FAILED AN ASSERTION\n");
    }
}