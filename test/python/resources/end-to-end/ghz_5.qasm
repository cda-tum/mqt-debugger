qreg q[5];

h q[0];
cx q[0], q[1];
cx q[1], q[2];
cx q[2], q[3];
cx q[3], q[4];

assert-ent q[0], q[1], q[2];
assert-ent q[0], q[1], q[2], q[3], q[4];
assert-sup q[0];
assert-sup q[1];
assert-sup q[2];
assert-sup q[3];
assert-sup q[4];
