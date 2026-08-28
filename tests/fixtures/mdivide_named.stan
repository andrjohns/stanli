// The named spellings of the solves, all six, at both dividend shapes.
// Every solve gets its own divisor and its own dividend, so each adjoint
// here is one contribution rather than a sum whose order the reference
// tape need not share. The general and triangular divisors get a dominant
// diagonal to stay invertible; the _spd ones are symmetric positive
// definite by construction.
data {
  matrix[3, 2] dm;
  matrix[3, 3] ds;
  matrix[2, 3] dr;
  matrix[3, 3] dt;
}
parameters {
  matrix[3, 3] da0;
  matrix[3, 3] da1;
  matrix[3, 3] da2;
  matrix[3, 3] da3;
  matrix[3, 2] ma0;
  vector[3] va1;
  matrix[3, 2] ma2;
  row_vector[3] ra3;
  matrix[3, 3] dw0;
  matrix[3, 3] dw1;
  matrix[3, 3] dw2;
  matrix[3, 3] dw3;
  matrix[3, 2] mw0;
  vector[3] vw1;
  matrix[3, 2] mw2;
  row_vector[3] rw3;
  matrix[3, 3] dc0;
  matrix[3, 3] dc1;
  matrix[3, 3] dc2;
  matrix[3, 3] dc3;
  matrix[3, 2] mc0;
  vector[3] vc1;
  matrix[3, 2] mc2;
  row_vector[3] rc3;
}
model {
  matrix[3, 3] xa0 = da0 + diag_matrix(rep_vector(4.0, 3));
  matrix[3, 3] xa1 = da1 + diag_matrix(rep_vector(4.0, 3));
  matrix[3, 3] xa2 = da2 + diag_matrix(rep_vector(4.0, 3));
  matrix[3, 3] xa3 = da3 + diag_matrix(rep_vector(4.0, 3));
  target += 1.0 * mdivide_left(xa0, ma0)[1, 1]
            + -0.7 * mdivide_left(xa1, va1)[2]
            + 1.3 * mdivide_right(ma2', xa2)[2, 3]
            + -0.9 * mdivide_right(ra3, xa3)[1];

  matrix[3, 3] xw0 = dw0 + dw0' + diag_matrix(rep_vector(6.0, 3));
  matrix[3, 3] xw1 = dw1 + dw1' + diag_matrix(rep_vector(6.0, 3));
  matrix[3, 3] xw2 = dw2 + dw2' + diag_matrix(rep_vector(6.0, 3));
  matrix[3, 3] xw3 = dw3 + dw3' + diag_matrix(rep_vector(6.0, 3));
  target += 1.1 * mdivide_left_spd(xw0, mw0)[1, 1]
            + 0.6 * mdivide_left_spd(xw1, vw1)[2]
            + -1.7 * mdivide_right_spd(mw2', xw2)[2, 3]
            + 0.8 * mdivide_right_spd(rw3, xw3)[1];
  // Both mixed scalar-type directions. These must select stan-math's vd/dv
  // overloads rather than promoting the data operand into the vv overload.
  target += 0.17 * mdivide_left_spd(xw0, dm)[2, 1];
  target += -0.23 * mdivide_left_spd(ds, mw0)[3, 2];
  target += 0.31 * mdivide_right_spd(dr, xw2)[1, 2];
  target += -0.37 * mdivide_right_spd(mw2', ds)[2, 2];

  matrix[3, 3] xc0 = dc0 + diag_matrix(rep_vector(4.0, 3));
  matrix[3, 3] xc1 = dc1 + diag_matrix(rep_vector(4.0, 3));
  matrix[3, 3] xc2 = dc2 + diag_matrix(rep_vector(4.0, 3));
  matrix[3, 3] xc3 = dc3 + diag_matrix(rep_vector(4.0, 3));
  target += 0.5 * mdivide_left_tri_low(xc0, mc0)[1, 1]
            + -1.2 * mdivide_left_tri_low(xc1, vc1)[2]
            + 0.3 * mdivide_right_tri_low(mc2', xc2)[2, 3]
            + 1.4 * mdivide_right_tri_low(rc3, xc3)[1];
  target += 0.41 * mdivide_left_tri_low(xc0, dm)[2, 1];
  target += -0.43 * mdivide_left_tri_low(dt, mc0)[3, 2];
  target += 0.47 * mdivide_right_tri_low(dr, xc2)[1, 2];
  target += -0.53 * mdivide_right_tri_low(mc2', dt)[2, 2];

}
