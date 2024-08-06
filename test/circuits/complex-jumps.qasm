qreg q[3];                      // 0

gate my_cx q0, q1 {             // 1
    cx q0, q1;                  // 2
}                               // 3

gate make_ghz q0, q1, q2 {      // 4
    h q0;                       // 5
    entangle q0, q1;            // 6
    entangle q0, q2;            // 7
}                               // 8

gate entangle q1, q2 {          // 9
    my_cx q1, q2;               // 10
}                               // 11

  make_ghz q[0], q[1], q[2];    // 12

entangle q[2], q[0];            // 13
entangle q[1], q[0];            // 14
