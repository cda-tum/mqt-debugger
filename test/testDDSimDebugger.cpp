#include "backend/dd/DDSimDebug.hpp"
#include "frontend/cli/CliFrontEnd.hpp"

// Entanglement test
const char* const CODE = "qreg q[3];creg c[3];h q[0];cx q[0], q[1];cx q[2], "
                         "q[0];assert-ent q[0], q[1];assert-ent q[2], q[0];";

// Superposition test
// const char* const CODE = "qreg q[3];creg c[3];h q[0];cx q[0], q[1];cx q[2],"
//                         "q[0];assert-sup q[0];assert-sup q[1];assert-sup
//                         q[2];assert-sup " "q[0],q[1];assert-sup
//                         q[0],q[2];assert-sup q[1],q[2];assert-sup "
//                         "q[0],q[1],q[2];";

// Feature test
// const char* const CODE = "qreg q[3];creg c[2];barrier;h q[0];measure q[0] ->
// "
//                         "c[0];measure q[1] -> c[1];";

int main() {
  DDSimulationState state;
  createDDSimulationState(&state);

  CliFrontEnd cli;
  cli.run(CODE, &state.interface);

  destroyDDSimulationState(&state);

  return 0;
}
