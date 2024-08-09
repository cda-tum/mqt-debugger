qreg q[3];

h q[0];
cx q[0], q[1];
cx q[2], q[0];

assert-ent q[0], q[1];
assert-ent q[1], q[2];
assert-ent q[0], q[2];

assert-sup q[0], q[1], q[2];
