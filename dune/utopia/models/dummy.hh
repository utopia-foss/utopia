#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

namespace Utopia {

/// Define data types of dummy model
using DummyModelTypes = ModelTypes<
    std::vector<double>,
    std::vector<double>
>;

/// Dummy model with simple update rule
/** Holds a vector of doubles and increments its entries by the boundary
 *  condition vector or 1 otherwise.
 */
class DummyModel:
    public Model<DummyModel, DummyModelTypes>
{
public:
    // convenience type definitions
    using Base = Model<DummyModel, DummyModelTypes>;
    using Data = typename Base::Data;
    using BCType = typename Base::BCType;

private:
    Data _state;
    BCType _bc;

public:
    /// Construct the dummy model with an initial state
    /** \param state Initial state of the model
     */
    DummyModel (const Data& state):
        Base(),
        _state(state),
        _bc(_state.size(), 1.0)
    { }

    /// Iterate by one time step
    void perform_step ()
    {
        std::transform(_state.begin(), _state.end(), _bc.begin(),
            _state.begin(),
            [](const auto a, const auto b) { return a + b; }
        );
    }

    /// Do nothing for now
    void write_data () {}

    // Set model boundary condition
    void set_boundary_condition (const BCType& bc) { _bc = bc; }

    /// Set model initial condition
    void set_initial_condition (const Data& ic) { _state = ic; }

    /// Return const reference to stored data
    const Data& data () const { return _state; }
};

/// Dummy model checking if 'iterate' can be overridden
class DummyModelWithIterate :
    public DummyModel
{
private:
    using Data = DummyModel::Data;
public:
    /// Create DummyModel with initial state
    DummyModelWithIterate (const Data& state):
        DummyModel(state)
    { }

    /// Iterate twice for checking this implementation
    void iterate () {
        this->perform_step();
        this->perform_step();
    }
};


} // namespace Utopia

/// Compare two containers
template<typename A, typename B>
bool compare_containers (const A& a, const B& b)
{
    if (a.size() != b.size())
        return false;
    
    std::vector<bool> res(a.size());
    std::transform(a.begin(), a.end(), b.begin(),
        res.begin(),
        [](const auto x, const auto y) { return x == y; }
    );

    return std::all_of(res.begin(), res.end(),
        [](const auto x){ return x; }
    );
}

#endif // UTOPIA_TEST_MODEL_TEST_HH