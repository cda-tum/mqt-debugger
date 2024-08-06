qreg q[5];                          // 0

h q[0];                             // 1
h q[3];                             // 2

cx q[0], q[1];                      // 3
cx q[1], q[2];                      // 4
cx q[3], q[4];                      // 5

assert-ent q[0], q[2];              // 6
assert-ent q[0], q[3];              // 7 (fails)
