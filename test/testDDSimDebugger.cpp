#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ddsimDebug.hpp"

#define ANSI_BG_YELLOW    "\x1b[43m"
#define ANSI_BG_RESET     "\x1b[0m"

// Entanglement test
const char* code = "qreg q[3];creg c[3];h q[0];cx q[0], q[1];cx q[2], q[0];assert-ent q[0], q[1];assert-ent q[2], q[0];";

// Superposition test
//const char* code = "qreg q[3];creg c[3];h q[0];cx q[0], q[1];cx q[2], q[0];assert-sup q[0];assert-sup q[1];assert-sup q[2];assert-sup q[0],q[1];assert-sup q[0],q[2];assert-sup q[1],q[2];assert-sup q[0],q[1],q[2];";

// Feature test
//const char* code = "qreg q[3];creg c[2];barrier;h q[0];measure q[0] -> c[0];measure q[1] -> c[1];";

void printState(DDSimulationState* state) {
    std::vector<std::string> lines;
    std::string token;
    std::istringstream tokenStream(code);
    while (std::getline(tokenStream, token, ';')) {
        lines.push_back(token);
    }
    lines.emplace_back("END");

    for(size_t i = 0; i < lines.size(); i++) {
        if(i == state->interface.getCurrentLine(&state->interface)) {
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
        state->interface.getAmplitudeBitstring(&state->interface, bitString, &c);
        printf("%s %f\t||\t", bitString, c.real);
    }
    printf("\n");
    if(state->assertionFailed) {
        printf("THE PREVIOUS LINE FAILED AN ASSERTION\n");
    }
}

void clearScreen() {
    // Clear the screen using an ANSI escape sequence
    std::cout << "\033[2J\033[1;1H";
}

int main() {
    char* command = static_cast<char*>(malloc(sizeof(char) * 100));
    DDSimulationState state;
    createDDSimulationState(&state);
    state.interface.loadCode(&state.interface, code);
    bool wasError = false;
    bool wasGet = false;

    while(strcmp(command, "exit") != 0) {
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
            if(state.interface.getClassicalVariable(&state.interface, command + 4, &v) == ERROR) {
                printf("Variable %s not found\n", command + 4);
            } else {
                if (v.type == VAR_BOOL) {
                    printf("%s = %s\n", command + 4, v.value.bool_value ? "true" : "false");
                } else if (v.type == VAR_INT) {
                    printf("%s = %d\n", command + 4, v.value.int_value);
                } else if (v.type == VAR_FLOAT) {
                    printf("%s = %f\n", command + 4, v.value.float_value);
                }
            }
            wasGet = false;
        }
        printState(&state);
        printf("Enter command: ");
        std::cin.getline(command, 100);
        if(strcmp(command, "run") == 0) {
            state.interface.runSimulation(&state.interface);
        } else if(strcmp(command, "step") == 0) {
            state.interface.stepForward(&state.interface);
        } else if(strcmp(command, "back") == 0) {
            state.interface.stepBackward(&state.interface);
        } else if(strcmp(command, "reset") == 0) {
            state.interface.resetSimulation(&state.interface);
        } else if(strcmp(command, "fix") == 0) {

        } else if(strlen(command) >= 5 && strncmp(command, "get ", 4) == 0) {
            wasGet = true;
        } else {
            wasError = true;
        }
    }
    destroyDDSimulationState(&state);

    return 0;
}