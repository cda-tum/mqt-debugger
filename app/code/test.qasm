qreg q[3];
creg c[2];
h q[0];
barrier;
cx q[0], q[1];
assert-ent q[0], q[1];
assert-ent q[0], q[2];
assert-ent q[1], q[2];
