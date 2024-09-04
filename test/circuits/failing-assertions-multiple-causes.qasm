qreg q[5];                                      // 0

cx q[0], q[1];                                  // 1
cx q[0], q[2];                                  // 2

assert-ent q[0], q[1], q[2], q[3], q[4];        // 3 (fails)
