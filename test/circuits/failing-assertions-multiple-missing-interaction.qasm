qreg q[5];                                            // 0

h q[0];                                               // 1

assert-ent q[0], q[1], q[2], q[3], q[4];              // 2 (fails)
