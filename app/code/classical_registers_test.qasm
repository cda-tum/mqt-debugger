qreg q[3];
creg c[2];
barrier;
h q[0];
measure q[0] -> c[0];
measure q[1] -> c[1];
