OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
h q[0];
cx q[0], q[1];
assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }
