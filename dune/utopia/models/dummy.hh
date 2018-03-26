#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

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
    Utopia::DataIO::Config _config;
    std::mt19937 _rng;
    DataIO::HDFFile _file;

public:
    /// Construct the dummy model with an initial state
    /** \param state Initial state of the model
     */
    DummyModel (const Data& state, Utopia::DataIO::Config config):
        Base(),
        _state(state),
        _bc(_state.size(), 1.0),
        _config(config),
        _rng(_config["seed"].as<int>()),
        _file(_config["output_path"].as<std::string>(), "w")
    { }

    /// Iterate by one time step
    void perform_step ()
    {
        auto gen = std::bind(std::uniform_real_distribution<>(), _rng);
        std::generate(_bc.begin(), _bc.end(), gen);
        std::transform(_state.begin(), _state.end(), _bc.begin(),
            _state.begin(),
            [](const auto a, const auto b) { return a + b; }
        );
    }

    /// Do nothing for now
    void write_data ()
    {
        const std::string set_name = "data-" + std::to_string(this->time);
        auto dataset = _file.get_basegroup()->open_dataset(set_name);
        dataset->write(_state.begin(), _state.end(),
            [](auto &value) { return value; });
    }

    // Set model boundary condition
    void set_boundary_condition (const BCType& bc) { _bc = bc; }

    /// Set model initial condition
    void set_initial_condition (const Data& ic) { _state = ic; }

    /// Return const reference to stored data
    const Data& data () const { return _state; }
};

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_TEST_HH