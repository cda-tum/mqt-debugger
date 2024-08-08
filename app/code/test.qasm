qreg q[3];

gate entangle q0, q1, q2 {
  cx q0, q1;
  cx q0, q2;
  barrier q2;
}

h q[0];

entangle q[0], q[1], q[2];

h q[2];

barrier q[2];
