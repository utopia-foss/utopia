#include <dune/utopia/utopia.hh>
#include <dune/utopia/data_io/config.hh>
#include <dune/common/exceptions.hh>

#include "config_test.hh"
#include "yaml-cpp/yaml.h"

/// Simple test of the Config class
int main(int argc, char **argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        using Config = Utopia::DataIO::Config;

        std::string config_filepath = "config_test.yaml";
        Config config(config_filepath);

        assert_config_members_and_parameter_access(config, config_filepath);

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