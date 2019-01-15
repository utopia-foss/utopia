#include <cassert>
#include <iostream>

#include <dune/utopia/core/cell_manager.hh>
#include <dune/utopia/data_io/cfg_utils.hh>


// A cell state definition
struct CellState {
    double a_double;
    std::string a_string;
    bool a_bool;
};

// A cell traits definition, i.e.: bundling the cell traits together.
// The second and third template parameters for sync and tags, respectively,
// are optional.
using CellTraits = Utopia::CellTraits<CellState>;

using Utopia::DataIO::Config;
using Utopia::CellManager;


/// A mock model class to hold the cell manager
class MockModel {
public:
    using Space = Utopia::DefaultSpace;

    const Config _cfg;
    const Space _space;

    CellManager<CellTraits, MockModel> _cm;


    MockModel(const Config& cfg)
    :
        _cfg(cfg),
        _space(cfg["space"]),
        _cm(*this)
    {}


    /// Return the space this model resides in
    std::shared_ptr<Space> get_space() const {
        return std::make_shared<Space>(_space);
    }

    /// Return the config node of this model
    Config get_cfg() const {
        return _cfg;
    }
};




int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc,argv);

        // Get a config node from file
        const YAML::Node cfg = YAML::LoadFile("cell_manager_test.yml");

        // Initialize the mock model with it
        MockModel mm(cfg);

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
