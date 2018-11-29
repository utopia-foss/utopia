// just include dependencies
#include <yaml-cpp/yaml.h>
#include <hdf5.h>
#include <boost/serialization/list.hpp>
#include <fftw3.h>

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

int main ()
{
    YAML::Node config;
    fftw_test();
    return 0;
}
