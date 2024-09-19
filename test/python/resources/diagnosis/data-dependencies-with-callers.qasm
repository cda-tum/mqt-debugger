qreg q[3];      // 0

gate test q {   // 1
  x q;          // 2
}               // 3

x q[0];         // 4
x q[1];         // 5

test q[0];      // 6

x q[0];         // 7
x q[2];         // 8

test q[2];      // 9
