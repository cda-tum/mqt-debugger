qreg q[3];

h q[2];

cx q[0], q[2];
cx q[0], q[1];

h q[0];

cx q[0], q[2];

barrier q[1];
barrier q[2];
