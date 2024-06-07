#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mockDebug.h"
#include "debug.h"

char* lines[] = {
    "h q[0];",
    "cx q[0], q[1];",
    "cx q[2], q[0];",
    "assert-ent q[0], q[1];",
    "assert-ent q[0], q[2];",
    "end"
};

char* linesFixed[] = {
    "h q[0];",
    "cx q[0], q[1];",
    "cx q[0], q[2];",
    "assert-ent q[0], q[1];",
    "assert-ent q[0], q[2];",
    "end"
};

void printState(MockSimulationState* state) {
    char** linesToUse = (state->useCorrectAction3) ? linesFixed : lines;
    printf("Current line: %d\t\t| %s\n\n", state->interface.getCurrentLine(&state->interface) + 1, linesToUse[state->interface.getCurrentLine(&state->interface)]);
    char* bitstrings[] = {"000", "001", "010", "011", "100", "101", "110", "111"};
    Complex c;
    for(int i = 0; i < 8; i++) {
        state->interface.getAmplitudeBitstring(&state->interface, bitstrings[i], &c);
        printf("%s %f\t||\t", bitstrings[i], c.real);
    }
    printf("\n");
    if(state->assertionFailed) {
        printf("THE PREVIOUS LINE FAILED AN ASSERTION\n");
    }
}

int main(int argc, char** argv) {
    char* command = (char*) malloc(sizeof(char) * 100);
    MockSimulationState state;
    createMockSimulationState(&state);

    while(strcmp(command, "exit") != 0) {
        system("clear");
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
            state.useCorrectAction3 = 1;
        } else {
            printf("Invalid command. Choose one of:\n");
            printf("run\n");
            printf("step\n");
            printf("back\n");
            printf("reset\n");
        }
    }

    return 0;
}