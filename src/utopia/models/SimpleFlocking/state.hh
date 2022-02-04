#ifndef UTOPIA_MODELS_SIMPLEFLOCKING_STATE_HH
#define UTOPIA_MODELS_SIMPLEFLOCKING_STATE_HH

#include <cmath>
#include <random>

#include <utopia/core/types.hh>

#include "utils.hh"


namespace Utopia::Models::SimpleFlocking {

/// An agent's state
class AgentState {
    /// Agent speed
    double speed;

    /// Orientation in radians, [-π, +π)
    /** Orientation zero points in positive x direction while ±π/2 points
     *  in ±y direction.
     */
    double orientation;

    /// The current displacement vector, updated upon any changes
    SpaceVecType<2> displacement;

public:
    /// Default constructor with zero-initialized members
    AgentState()
    :
        speed(0.)
    ,   orientation(0.)
    ,   displacement({0., 0.})
    {}

    /// Constructor with config node and RNG
    template<typename RNGType>
    AgentState(const Config& cfg, const std::shared_ptr<RNGType>& rng)
    :
        speed(get_as<double>("speed", cfg, 0.))
    ,   orientation(random_angle(rng))
    ,   displacement({0., 0.})
    {
        update_displacement();
    }

    // .. Getters .............................................................

    /// Returns the current speed of this agent
    auto get_speed () const {
        return speed;
    }

    /// Returns the current orientation in radians, [-π, +π)
    /** An orientation value of zero points in positive x direction while a
     *  value of ±π/2 points in ±y direction.
     */
    auto get_orientation () const {
        return orientation;
    }

    /// The current value of the displacement vector
    const auto& get_displacement () const {
        return displacement;
    }

    // .. Setters .............................................................

    /// Sets the speed and subsequently updates the displacement vector
    auto set_speed (double new_speed) {
        speed = new_speed;
        update_displacement();
    }

    /// Sets the orientation and subsequently updates the displacement vector
    /** This also makes sure the new orientation is within a valid range
      */
    auto set_orientation (double new_orientation) {
        orientation = constrain_angle(new_orientation);
        update_displacement();
    }

protected:
    // .. Helpers .............................................................

    /// Updates the displacement vector using current speed and orientation
    void update_displacement () {
        displacement[0] = speed * std::cos(orientation);
        displacement[1] = speed * std::sin(orientation);
    }
};


} // namespace Utopia::Models::SimpleFlocking

#endif // UTOPIA_MODELS_SIMPLEFLOCKING_STATE_HH
