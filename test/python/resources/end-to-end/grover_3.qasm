qreg q[2];
qreg flag[1];
creg c[2];

// initialization
h q;
x flag;
barrier q;

// oracle: mark target state |11>
h flag;
ccx q[0], q[1], flag;
h flag;
barrier q;

// diffusion
h q;
x q;
h q[1];
cx q[0], q[1];
h q[1];
x q;
h q;

measure q -> c;
