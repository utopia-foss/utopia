#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <utopia/core/model.hh>

namespace Utopia {

/// Define data types for the test models
template<WriteMode data_write_mode=DefaultWriteMode>
using TestModelTypes = ModelTypes<DefaultRNG, data_write_mode>;

/// Test model with simple update rule
/** Holds a vector of doubles and increments its entries by the boundary
 *  condition vector or 1 otherwise.
 *
 *  This also tests whether inheritance from the base Model class works as
 *  desired.
 */
template<WriteMode data_write_mode = DefaultWriteMode>
class TestModel:
    public Model<TestModel<data_write_mode>, TestModelTypes<data_write_mode>>
{
public:
    /// The base model class
    using Base = Model<TestModel<data_write_mode>, TestModelTypes<data_write_mode>>;

    /// Define the data type to use
    using Data = std::vector<double>;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for the datasets
    using DataSet = typename Base::DataSet;

    /// Space type of the base model class
    using Space = typename Base::Space;

    using Self = TestModel<data_write_mode>;

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
    template<class ParentModel, class... WriterArgs>
    TestModel (
        const std::string name,
        const ParentModel &parent_model,
        const Data& initial_state,
        const DataIO::Config& custom_cfg = {},
        std::tuple<WriterArgs...> writer_args = {}, 
        Utopia::DataIO::Default::DefaultDecidermap<Self> deciders = Utopia::DataIO::Default::default_deciders<Self>,
        Utopia::DataIO::Default::DefaultTriggermap<Self> triggers = Utopia::DataIO::Default::default_triggers<Self>
    )
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model, custom_cfg, writer_args, deciders, triggers),
        // Initialize state and boundary condition members
        _state(initial_state),
        _bc(_state.size(), 1.0),
        // Initialize datatsets
        _dset_state(
            this->create_dset("state", {initial_state.size()}, true)),
        _dset_mean(
            this->create_dset("mean", {}, false))
    {
        // test the interface to _space member
        BOOST_TEST(this->_space->dim == 2);
        BOOST_TEST(this->_space->periodic == false);
        BOOST_TEST(this->_space->extent.size() == this->_space->dim);
        BOOST_TEST(this->_space->extent[0] == 1.);
        BOOST_TEST(this->_space->extent[1] == 1.);
    }

    // having these makes things a bit easier where no explicit, workable 
    // object is needed
    TestModel() = default;
    ~TestModel() = default;



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
        this->_monitor.set_entry("state_mean", compute_mean_state());
    }

    /// Do nothing yet
    void write_data () {
        _dset_state->write(_state);
        _dset_mean->write(compute_mean_state());
    }

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

    double compute_mean_state () const {
        const double sum = std::accumulate(this->_state.begin(),
                                           this->_state.end(),
                                           0);
        return sum / this->_state.size();
    }
};



/// Test model checking if 'iterate' can be overwritten
class TestModelWithIterate :
    public TestModel<Utopia::WriteMode::basic>
{
private:
    using Data = TestModel::Data;

    using Parent = TestModel<Utopia::WriteMode::basic>;

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
    /** \warning Doing this is NOT recommended! If you absolutely need to do
      *          this, be careful that you invoke the parent method such that
      *          all the required procedures take place
      */
    void iterate () {
        // Invoke the parent method
        Parent::iterate();

        // ... and additionally perform the step once more, just for testing...
        this->perform_step();
        // NOTE This is of course something useless to do, because this will
        //      not be account for when writing data. This is only done for
        //      testing anyway ...
    }
};

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_TEST_HH
