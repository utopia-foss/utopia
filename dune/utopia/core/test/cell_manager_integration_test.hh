#ifndef UTOPIA_TEST_CELL_MANAGER_INTEGRATION_TEST_HH
#define UTOPIA_TEST_CELL_MANAGER_INTEGRATION_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/cell_manager.hh>


namespace Utopia {
namespace Test {

/// Define data types for the cell manager test model
using CMTestModelTypes = ModelTypes<DefaultRNG, DefaultSpace>;


/// Specialize the cell traits struct with a dummy cell state type
using CellTraits = CellTraits<bool>;


/// Model to test function and integration of CellManager
class CMTest:
    public Model<CMTest, CMTestModelTypes>
{
public:
    /// The base model class
    using Base = Model<CMTest, CMTestModelTypes>;

    /// The Space type
    using Space = Base::Space;

private:
    // Members
    CellManager<CellTraits, CMTest> _cm;

public:
    /// Construct the test model with an initial state
    /** \param state Initial state of the model
     */
    template<class ParentModel>
    CMTest (const std::string name, const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model)
    {}

    /// Iterate by one time step
    void perform_step () {}

    /// Monitor the mean of the state
    void monitor () {}

    /// Do nothing yet
    void write_data () {}
};

} // namespace Test
} // namespace Utopia


#endif // UTOPIA_TEST_CELL_MANAGER_INTEGRATION_TEST_HH
