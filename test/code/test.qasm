qreg q[3];
creg c[3];
h q[0];
barrier;
measure q[0] -> c[0];
if (c == 0) x q[0];
assert-eq q[0] { 0, 1 };
