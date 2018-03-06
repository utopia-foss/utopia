#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

namespace Utopia {

/// Wrapper struct for defining base class data types
/** \tparam DataType Type of the data the model operates on (state)
 *  \tparam BoundaryConditionType Data type of the boundary condition
 */
template<typename DataType, typename BoundaryConditionType>
struct ModelTypes
{
    using Data = DataType;
    using BCType = BoundaryConditionType;
};

/// Base class interface for Models using the CRT Pattern
/** \tparam Derived Type of the derived model class
 *  \tparam ModelTypes Convenience wrapper for extracting model data types
 */
template<class Derived, typename ModelTypes>
class Model
{
public:
    /// Construct model base. Set time to 0
    Model ():
        time(0)
    { }

protected:
    /// Data type of the state
    using Data = typename ModelTypes::Data;
    /// Data type of the boundary condition
    using BCType = typename ModelTypes::BCType;
    /// Model internal time stamp
    unsigned int time;

public:

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