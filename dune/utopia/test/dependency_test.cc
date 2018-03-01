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
    hid_t some_id;
    Serial s;
    return 0;
}