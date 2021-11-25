#ifndef UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH
#define UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH

#include <cmath>
#include <random>

#include <utopia/core/types.hh>


namespace Utopia::Models::SimpleFlocking {


constexpr double TAU = 2*M_PI;

/// Constrains an angle value to interval [-π, +π)
template<class T>
T constrain_angle (T angle) {  // TODO find better name
    angle = std::fmod(angle + M_PI, TAU);
    while (angle < 0.) {
        angle += TAU;
    }
    return angle - M_PI;
}

/// Returns a uniformly random angle value in [-π, +π)
template<class RNG, class T=double>
T random_angle (const std::shared_ptr<RNG>& rng) {
    return std::uniform_real_distribution<T>(-M_PI, +M_PI)(*rng);
}




} // namespace Utopia::Models::SimpleFlocking

#endif // UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH
