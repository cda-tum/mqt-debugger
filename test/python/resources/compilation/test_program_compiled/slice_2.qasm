// ASSERT: (test_q0) {superposition}
creg test_q0[1];
qreg q[2];
h q[0];
cx q[0], q[1];
measure q[0] -> test_q0[0];
