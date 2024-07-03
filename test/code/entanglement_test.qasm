qreg q[3];
creg c[3];
h q[0];
cx q[0], q[1];
cx q[2], q[0];
assert-ent q[0], q[1];
assert-ent q[2], q[0];
