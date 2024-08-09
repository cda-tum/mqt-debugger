qreg q[3];
qreg ancilla[1];

gate oracle q0, q1, q2, anc {
    cx q0, anc;
    cx q2, anc;
}

h q;
x ancilla;
h ancilla;

oracle q[0], q[1], q[2], ancilla;

h q;

assert-eq 0.9, q[0], q[1], q[2], ancilla { 0, 0, 0, 0, 0, 0.707, 0, 0, 0, 0, 0, 0, 0, -0.707, 0, 0 }
