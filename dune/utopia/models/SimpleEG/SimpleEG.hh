#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {

/// Define data types of SimpleEG model
using SimpleEGModelTypes = ModelTypes<
    std::vector<double>,
    std::vector<double>
>;

/// Dummy model with simple update rule
/** Holds a vector of doubles and increments its entries by the boundary
 *  condition vector or 1 otherwise.
 */
class SimpleEGModel:
    public Model<SimpleEGModel, SimpleEGModelTypes>
{
public:
    // convenience type definitions
    using Base = Model<SimpleEGModel, SimpleEGModelTypes>;
    using Data = typename Base::Data;
    using BCType = typename Base::BCType;

private:
    const std::string _name;
    Utopia::DataIO::Config _config;
    std::shared_ptr<Utopia::DataIO::HDFGroup> _group;
    std::mt19937 _rng;

public:
    /// Construct the SimpleEG model
    /** \param name Name of this model instance
     *  \param config The corresponding config node
     *  \param group The HDFGroup to write data to
     */
    SimpleEGModel (const std::string name,
                   Utopia::DataIO::Config config,
                   std::shared_ptr<Utopia::DataIO::HDFGroup> group):
        Base(),
        _name(name),
        _config(config),
        _group(group->open_group(_name)),
        _rng(_config["seed"].as<int>())
    { }

    /// Setup the grid
    void setup_grid ()
    {
        // TODO
        // ...
    }

    /// Iterate single step
    void perform_step ()
    {
        // TODO
        // ...
    }

    /// Write data
    void write_data ()
    {
        // TODO
        // ...
    }

    // TODO Check what to do with the below methods
    /// Set model boundary condition
    // void set_boundary_condition (const BCType& bc) { _bc = bc; }

    /// Set model initial condition
    // void set_initial_condition (const Data& ic) { _state = ic; }

    /// Return const reference to stored data
    // const Data& data () const { return _state; }
};

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_TEST_HH
