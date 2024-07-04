qreg q[3];
creg c[3];
h q[0];
cx q[0], q[1];
cx q[0], q[2];
assert-eq 0.9, q[0], q[1], q[2] {
    qreg q[3];
    h q[1];
    cx q[1], q[2];
    cx q[1], q[0];
};
