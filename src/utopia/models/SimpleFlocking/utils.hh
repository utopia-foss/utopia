#ifndef UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH
#define UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH

#include <cmath>
#include <random>
#include <numeric>
#include <vector>
#include <limits>

#include <armadillo>
#include <spdlog/spdlog.h>

#include <utopia/core/types.hh>


namespace Utopia::Models::SimpleFlocking {

constexpr double TAU = 2*M_PI;
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();


// -- Angle-related tools -----------------------------------------------------

/// Returns a uniformly random angle value in [-π, +π)
template<class RNG, class T=double>
T random_angle (const std::shared_ptr<RNG>& rng) {
    return std::uniform_real_distribution<T>(-M_PI, +M_PI)(*rng);
}

/// Constrains an angle value to interval [-π, +π)
template<class T>
T constrain_angle (T angle) {
    angle = std::fmod(angle + M_PI, TAU);
    while (angle < 0.) {
        angle += TAU;
    }
    return angle - M_PI;
}

/// In-place constrains all angles in a container to interval [-π, +π)
template<class Container>
void constrain_angles (Container& angles) {
    std::transform(
        angles.begin(), angles.end(), angles.begin(),
        [](auto angle){ return constrain_angle(angle); }
    );
}


// -- Geometry ----------------------------------------------------------------

/// Computes the absolute group velocity from a container of velocity vectors
/** Essentially: the 2-norm of the sum of all velocity vectors, divided by the
  * number of vectors.
  * Returns NaN if the given container is empty.
  */
template<class Container>
double absolute_group_velocity (const Container& velocities) {
    if (velocities.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    using Vec = typename Container::value_type;
    Vec zero = velocities[0];
    zero.fill(0.);

    const Vec group_velocity = std::accumulate(
        velocities.begin(), velocities.end(), zero
    );
    return arma::norm(group_velocity, 2) / velocities.size();
}


// -- Circular Statistics -----------------------------------------------------

/// Compute sum of sine and cosine values from angles in a container
template<class Container>
auto _circular_sin_cos_sum (const Container& angles) {
    static_assert(
        std::is_floating_point<typename Container::value_type>::value,
        "need angles specified as floating-point types!"
    );

    const auto sin_sum = std::accumulate(
        angles.begin(), angles.end(), 0.,
        [](auto a1, auto a2){ return a1 + std::sin(a2); }
    );
    const auto cos_sum = std::accumulate(
        angles.begin(), angles.end(), 0.,
        [](auto a1, auto a2){ return a1 + std::cos(a2); }
    );

    return std::make_pair(sin_sum, cos_sum);
}


/// Computes the circular mean from a sample of (constrained) angles
/** Uses circular statistics to compute the mean.
  * Assumes angles to be in radians. While it does not matter in which
  * interval they are, the resulting mean value will be in [-π, +π).
  *
  * Returns NaN if the given container is empty.
  *
  * See scipy implementation for reference:
  *   https://github.com/scipy/scipy/blob/v1.7.1/scipy/stats/morestats.py#L3474
  */
template<class Container = std::vector<double>>
auto circular_mean (const Container& angles) {
    if (angles.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const auto&& [sin_sum, cos_sum] = _circular_sin_cos_sum(angles);
    return constrain_angle(std::atan2(sin_sum, cos_sum));
}


/// Computes the circular mean and std from a sample of (constrained) angles
/** Uses circular statistics to compute the mean and standard deviation.
  * Assumes angles to be in radians. While it does not matter in which
  * interval they are, the resulting mean value will be in [-π, +π).
  *
  * Returns NaN if the given container is empty.
  *
  * See scipy implementation for reference:
  *   https://github.com/scipy/scipy/blob/v1.7.1/scipy/stats/morestats.py#L3595
  */
template<class Container = std::vector<double>>
auto circular_mean_and_std (const Container& angles) {
    if (angles.empty()) {
        return std::make_pair(NaN, NaN);
    }

    const auto&& [sin_sum, cos_sum] = _circular_sin_cos_sum(angles);
    const auto mean = constrain_angle(std::atan2(sin_sum, cos_sum));

    const auto r = std::min(
        1., std::sqrt(sin_sum*sin_sum + cos_sum*cos_sum) / angles.size()
    );
    const auto std = std::sqrt(-2. * std::log(r));

    return std::make_pair(mean, std);
}


} // namespace Utopia::Models::SimpleFlocking

#endif // UTOPIA_MODELS_SIMPLEFLOCKING_UTILS_HH
