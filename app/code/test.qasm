gate oracle q0, q1, q2, flag {
    // mark target state |111>
    assert-sup q0, q1, q2;
    mcphase(pi) q0, q1, q2, flag;
}

gate diffusion q0, q1, q2 {
    h q0; h q1; h q2;
    x q0; x q1; x q2;
    h q2;
    ccx q0, q1, q2;
    h q2;
    x q2; x q1; x q0;
    h q2; h q1; h q0;
}

qreg q[3];
qreg flag[1];
creg c[3];

// initialization
h q;
x flag;

// repetition 1
oracle q[0], q[1], q[2], flag;
diffusion q[0], q[1], q[2];
assert-eq 0.8, q[0], q[1], q[2] { 0, 0, 0, 0, 0, 0, 0, 1 }

// repetition 2
oracle q[0], q[1], q[2], flag;
diffusion q[0], q[1], q[2];
assert-eq 0.9, q[0], q[1], q[2] { 0, 0, 0, 0, 0, 0, 0, 1 }

measure q -> c;
