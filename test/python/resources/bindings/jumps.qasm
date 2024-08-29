qreg q[3];

gate my_cx q0, q1 {
    cx q0, q1;
}

gate my_h q0 {
    h q0;
}

gate superposition q {
    my_h q;
}

gate create_ghz q0, q1, q2 {
    superposition q0;
    assert-sup q0;
    my_cx q0, q1;
    my_cx q1, q2;
    assert-sup q0; assert-sup q1; assert-sup q2;
    assert-ent q0, q1, q2;
}

create_ghz q[0], q[1], q[2];

assert-eq q[0], q[1], q[2] {
    qreg q[3];
    h q[0];
    cx q[0], q[1];
    cx q[0], q[2];
}

assert-eq 0.9, q[0], q[1], q[2] { 0.7, 0, 0, 0, 0, 0, 0, 0.7 }
