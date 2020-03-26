#include <armadillo>

// Check Armadillo features
/**
 *  - v9.400: `.as_row()` and `.as_col()`
 *  - v9.600: `.clean(threshold)`
 */
int main ()
{
  arma::mat X = arma::randu<arma::mat>(4, 5);

  // 9.400 features
  arma::vec V = X.as_col();
  arma::vec U = X.as_row();

  // 9.600 features
  U.clean(1e-16);

  return 0;
}
