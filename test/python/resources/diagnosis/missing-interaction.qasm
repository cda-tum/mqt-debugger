qreg q[4];

cx q[0], q[1];
cx q[1], q[2];
assert-ent q[0], q[2]; // fails without missing interaction
assert-ent q[0], q[3]; // fails with missing interaction
