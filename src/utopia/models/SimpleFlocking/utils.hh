#ifndef UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH
#define UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH

#include <cmath>
#include <random>

#include <utopia/core/types.hh>


namespace Utopia::Models::SimpleFlocking {


constexpr double TAU = 2*M_PI;

/// Regularizes agent orientation to interval [0, 2π]
template<class T>
T fmod_orientation (T orientation) {  // TODO find better name
    return std::fmod(orientation, TAU);
}

template<class RNG, class T=double>
T random_orientation (const std::shared_ptr<RNG>& rng) {
    return std::uniform_real_distribution<T>(0., TAU)(*rng);
}





} // namespace Utopia::Models::SimpleFlocking

#endif // UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH
