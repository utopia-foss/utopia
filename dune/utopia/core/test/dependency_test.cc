// just include dependencies
#include <yaml-cpp/yaml.h>
#include <hdf5.h>
#include <boost/serialization/list.hpp>

class Serial
{
private:
    friend class boost::serialization::access;
    int number;
};

int main ()
{
    YAML::Node config;
    return 0;
}