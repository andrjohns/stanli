// generated with brms 2.23.0
functions {
  /* integer sequence of values
   * Args:
   *   start: starting integer
   *   end: ending integer
   * Returns:
   *   an integer sequence from start to end
   */
  array[] int sequence(int start, int end) {
    array[end - start + 1] int seq;
    for (n in 1:num_elements(seq)) {
      seq[n] = n + start - 1;
    }
    return seq;
  }
  // are two 1D integer arrays equal?
  int is_equal(array[] int a, array[] int b) {
    int n_a = size(a);
    int n_b = size(b);
    if (n_a != n_b) {
      return 0;
    }
    for (i in 1:n_a) {
      if (a[i] != b[i]) {
        return 0;
      }
    }
    return 1;
  }

  /* grouped data stored linearly in "data" as indexed by begin and end
   * is repacked to be stacked into an array of vectors.
   */
  array[] vector stack_vectors(vector long_data, int n, array[] int stack,
                               array[] int begin, array[] int end) {
    int S = sum(stack);
    int G = size(stack);
    array[S] vector[n] stacked;
    int j = 1;
    for (i in 1:G) {
      if (stack[i] == 1) {
        stacked[j] = long_data[begin[i]:end[i]];
        j += 1;
      }
    }
    return stacked;
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming homogoneous variances
   * Args:
   *   y: response vector
   *   mu: mean parameter vector
   *   sigma: residual standard deviation
   *   chol_cor: cholesky factor of the correlation matrix
   *   nobs: number of observations in each group
   *   begin: the first observation in each group
   *   end: the last observation in each group
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_hom_lpdf(vector y, vector mu, real sigma, matrix chol_cor,
                            array[] int nobs, array[] int begin, array[] int end) {
    int I = size(nobs);
    vector[I] lp;
    matrix[rows(chol_cor), cols(chol_cor)] L = sigma * chol_cor;
    for (i in 1:I) {
      matrix[nobs[i], nobs[i]] L_i = L[1:nobs[i], 1:nobs[i]];
      lp[i] = multi_normal_cholesky_lpdf(
        y[begin[i]:end[i]] | mu[begin[i]:end[i]], L_i
      );
    }
    return sum(lp);
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming heterogenous variances
   * Deviating Args:
   *   sigma: residual standard deviation vector
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_het_lpdf(vector y, vector mu, vector sigma, matrix chol_cor,
                            array[] int nobs, array[] int begin, array[] int end) {
    int I = size(nobs);
    vector[I] lp;
    for (i in 1:I) {
      matrix[nobs[i], nobs[i]] L_i;
      L_i = diag_pre_multiply(sigma[begin[i]:end[i]], chol_cor[1:nobs[i], 1:nobs[i]]);
      lp[i] = multi_normal_cholesky_lpdf(
        y[begin[i]:end[i]] | mu[begin[i]:end[i]], L_i
      );
    }
    return sum(lp);
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming homogoneous variances
   * allows for flexible correlation matrix subsets
   * Deviating Args:
   *   Jtime: array of time indices per group
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_hom_flex_lpdf(vector y, vector mu, real sigma, matrix chol_cor,
                                 array[] int nobs, array[] int begin, array[] int end,
                                 array[,] int Jtime) {
    real lp = 0.0;
    int I = size(nobs);
    array[I] int has_lp = rep_array(0, I);
    int i = 1;
    matrix[rows(chol_cor), cols(chol_cor)] L;
    matrix[rows(chol_cor), cols(chol_cor)] Cov;
    L = sigma * chol_cor;
    Cov = multiply_lower_tri_self_transpose(L);
    while (i <= I) {
      array[nobs[i]] int iobs = Jtime[i, 1:nobs[i]];
      array[I-i+1] int lp_terms = rep_array(0, I-i+1);
      matrix[nobs[i], nobs[i]] L_i;
      if (is_equal(iobs, sequence(1, rows(L)))) {
        // all timepoints are present in this group
        L_i = L;
      } else {
        // arbitrary subsets cannot be taken on L directly
        L_i = cholesky_decompose(Cov[iobs, iobs]);
      }
      has_lp[i] = 1;
      lp_terms[1] = 1;
      // find all additional groups where we have the same timepoints
      for (j in (i+1):I) {
        if (has_lp[j] == 0 && is_equal(Jtime[j], Jtime[i]) == 1) {
          has_lp[j] = 1;
          lp_terms[j-i+1] = 1;
        }
      }
      // vectorize the log likelihood by stacking the vectors
      lp += multi_normal_cholesky_lpdf(
        stack_vectors(y, nobs[i], lp_terms, begin[i:I], end[i:I]) |
        stack_vectors(mu, nobs[i], lp_terms, begin[i:I], end[i:I]), L_i
      );
      while (i <= I && has_lp[i] == 1) {
        i += 1;
      }
    }
    return lp;
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming heterogenous variances
   * allows for flexible correlation matrix subsets
   * Deviating Args:
   *   sigma: residual standard deviation vectors
   *   Jtime: array of time indices per group
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_het_flex_lpdf(vector y, vector mu, vector sigma, matrix chol_cor,
                                 array[] int nobs, array[] int begin, array[] int end,
                                 array[,] int Jtime) {
    int I = size(nobs);
    vector[I] lp;
    array[I] int has_lp = rep_array(0, I);
    int i = 1;
    matrix[rows(chol_cor), cols(chol_cor)] Cor;
    Cor = multiply_lower_tri_self_transpose(chol_cor);
    while (i <= I) {
      array[nobs[i]] int iobs = Jtime[i, 1:nobs[i]];
      matrix[nobs[i], nobs[i]] Lcor_i;
      matrix[nobs[i], nobs[i]] L_i;
      if (is_equal(iobs, sequence(1, rows(chol_cor)))) {
        // all timepoints are present in this group
        Lcor_i = chol_cor;
      } else {
        // arbitrary subsets cannot be taken on chol_cor directly
        Lcor_i = cholesky_decompose(Cor[iobs, iobs]);
      }
      L_i = diag_pre_multiply(sigma[begin[i]:end[i]], Lcor_i);
      lp[i] = multi_normal_cholesky_lpdf(y[begin[i]:end[i]] | mu[begin[i]:end[i]], L_i);
      has_lp[i] = 1;
      // find all additional groups where we have the same timepoints
      for (j in (i+1):I) {
        if (has_lp[j] == 0 && is_equal(Jtime[j], Jtime[i]) == 1) {
          // group j may have different sigmas that group i
          L_i = diag_pre_multiply(sigma[begin[j]:end[j]], Lcor_i);
          lp[j] = multi_normal_cholesky_lpdf(y[begin[j]:end[j]] | mu[begin[j]:end[j]], L_i);
          has_lp[j] = 1;
        }
      }
      while (i <= I && has_lp[i] == 1) {
        i += 1;
      }
    }
    return sum(lp);
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming homogoneous variances
   * and known standard errors
   * Args:
   *   y: response vector
   *   mu: mean parameter vector
   *   sigma: residual standard deviation
   *   se2: square of user defined standard errors
   *   chol_cor: cholesky factor of the correlation matrix
   *   nobs: number of observations in each group
   *   begin: the first observation in each group
   *   end: the last observation in each group
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_hom_se_lpdf(vector y, vector mu, real sigma, data vector se2,
                               matrix chol_cor, array[] int nobs, array[] int begin, array[] int end) {
    int I = size(nobs);
    vector[I] lp;
    matrix[rows(chol_cor), cols(chol_cor)] Cov;
    Cov = multiply_lower_tri_self_transpose(sigma * chol_cor);
    for (i in 1:I) {
      matrix[nobs[i], nobs[i]] Cov_i = Cov[1:nobs[i], 1:nobs[i]];
      // need to add 'se' to the covariance matrix itself
      Cov_i += diag_matrix(se2[begin[i]:end[i]]);
      lp[i] = multi_normal_lpdf(y[begin[i]:end[i]] | mu[begin[i]:end[i]], Cov_i);
    }
    return sum(lp);
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming heterogenous variances
   * and known standard errors
   * Deviating Args:
   *   sigma: residual standard deviation vector
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_het_se_lpdf(vector y, vector mu, vector sigma, data vector se2,
                               matrix chol_cor, array[] int nobs, array[] int begin, array[] int end) {
    int I = size(nobs);
    vector[I] lp;
    for (i in 1:I) {
      matrix[nobs[i], nobs[i]] Cov_i;
      Cov_i = diag_pre_multiply(sigma[begin[i]:end[i]], chol_cor[1:nobs[i], 1:nobs[i]]);
      // need to add 'se' to the covariance matrix itself
      Cov_i = multiply_lower_tri_self_transpose(Cov_i);
      Cov_i += diag_matrix(se2[begin[i]:end[i]]);
      lp[i] = multi_normal_lpdf(y[begin[i]:end[i]] | mu[begin[i]:end[i]], Cov_i);
    }
    return sum(lp);
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming homogoneous variances
   * and known standard errors
   * allows for flexible correlation matrix subsets
   * Deviating Args:
   *   Jtime: array of time indices per group
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_hom_se_flex_lpdf(vector y, vector mu, real sigma, data vector se2,
                                    matrix chol_cor, array[] int nobs, array[] int begin,
                                    array[] int end, array[,] int Jtime) {
    int I = size(nobs);
    vector[I] lp;
    matrix[rows(chol_cor), cols(chol_cor)] Cov;
    Cov = multiply_lower_tri_self_transpose(sigma * chol_cor);
    for (i in 1:I) {
      array[nobs[i]] int iobs = Jtime[i, 1:nobs[i]];
      matrix[nobs[i], nobs[i]] Cov_i = Cov[iobs, iobs];
      Cov_i += diag_matrix(se2[begin[i]:end[i]]);
      lp[i] = multi_normal_lpdf(y[begin[i]:end[i]] | mu[begin[i]:end[i]], Cov_i);
    }
    return sum(lp);
  }
  /* multi-normal log-PDF for time-series covariance structures
   * in Cholesky parameterization and assuming heterogenous variances
   * and known standard errors
   * allows for flexible correlation matrix subsets
   * Deviating Args:
   *   sigma: residual standard deviation vector
   *   Jtime: array of time indices per group
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_time_het_se_flex_lpdf(vector y, vector mu, vector sigma, data vector se2,
                                    matrix chol_cor, array[] int nobs, array[] int begin,
                                    array[] int end, array[,] int Jtime) {
    int I = size(nobs);
    vector[I] lp;
    matrix[rows(chol_cor), cols(chol_cor)] Cor;
    Cor = multiply_lower_tri_self_transpose(chol_cor);
    for (i in 1:I) {
      array[nobs[i]] int iobs = Jtime[i, 1:nobs[i]];
      matrix[nobs[i], nobs[i]] Cov_i;
      Cov_i = quad_form_diag(Cor[iobs, iobs], sigma[begin[i]:end[i]]);
      Cov_i += diag_matrix(se2[begin[i]:end[i]]);
      lp[i] = multi_normal_lpdf(y[begin[i]:end[i]] | mu[begin[i]:end[i]], Cov_i);
    }
    return sum(lp);
  }
  /* compute the cholesky factor of an AR1 correlation matrix
   * Args:
   *   ar: AR1 autocorrelation
   *   nrows: number of rows of the covariance matrix
   * Returns:
   *   A nrows x nrows matrix
   */
   matrix cholesky_cor_ar1(real ar, int nrows) {
     matrix[nrows, nrows] mat;
     vector[nrows - 1] gamma;
     mat = diag_matrix(rep_vector(1, nrows));
     for (i in 2:nrows) {
       gamma[i - 1] = pow(ar, i - 1);
       for (j in 1:(i - 1)) {
         mat[i, j] = gamma[i - j];
         mat[j, i] = gamma[i - j];
       }
     }
     return cholesky_decompose(mat ./ (1 - ar^2));
   }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  // data needed for ARMA correlations
  int<lower=0> Kar;  // AR order
  int<lower=0> Kma;  // MA order
  // see the functions block for details
  int<lower=1> N_tg;
  array[N_tg] int<lower=1> begin_tg;
  array[N_tg] int<lower=1> end_tg;
  array[N_tg] int<lower=1> nobs_tg;
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc] Xc;  // centered version of X without an intercept
  vector[Kc] means_X;  // column means of X before centering
  int max_lag = max(Kar, Kma);
  int max_nobs_tg = max(nobs_tg);  // maximum dimension of the autocorrelation matrix
  for (i in 2:K) {
    means_X[i - 1] = mean(X[, i]);
    Xc[, i - 1] = X[, i] - means_X[i - 1];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  real Intercept;  // temporary intercept for centered predictors
  vector<lower=-1,upper=1>[Kar] ar;  // autoregressive coefficients
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // cholesky factor of the autocorrelation matrix
  matrix[max_nobs_tg, max_nobs_tg] Lcortime;
  // prior contributions to the log posterior
  real lprior = 0;
  // compute residual covariance matrix
  Lcortime = cholesky_cor_ar1(ar[1], max_nobs_tg);
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    target += normal_time_hom_lpdf(Y | mu, sigma, Lcortime, nobs_tg, begin_tg, end_tg);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

