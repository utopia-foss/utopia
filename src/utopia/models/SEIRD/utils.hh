#ifndef UTOPIA_MODELS_SEIRD_UTILS_HH
#define UTOPIA_MODELS_SEIRD_UTILS_HH

#include <array>
#include <string>

#include <utopia/core/types.hh>


namespace Utopia::Models::SEIRD {

/// A struct holding counters for state transitions and other global counters
template<class Counter>
struct Counters {
private:
    std::array<Counter, 12> _counts {};

    inline static const std::array<std::string, 12> _labels {
        "empty_to_susceptible",
        "living_to_empty",
        "susceptible_to_exposed_local",
        "susceptible_to_exposed_random",
        "exposed_to_infected",
        "infected_to_recovered",
        "infected_to_deceased",
        "recovered_to_susceptible",
        "deceased_to_empty",
        "infected_via_exposure_control",
        "move_randomly",
        "move_away_from_infected"
    };

public:
    /// Counstruct a Counters object with all counts set to zero
    Counters () {
        reset();
    }

    /// Return a copy of the current value of all counts
    /** \note To increment a counter, use the reference-returning *methods*.
      */
    auto counts () {
        return _counts;
    }

    /// The labels cooresponding to each entry of the counts array
    const auto& labels () {
        return _labels;
    }

    /// Reset all counts
    void reset () {
        _counts.fill(0);
    }

    Counter& empty_to_susceptible () {
        return _counts[0];
    }

    Counter& living_to_empty () {
        return _counts[1];
    }

    Counter& susceptible_to_exposed_local () {
        return _counts[2];
    }

    Counter& susceptible_to_exposed_random () {
        return _counts[3];
    }

    Counter& exposed_to_infected () {
        return _counts[4];
    }

    Counter& infected_to_recovered () {
        return _counts[5];
    }

    Counter& infected_to_deceased () {
        return _counts[6];
    }

    Counter& recovered_to_susceptible () {
        return _counts[7];
    }

    Counter& deceased_to_empty () {
        return _counts[8];
    }

    Counter& infected_via_exposure_control () {
        return _counts[9];
    }

    Counter& move_randomly () {
        return _counts[10];
    }

    Counter& move_away_from_infected () {
        return _counts[11];
    }
};


}  // namespace Utopia::Models::SEIRD

#endif  // UTOPIA_MODELS_SEIRD_UTILS_HH
