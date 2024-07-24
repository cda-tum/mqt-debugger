qreg q[2];

h q[0];
cx q[0], q[1];

assert-ent q[0], q[1];
assert-sup q[0];
assert-sup q[1];
assert-sup q[0], q[1];
