#ifndef UTOPIA_MODELS_DUMMY_HH
#define UTOPIA_MODELS_DUMMY_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include "../../data_io/hdfdataset.hh"
#include "../../data_io/hdfgroup.hh"
namespace Utopia
{
namespace Models
{
namespace Dummy
{
/// Define data types of dummy model
using DummyTypes = ModelTypes<std::vector<double>, std::vector<double>>;

/// Dummy model with simple update rule
/** Holds a vector of doubles and increments its entries by the boundary
 *  condition vector or 1 otherwise.
 */
class Dummy : public Model<Dummy, DummyTypes>
{
private:
    /// The base model class
    using Base = Model<Dummy, DummyTypes>;

    // Type shortcut for dataset
    using Dset = std::shared_ptr<Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>>;

    /// The current state of the model
    Data _state;

    /// The boundary conditions of the model
    BCType _bc;

    // dataset to write state to
    Dset _dataset;

public:
    auto get_dataset(){
        return _dataset;
    }

    auto get_state(){
        return _state;
    }

    auto get_bc(){
        return _bc;
    }


    /// Construct the dummy model with an initial state
    /** \param initial_state Initial state of the model
     */
    template <class ParentModel>
    Dummy(const std::string name, const ParentModel& parent_model, const Data& initial_state)
        : // Use the base constructor for the main parts
          Base(name, parent_model),
          // Initialise state and boundary condition members
          _state(initial_state),
          _bc(_state.size(), 1.0),
          _dataset(this->_hdfgrp->open_dataset("state_data"))
    {
        // set capacity such that dataset is 2d and has unlimited number of lines, 
        // but '_state.size()' many columns
        _dataset->set_capacity({H5S_UNLIMITED, _state.size()});
        // Write initial state
        this->write_data();
    }

    /// Iterate by one time step
    /** @detail This writes random numbers into the state vector, incrementing
     *          the already existing ones. Thus, with numbers between 0 and 1,
     *          the mean value of the state increases by 0.5 for each
     *          performed step.
     */
    void perform_step()
    {
        // Write some random numbers into the state vector
        auto gen = std::bind(std::uniform_real_distribution<>(), *this->_rng);
        std::generate(_bc.begin(), _bc.end(), gen);
        std::transform(_state.begin(), _state.end(), _bc.begin(), _state.begin(),
                       [](const auto a, const auto b) { return a + b; });
    }

    /// Write data into a dataset that corresponds to the current step
    void write_data()
    {
        _dataset->write(_state.begin(), _state.end(),
                       [](auto& value) { return value; });
    }

    // Set model boundary condition
    void set_boundary_condition(const BCType& new_bc)
    {
        _bc = new_bc;
    }

    /// Set model initial condition
    void set_initial_condition(const Data& ic)
    {
        _state = ic;
    }

    /// Return const reference to stored data
    const Data& data() const
    {
        return _state;
    }
};

} // namespace Dummy
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_DUMMY_HH
