qreg q[4];                                      // 0

h q[0];                                         // 1

cx q[0], q[1];                                  // 2
cx q[1], q[2];                                  // 3
// here the control and target were swapped
cx q[3], q[0];                                  // 4

assert-ent q[0], q[2];                          // 5
assert-ent q[2], q[3];                          // 6 (fails)

z q[0];                                         // 7

h q[0];                                         // 8

assert-sup q[3];                                // 9 (fails)
assert-sup q[0], q[1], q[2], q[3];              // 10
