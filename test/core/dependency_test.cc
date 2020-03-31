// just include dependencies
#include "utopia/core/logging.hh"
#include <yaml-cpp/yaml.h>
#include <hdf5.h>
#include <boost/serialization/list.hpp>
#include <armadillo>

class Serial {
public:
    friend class boost::serialization::access;
};

void armadillo_test () {
    // define two objects
    arma::mat A = arma::randu(3, 3);
    arma::vec x = arma::randu(3);

    // multiply and print result
    auto b = A*x;
    b.print("result:");
}

int main () {
    Utopia::setup_loggers();
    YAML::Node config;
    armadillo_test();
    return 0;
}
