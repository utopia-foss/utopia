#ifndef UTOPIA_TEST_NEIGHBORHOOD_TEST_HH
#define UTOPIA_TEST_NEIGHBORHOOD_TEST_HH

#include <dune/utopia/core/cell_manager.hh>
#include <dune/utopia/core/grid_new.hh>  // final name: grid.hh
#include <dune/utopia/core/logging.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/data_io/cfg_utils.hh>


namespace Utopia {
namespace Test {

// Import some types
using Utopia::DataIO::Config;


/// Define data types for the cell manager test model
using NBTestModelTypes = ModelTypes<DefaultRNG, DefaultSpace>;

/// A cell state definition that is default-constructible
struct CellStateDC {
    unsigned short foobar;
};

/// Cell traits for a default-constructible cell state
using CellTraitsDC = Utopia::CellTraits<CellStateDC>;


/// Model to test neighborhoods
class NBTest:
    public Model<NBTest, NBTestModelTypes>
{
public:
    /// The base model class
    using Base = Model<NBTest, NBTestModelTypes>;

    // Public cell manager (for more convenient testing)
    CellManager<CellTraitsDC, NBTest> cm;

public:
    /// Construct the test model with an initial state
    /** \param state Initial state of the model
     */
    template<class ParentModel>
    NBTest (const std::string name, const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        cm(*this)
    {}

    void perform_step () {}

    void monitor () {}

    void write_data () {} // TODO write out some data
};

} // namespace Test
} // namespace Utopia


// Testing functions ---------------------------------------------------------

/// Assure that a periodic grid has the correct Neighbor count
template<typename CellManager>
void check_num_neighbors (const CellManager& cm, unsigned int expected) {
    bool exception = false;

    for (auto& cell : cm.cells()) {
        auto neighbors = cm.neighbors_of(cell);
        
        if (neighbors.size() != expected) {
            std::cerr << "Cell No. " << cell->id()
                << " has " << neighbors.size()
                << " neighbors! Expected " << expected << std::endl;
            exception = true;
        }
    }

    if (exception) {
        std::runtime_error("At least one cell had the wrong neighbor count!");
    }
}





#endif // UTOPIA_TEST_NEIGHBORHOOD_TEST_HH
