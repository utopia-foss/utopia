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

    /// Orientation (in radians)
    double orientation;

    /// The current displacement vector, updated alongside  change
    SpaceVecType<2> displacement;

public:
    /// Default constructor
    AgentState()
    :
        speed(1.e-3)
    ,   orientation(0.)
    ,   displacement({0., 0.})
    {}

    /// Constructor with config node and RNG
    template<typename RNGType>
    AgentState(const Config& cfg, const std::shared_ptr<RNGType>& rng)
    :
        speed(get_as<double>("speed", cfg, 1.e-3))
    ,   orientation(random_orientation(rng))
    {}

    // .. Getters .............................................................

    auto get_speed () const {
        return speed;
    }

    auto get_orientation () const {
        return orientation;
    }

    auto get_displacement () const {
        return displacement;
    }

    // .. Setters .............................................................

    auto set_speed (double new_speed) {
        speed = new_speed;
        update_displacement();
    }

    auto set_orientation (double new_orientation) {
        orientation = new_orientation;
        update_displacement();
    }

    // .. Helpers .............................................................

    void update_displacement () {
        displacement[0] = speed * std::sin(orientation);
        displacement[1] = speed * std::cos(orientation);
    }
};


} // namespace Utopia::Models::SimpleFlocking

#endif // UTOPIA_MODELS_SIMPLEFLOCKING_STATE_HH
