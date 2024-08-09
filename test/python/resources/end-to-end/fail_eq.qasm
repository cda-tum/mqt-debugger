qreg q[2];

x q[0];
h q[1];

assert-eq q[0], q[1] { 0, 0.707, 0, 0.707}      // should fail
assert-eq 1.0, q[0], q[1] { 0, 0.707, 0, 0.707} // should fail
assert-eq 0.9, q[0], q[1] { 0, 0.707, 0, 0.707} // should pass

assert-eq q[1], q[0] { 0, 0.707, 0, 0.707}      // should fail
assert-eq 1.0, q[1], q[0] { 0, 0.707, 0, 0.707} // should fail
assert-eq 0.9, q[1], q[0] { 0, 0.707, 0, 0.707} // should fail

assert-eq q[1], q[0] { 0, 0, 0.707, 0.707}   // should fail
assert-eq 1.0, q[1], q[0] { 0, 0, 0.707, 0.707} // should fail
assert-eq 0.9, q[1], q[0] { 0, 0, 0.707, 0.707} // should pass
