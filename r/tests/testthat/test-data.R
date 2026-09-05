# The data writer, which has no runtime dependency.

test_that("non-finite reals go out in the spellings Stan's reader accepts", {
  expect_equal(stanli:::json_scalar(Inf), "\"Infinity\"")
  expect_equal(stanli:::json_scalar(-Inf), "\"-Infinity\"")
  expect_equal(stanli:::json_scalar(NaN), "\"NaN\"")
  expect_equal(stanli:::json_scalar(NA_real_), "\"NaN\"")
  expect_equal(stanli:::to_json(list(lb = -Inf, ub = 3.5)),
               "{\"lb\": \"-Infinity\", \"ub\": 3.5}")
  expect_equal(stanli:::to_json(list(y = c(1.5, Inf, NA))),
               "{\"y\": [1.5, \"Infinity\", \"NaN\"]}")
  expect_equal(stanli:::to_json(list(M = matrix(c(1, Inf, 2, 3), 2, 2))),
               "{\"M\": [[1, 2], [\"Infinity\", 3]]}")
})

test_that("integer data still rejects a missing value", {
  expect_error(stanli:::json_scalar(NA_integer_), "integer data")
  expect_error(stanli:::json_scalar(NA), "integer data")
  expect_error(stanli:::to_json(list(N = NA_integer_)), "integer data")
})

test_that("a model reads data holding an infinite bound", {
  if (!stanli_available()) skip("no stanli runtime installed")
  m <- stanli_model(code = "
    data { real lb; real<upper=5> ub; }
    parameters { real mu; }
    model { mu ~ normal(0, 1); target += ub; }",
    data = list(lb = -Inf, ub = 3.5))
  expect_s3_class(m, "stanli_model")
  expect_true(is.finite(log_prob_grad(m, 0)$lp))
})
