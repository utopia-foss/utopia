// just include dependencies
#include <yaml-cpp/yaml.h>
#include <hdf5.h>
#include <boost/serialization/list.hpp>
#include <fftw3.h>
#include <armadillo>

class Serial
{
public:
    friend class boost::serialization::access;
};

void fftw_test ()
{
    fftw_complex *in;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 1000);
    fftw_free(in);
}

void armadillo_test ()
{
    // define two objects
    arma::mat A = arma::randu(3, 3);
    arma::vec x = arma::randu(3);

    // multiply and print result
    auto b = A*x;
    b.print("result:");
}

int main ()
{
    YAML::Node config;
    fftw_test();
    armadillo_test();
    return 0;
}
