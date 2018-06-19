// just include dependencies
#include <yaml-cpp/yaml.h>
#include <hdf5.h>
#include <boost/serialization/list.hpp>

class Serial
{
public:
    friend class boost::serialization::access;
    int number;
};

int main ()
{
    YAML::Node config;
    Serial s;
    s.number = 1;
    return 0;
}