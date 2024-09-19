gate gate1 q0, q1 {             // 0
    x q0;                       // 1
    x q1;                       // 2
    assert-ent q0, q1;          // 3
}                               // 4

qreg q[2];                      // 5

cx q[0], q[1];                  // 6
gate1 q[0], q[1];               // 7
