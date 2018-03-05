#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

namespace Utopia {

/// Base class interface for Models using the CRT Pattern
/** \tparam Derived Type of the derived model class
 */
template<class Derived, typename Data, typename BCType>
class Model
{
public:
    /// Construct model base. Set time to 0
    Model ():
        time(0)
    { }

protected:
    /// Model internal time stamp
    unsigned int time;

public:
    /// Return const reference to stored data
    const Data& data () const { return impl().data(); }
    /// Iterate one (time) step
    void iterate () { impl().iterate(); }
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

protected:
    /// cast to the derived class
    Derived& impl () { return static_cast<Derived&>(*this); }
    /// const cast to the derived interface
    const Derived& impl () const { return static_cast<const Derived&>(*this); }
};

} // namespace Utopia

#endif // UTOPIA_MODEL_HH