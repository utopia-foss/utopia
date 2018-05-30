#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {

namespace Models {

/// Define data types of dummy model
using DummyTypes = ModelTypes<
    std::vector<double>,
    std::vector<double>
>;

/// Dummy model with simple update rule
/** Holds a vector of doubles and increments its entries by the boundary
 *  condition vector or 1 otherwise.
 */
class Dummy : public Model<Dummy, DummyTypes>
{
public:
    /// The base model class
    using Base = Model<Dummy, DummyTypes>;

private:
    /// The current state of the model
    Data state;

    /// The boundary conditions of the model
    BCType bc;

public:
    /// Construct the dummy model with an initial state
    /** \param initial_state Initial state of the model
     */
    Dummy (const std::string name,
           Config &cfg,
           std::shared_ptr<DataGroup> parent_group,
           std::shared_ptr<RNG> rng,
           const Data& initial_state)
    :
        // Use the base constructor for the main parts
        Base(name, cfg, parent_group, rng),
        // Initialise state and boundary condition members
        state(initial_state),
        bc(state.size(), 1.0)
    {
        // Write initial state
        this->write_data();
    }

    /// Iterate by one time step
    void perform_step ()
    {
        // Communicate which iteration step is performed
        std::cout << "  Performing step " << this->time << " ..." << std::endl;

        // Write some random numbers into the state vector
        auto gen = std::bind(std::uniform_real_distribution<>(), *rng);
        std::generate(bc.begin(), bc.end(), gen);
        std::transform(state.begin(), state.end(),
                       bc.begin(), state.begin(),
                       [](const auto a, const auto b) { return a + b; }
        );
    }

    /// Write data into a dataset that corresponds to the current step
    void write_data ()
    {
        // Generate the dataset name
        const std::string set_name = "data-" + std::to_string(this->time);

        // Open the dataset and write the state into it
        auto dataset = hdfgrp->open_dataset(set_name);
        dataset->write(state.begin(), state.end(),
            [](auto &value) { return value; });
    }

    // Set model boundary condition
    void set_boundary_condition (const BCType& new_bc) { bc = new_bc; }

    /// Set model initial condition
    void set_initial_condition (const Data& ic) { state = ic; }

    /// Return const reference to stored data
    const Data& data () const { return state; }
};

} // namespace Models

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_TEST_HH
