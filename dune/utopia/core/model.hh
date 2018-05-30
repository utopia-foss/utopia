#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdfgroup.hh>

namespace Utopia {

/// Wrapper struct for defining base class data types
/** \tparam DataType Type of the data the model operates on (state)
 *  \tparam BoundaryConditionType Data type of the boundary condition
 */
template<typename DataType,
         typename BoundaryConditionType,
         typename RNGType=std::mt19937,
         typename ConfigType=Utopia::DataIO::Config,
         typename DataGroupType=Utopia::DataIO::HDFGroup
         >
struct ModelTypes
{
    using Data = DataType;
    using BCType = BoundaryConditionType;
    using RNG = RNGType;
    using Config = ConfigType;
    using DataGroup = DataGroupType;
};

/// Base class interface for Models using the CRT Pattern
/** \tparam Derived Type of the derived model class
 *  \tparam ModelTypes Convenience wrapper for extracting model data types
 */
template<class Derived, typename ModelTypes>
class Model
{
protected:
    // -- Data types uses throughout the model class -- //
    /// Data type of the state
    using Data = typename ModelTypes::Data;
    
    /// Data type of the boundary condition
    using BCType = typename ModelTypes::BCType;
    
    /// Data type that holds the configuration
    using Config = typename ModelTypes::Config;
    
    /// Data type that is used for writing data
    using DataGroup = typename ModelTypes::DataGroup;

    /// Data type of the shared RNG
    using RNG = typename ModelTypes::RNG;

    // -- Member declarations -- //
    /// Model internal time stamp
    unsigned int time;
    
    /// Name of the model instance
    const std::string name; // TODO check if it should really be const?

    /// Config node belonging to this model instance
    Config cfg;

    /// The HDF group this model instance should write its data to
    std::shared_ptr<DataGroup> hdfgrp;

    /// The RNG shared between models
    std::shared_ptr<RNG> rng;

public:
    /// Base model constructor
    /** \detail creates an instance of model and extracts the relevant info
     *          from the passed arguments.
     *
     *  \param name          The name of this model instance, ideally used only
     *                       once on the current hierarchical level
     *  \param parent_cfg    The parent config node. This node has to contain
     *                       a key that matches the name of the instance
     *  \param parent_group  The parent HDFGroup in which a new group with
     *                       the name of the model instance is created
     *  \param shared_rng    A pointer to the RNG object shared between models
     */
    Model (const std::string name,
           Config &parent_cfg,
           std::shared_ptr<DataGroup> parent_group,
           std::shared_ptr<RNG> shared_rng):
        time(0),
        name(name),
        // extract the relevant config by instance name
        cfg(parent_cfg[this->name]),
        // create a group with the instance name
        hdfgrp(parent_group->open_group(this->name)),
        // the shared RNG can just be stored
        rng(shared_rng)
    { }

    // -- Default implementations -- //

    /// Iterate one (time) step
    /** Perform computation of one time step, increment time and write data
     */
    void iterate () {
        perform_step();
        increment_time();
        write_data();
    }

    // -- User-defined implementations -- //

    /// Return const reference to stored data
    const Data& data () const { return impl().data(); }
    /// Set model boundary condition
    void set_boundary_condition (const BCType& bc)
    {
        impl().set_boundary_condition(bc);
    }
    /// Set model initial condition
    void set_initial_condition (const Data& ic)
    {
        impl().set_initial_condition(ic);
    }
    /// Perform the computation of a step
    void perform_step () { impl().perform_step(); }
    /// Write data
    void write_data () { impl().write_data(); }

protected:
    /// cast to the derived class
    Derived& impl () { return static_cast<Derived&>(*this); }
    /// const cast to the derived interface
    const Derived& impl () const { return static_cast<const Derived&>(*this); }
    /// Increment time
    /** \param dt Time increment
     */
    void increment_time (unsigned int dt=1) { time += dt; }
};

} // namespace Utopia

#endif // UTOPIA_MODEL_HH
