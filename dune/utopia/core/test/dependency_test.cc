// just include dependencies
#include <yaml-cpp/yaml.h>
#include <hdf5.h>
#include <boost/serialization/list.hpp>

class Serial
{
public:
    friend class boost::serialization::access;
};

int main ()
{
    YAML::Node config;
    return 0;
}