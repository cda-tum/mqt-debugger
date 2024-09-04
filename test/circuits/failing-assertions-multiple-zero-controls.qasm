qreg q[4];                                      // 0

cx q[0], q[1];                                  // 1
cx q[0], q[2];                                  // 2
cx q[0], q[3];                                  // 3

assert-ent q[0], q[1], q[2], q[3];                    // 4 (fails)
