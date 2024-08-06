qreg q[3];                      // 0
qreg p[1];                      // 1
creg c[3];                      // 2
creg hello[1];                  // 3

x q[0];                         // 4
x q[1];                         // 5

measure q -> c;                 // 6

x q[2];                         // 7
measure q[2] -> c[2];           // 8
x q[2];                         // 9

h q[0];                         // 10
cx q[0], p[0];                  // 11

measure p[0] -> hello[0];       // 12

y q[2];                         // 13
y q[1];                         // 14

measure q[0] -> c[0];           // 15

barrier;                        // 16
