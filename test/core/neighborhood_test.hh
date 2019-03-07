#ifndef UTOPIA_TEST_NEIGHBORHOOD_TEST_HH
#define UTOPIA_TEST_NEIGHBORHOOD_TEST_HH

#include <algorithm>

#include <utopia/core/logging.hh>
#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>
#include <utopia/data_io/cfg_utils.hh>


namespace Utopia {
namespace Test {

// Import some types
using Utopia::DataIO::Config;
using Utopia::UpdateMode;


/// Define data types for the cell manager test model
using NBTestModelTypes = ModelTypes<DefaultRNG, DefaultSpace>;

/// A cell state definition that is default-constructible
struct CellStateDC {
    unsigned short foobar;
};

/// Cell traits for a cell state that should use the default constructor
using CellTraitsDC = Utopia::CellTraits<CellStateDC, UpdateMode::async, true>;


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
template<class CellManager>
void check_num_neighbors (const CellManager& cm, unsigned int expected) {
    bool err = false;

    for (const auto& cell : cm.cells()) {
        auto neighbors = cm.neighbors_of(cell);
        
        if (neighbors.size() != expected) {
            std::cerr << "Cell No. " << cell->id()
                << " has " << neighbors.size()
                << " neighbors, not the expected " << expected << "!"
                << std::endl;
            err = true;
        }
    }

    if (err) {
        throw std::runtime_error("At least one cell had the wrong neighbor "
                                 "count!");
    }
}


/// Assert all members were there only once
template<class CellManager>
bool unique_neighbors (const CellManager& cm) {
    bool err = false;

    for (const auto& cell : cm.cells()) {
        auto neighbors = cm.neighbors_of(cell);
        
        // Create list of neighborhood IDs
        Utopia::IndexContainer nb_ids;
        std::transform(neighbors.begin(), neighbors.end(),
                       std::back_inserter(nb_ids),
                       [](const auto& cell){ return cell->id(); });

        // Make sure they are unique
        std::sort(nb_ids.begin(), nb_ids.end()); // needed for std::unique
        nb_ids.erase(std::unique(nb_ids.begin(), nb_ids.end()), nb_ids.end());

        if (nb_ids.size() != neighbors.size()) {
            std::cerr << "There were duplicate neighbors for cell with ID "
                      << cell->id() << "!" << std::endl;
            err = true;
        }
    }

    return (not err);
}


/// Check the expected neighbors by ID
template<class CellManager, class Cell>
bool expected_neighbors (const CellManager& cm, const Cell& cell,
                         std::vector<std::size_t> expected_ids)
{
    auto neighbors = cm.neighbors_of(cell);

    // Check the count matches
    if (neighbors.size() != expected_ids.size()) {
        std::cerr << "Expected " << expected_ids.size() << " neighbors, but "
                  << "cell " << cell->id() << " was calculated to have "
                  << neighbors.size() << " neighbors!" << std::endl;
        return false;
    }

    // Check by ID
    for (const auto& nb : neighbors) {
        if (std::find(expected_ids.begin(), expected_ids.end(),
                      nb->id()) == expected_ids.end())
        {
            std::cerr << "Neighborhood cell with ID " << nb->id() << " was "
                      << "not among the expected neighborhood cell IDs for "
                      << "cell " << cell->id() << "!" << std::endl;
            return false;
        }
    }

    // All good.
    return true;
}


#endif // UTOPIA_TEST_NEIGHBORHOOD_TEST_HH
