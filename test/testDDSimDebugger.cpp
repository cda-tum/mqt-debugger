#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ddsimDebug.hpp"

const char* code = "qreg q[3];creg c[3];h q[0];cx q[0], q[1];cx q[2], q[0];assert-ent q[0], q[1];assert-ent q[2], q[0];";

void printState(DDSimulationState* state) {
    std::vector<std::string> lines;
    std::string token;
    std::istringstream tokenStream(code);
    while (std::getline(tokenStream, token, ';')) {
        lines.push_back(token);
    }
    lines.emplace_back("END");

    printf("Current line: %lu\t\t| %s\n\n", state->interface.getCurrentLine(&state->interface) + 1, lines[state->interface.getCurrentLine(&state->interface)].c_str());
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

int main() {
    char* command = static_cast<char*>(malloc(sizeof(char) * 100));
    DDSimulationState state;
    createDDSimulationState(&state);
    state.interface.loadCode(&state.interface, code);
    bool wasError = false;

    while(strcmp(command, "exit") != 0) {
        system("clear");
        if(wasError) {
            printf("Invalid command. Choose one of:\n");
            printf("run\n");
            printf("step\n");
            printf("back\n");
            printf("fix\n");
            printf("reset\n");
            printf("exit\n");
            wasError = false;
        }
        printState(&state);
        printf("Enter command: ");
        scanf("%s", command);
        if(strcmp(command, "run") == 0) {
            state.interface.runSimulation(&state.interface);
        } else if(strcmp(command, "step") == 0) {
            state.interface.stepForward(&state.interface);
        } else if(strcmp(command, "back") == 0) {
            state.interface.stepBackward(&state.interface);
        } else if(strcmp(command, "reset") == 0) {
            state.interface.resetSimulation(&state.interface);
        } else if(strcmp(command, "fix") == 0) {

        } else {
            wasError = true;
        }
    }
    destroyDDSimulationState(&state);

    return 0;
}