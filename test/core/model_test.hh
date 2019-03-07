#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <dune/utopia/core/model.hh>

namespace Utopia {

/// Define data types for the test models
using TestModelTypes = ModelTypes<>;

/// Test model with simple update rule
/** Holds a vector of doubles and increments its entries by the boundary
 *  condition vector or 1 otherwise.
 *  
 *  This also tests whether inheritance from the base Model class works as
 *  desired.
 */
class TestModel:
    public Model<TestModel, TestModelTypes>
{
public:
    /// The base model class
    using Base = Model<TestModel, TestModelTypes>;

    /// Define the data type to use
    using Data = std::vector<double>;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for the datasets
    using DataSet = typename Base::DataSet;

private:
    // Declare members
    Data _state;
    Data _bc;

    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_state;
    std::shared_ptr<DataSet> _dset_mean;

public:
    /// Construct the test model with an initial state
    /** \param state Initial state of the model
     */
    template<class ParentModel>
    TestModel (const std::string name,
               const ParentModel &parent_model,
               const Data& initial_state)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        // Initialize state and boundary condition members
        _state(initial_state),
        _bc(_state.size(), 1.0),
        // Initialize datatsets
        _dset_state(
            this->create_dset("state", {initial_state.size()}, true)),
        _dset_mean(
            this->create_dset("mean", {}, false))
    {}

    /// Iterate by one time step
    void perform_step ()
    {
        std::transform(_state.begin(), _state.end(),
                       _bc.begin(), _state.begin(),
            [](const auto a, const auto b) { return a + b; }
        );
    }

    /// Monitor the mean of the state
    void monitor () {
        _monitor.set_entry("state_mean", [this](){
            const double sum = std::accumulate(this->_state.begin(),
                                               this->_state.end(),
                                               0);
            return sum / this->_state.size();
        });
    }

    /// Do nothing yet
    void write_data () {}

    // Set model boundary condition
    void set_bc (const Data& bc) { _bc = bc; }

    /// Set model initial condition
    void set_state (const Data& s) { _state = s; }

    /// Return const reference to stored data
    const Data& state () const { return _state; }

    // -- Getters -- //
    std::shared_ptr<DataSet> get_dset_state () {
        return _dset_state;
    }

    std::shared_ptr<DataSet> get_dset_mean () {
        return _dset_mean;
    }
};



/// Test model checking if 'iterate' can be overwritten
class TestModelWithIterate :
    public TestModel
{
private:
    using Data = TestModel::Data;

public:
    /// Create TestModel with initial state
    template<class ParentModel>
    TestModelWithIterate (const std::string name,
                          const ParentModel &parent,
                          const Data& initial_state)
    :
        // Initialize completely via parent class constructor
        TestModel(name, parent, initial_state)
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
