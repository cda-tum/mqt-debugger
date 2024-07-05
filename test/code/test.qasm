qreg q[3];
creg c[3];

h q[1];

gate entangle q0, q1 {
    cx q0, q1;
    cx q1, q0;
}

entangle q[1], q[0];
measure q[1] -> c[1];
measure q[0] -> c[0];
