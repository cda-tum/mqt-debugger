qreg q[3];

h q[0];
cx q[0], q[1];
cx q[0], q[2];

assert-ent q[0], q[1], q[2];
assert-sup q[0];
assert-sup q[1];
assert-sup q[0], q[1];
