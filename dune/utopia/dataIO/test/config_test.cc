#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>

#include "config.hh"
#include "yaml-cpp/yaml.h"

/// 
int main(int argc, char **argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        config_filepath = "test_config.yaml";

        YAML::Node config = YAML::LoadFile(config_filepath);

        // create YamlFile objects with different constructors
        YamlFile a();
        a.set_filepath(config_filepath); 
        a.set_config(YAML::LoadFile(config_filepath));

        YamlFile b = a;

        YamlFile c(config_filepath);

        // assert that YamlFile members are correct and that the config values are correct
        assert_config_members_and_parameter_access(a, filepath);
        assert_config_members_and_parameter_access(b, filepath);
        assert_config_members_and_parameter_access(c, filepath);

        return 0;
    }
    catch(Dune::Exception c){
        std::cerr << c << std::endl;
        return 1;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}