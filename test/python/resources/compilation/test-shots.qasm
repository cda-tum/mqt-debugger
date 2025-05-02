// ASSERT: (test_q0,test_q1) {0.499849,0,0,0.499849} 0.9
// ASSERT: (test_q0_,) {superposition}
// ASSERT: (test_q0__,test_q1_) {zero}
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
h q[0];
cx q[0], q[1];
barrier q[0];
creg test_q0[1];
creg test_q1[1];
measure q[0] -> test_q0[0];
measure q[1] -> test_q1[0];
// remainder of program is not important for tests
