#ifndef UTOPIA_TEST_CELL_MANAGER_INTEGRATION_TEST_HH
#define UTOPIA_TEST_CELL_MANAGER_INTEGRATION_TEST_HH

#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>


namespace Utopia {
namespace Test {

/// Define data types for the cell manager test model
using CMTestModelTypes = ModelTypes<DefaultRNG, DefaultSpace>;

/// Dummy cell state type
struct CST {
    /// Some property
    int foo;

    /// The (required) config constructor
    CST(const DataIO::Config& cfg)
    :
        foo(get_as<int>("foo", cfg))
    {}
};

/// Specialize the cell traits struct with a dummy cell state type
using CMTestCellTraits = Utopia::CellTraits<CST, UpdateMode::sync>;


/// Model to test function and integration of CellManager
class CMTest:
    public Model<CMTest, CMTestModelTypes>
{
public:
    /// The base model class
    using Base = Model<CMTest, CMTestModelTypes>;

    /// Public cell manager (for easier testing)
    CellManager<CMTestCellTraits, CMTest> cm;

public:
    /// Construct the test model with an initial state
    /** \param state Initial state of the model
     */
    template<class ParentModel>
    CMTest (const std::string name, const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        cm(*this)
    {}

    void perform_step () {}

    void monitor () {}

    void write_data () {}
};

} // namespace Test
} // namespace Utopia


#endif // UTOPIA_TEST_CELL_MANAGER_INTEGRATION_TEST_HH
