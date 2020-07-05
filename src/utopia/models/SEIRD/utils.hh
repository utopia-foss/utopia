#ifndef UTOPIA_MODELS_SEIRD_UTILS_HH
#define UTOPIA_MODELS_SEIRD_UTILS_HH

#include <array>
#include <string>

#include <utopia/core/types.hh>


namespace Utopia::Models::SEIRD {

/// A struct holding counters for state transitions and other global counters
/** This struct is meant to count certain events over the time of a simulation
  * run. The individual counters can be accessed via the reference-returning
  * methods to individual entries of the underlying array.
  *
  * The counters implemented here should be understood as *cumulative*. Thus,
  * only the `++` operation should be invoked on them.
  */
template<class Counter>
struct Counters {
    /// Number of counters
    /** \NOTE When adjusting this, make sure to adjust the _labels as well!
      */
    static constexpr std::size_t size = 11;

private:
    /// The array holding the counter values
    std::array<Counter, size> _counts {};

    /// The array holding the corresponding counter labels
    inline static const std::array<std::string, size> _labels {
        "empty_to_susceptible",
        "living_to_empty",
        "susceptible_to_exposed_local",
        "susceptible_to_exposed_random",
        "susceptible_to_exposed_controlled",
        "exposed_to_infected",
        "infected_to_recovered",
        "infected_to_deceased",
        "recovered_to_susceptible",
        "move_randomly",
        "move_away_from_infected"
    };

public:
    /// Counstruct a Counters object with all counts set to zero
    Counters () {
        _counts.fill(0);
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

    /// Counts transitions from empty to susceptible
    Counter& empty_to_susceptible () {
        return _counts[0];
    }

    /// Counts transitions from living to empty
    Counter& living_to_empty () {
        return _counts[1];
    }

    /// Counts transitions from susceptible to exposed via local interaction
    Counter& susceptible_to_exposed_local () {
        return _counts[2];
    }

    /// Counts transitions from susceptible to exposed via random infections
    Counter& susceptible_to_exposed_random () {
        return _counts[3];
    }

    /// Counts transitions from susceptible to exposed via Exposure Control
    Counter& susceptible_to_exposed_controlled () {
        return _counts[4];
    }

    /// Counts transitions from exposed to infected
    Counter& exposed_to_infected () {
        return _counts[5];
    }

    /// Counts transitions from infected to recovered
    Counter& infected_to_recovered () {
        return _counts[6];
    }

    /// Counts transitions from infected to deceased
    Counter& infected_to_deceased () {
        return _counts[7];
    }

    /// Counts transitions from recovered to susceptible
    Counter& recovered_to_susceptible () {
        return _counts[8];
    }

    /// Counts random movement events
    Counter& move_randomly () {
        return _counts[9];
    }

    /// Counts events where an agent moves away from an infected agent
    Counter& move_away_from_infected () {
        return _counts[10];
    }
};


}  // namespace Utopia::Models::SEIRD

#endif  // UTOPIA_MODELS_SEIRD_UTILS_HH
