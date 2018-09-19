#ifndef UTOPIA_MODELS_DUMMY_HH
#define UTOPIA_MODELS_DUMMY_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>


namespace Utopia {
namespace Models {
namespace Dummy {

/// Define data types of dummy model
using DummyTypes = ModelTypes<>;

/// Dummy model with simple update rule
/** Holds a vector of doubles and increments its entries by random numbers
 *  with the bounds determined by the boundary condition vector.
 */
class Dummy : public Model<Dummy, DummyTypes>
{
public:
    /// The base model class
    using Base = Model<Dummy, DummyTypes>;

    /// The data type to use for _state and _bc members
    using Data = std::vector<double>;

    // Type shortcut for dataset
    using DataSet = Base::DataSet;

private:
    /// The current state of the model
    Data _state;

    /// The boundary conditions of the model
    Data _bc;

    /// Dataset to write the state to
    std::shared_ptr<DataSet> _dset_state;

public:
    /// Construct the dummy model with an initial state
    /** \param name          Name of this model instance
     *  \param parent_model  The parent model instance this instance appears
     *                       under
     *  \param initial_state Initial state of the model
     */
    template <class ParentModel>
    Dummy(const std::string name,
          const ParentModel& parent_model,
          const Data& initial_state)
        :
          // Use the base constructor for the main parts
          Base(name, parent_model),
          // Initialise state and boundary condition members
          _state(initial_state),
          _bc(_state.size(), 1.0),
          _dset_state(this->_hdfgrp->open_dataset("state"))
    {
        // Set capacity to a 2D dataset with as many rows as there are steps
        // and as many columns as the state is large.
        _dset_state->set_capacity({this->get_time_max() + 1, _state.size()});
        
        // Now write the initial state
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
        std::transform(_state.begin(), _state.end(),
                       _bc.begin(), _state.begin(),
                       [](const auto a, const auto b) { return a + b; });
    }


    /// Write data into a dataset that corresponds to the current step
    void write_data()
    {
        _dset_state->write(_state.begin(), _state.end(),
                           [](auto& value) { return value; });
    }


    // -- Getters and Setters -- //

};

} // namespace Dummy
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_DUMMY_HH
