qreg q[3];

x q[0];

gate test1 q0, q1 {
  cx q0, q1;
}

gate test2 q0, q1 {
  cx q0, q1;
}

test1 q[2], q[1];
test1 q[1], q[2];

test2 q[0], q[2];
test2 q[1], q[2];

cx q[1], q[2];
