gate level_one q0, q1, q2 {                         // 0
    level_two q0, q1;                               // 1
    level_two q1, q2;                               // 2
}                                                   // 3

gate level_two q0, q1 {                             // 4
    cx q0, q1;                                      // 5
    level_three_a q0;                               // 6

    level_three_b q1;                               // 7
}                                                   // 8

gate level_three_a q {                              // 9
    x q;                                            // 10
}                                                   // 11

gate level_three_b q {                              // 12
    z q;                                            // 13
}                                                   // 14

qreg q[4];                                          // 15

x q[0];                                             // 16
level_one q[2], q[1], q[0];                         // 17

assert-eq q[0], q[1], q[2], q[3] { qreg x[4]; }     // 18
