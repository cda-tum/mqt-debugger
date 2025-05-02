// ASSERT: (test_q0,test_q1) {zero}
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
h q[0];
cx q[0], q[1];
creg test_q0[1];
creg test_q1[1];
cx q[0], q[1];
h q[0];
measure q[0] -> test_q0[0];
measure q[1] -> test_q1[0];
h q[0];
cx q[0], q[1];
