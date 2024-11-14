#ifndef UTOPIA_MODELS_SEIRD_COUNTERS_HH
#define UTOPIA_MODELS_SEIRD_COUNTERS_HH

#include <array>
#include <string>

#include <utopia/core/types.hh>


namespace Utopia::Models::SEIRD {

/// A struct holding counters for state transitions and other global counters
/** This struct is meant to count certain events over the time of a simulation
  * run. The individual counters can be incremented via individual methods
  * which increment the values in the underlying array.
  *
  * The counters implemented here should be understood as *cumulative*, that's
  * why there is no option to reset them.
  */
template<class Counter>
struct Counters {
    /// Number of counters
    /** NOTE When adjusting this, make sure to adjust the _labels as well!
      */
    static constexpr std::size_t size = 11;

private:
    /// The array holding the counter values
    std::array<Counter, size> _counts {};

    /// The array holding the corresponding counter labels
    inline static const std::array<std::string, size> _labels {
        "empty_to_susceptible",
        "living_to_empty",
        "susceptible_to_exposed_contact",
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
    auto counts () {
        return _counts;
    }

    /// The labels cooresponding to each entry of the counts array
    const auto& labels () {
        return _labels;
    }

    /// Increment counter for transitions from empty to susceptible
    void increment_empty_to_susceptible () {
        ++_counts[0];
    }

    /// Increment counter for transitions from living to empty
    void increment_living_to_empty () {
        ++_counts[1];
    }

    /// Increment counter for transitions from susceptible to exposed (contact)
    void increment_susceptible_to_exposed_contact () {
        ++_counts[2];
    }

    /// Increment counter for transitions from susceptible to exposed (random)
    void increment_susceptible_to_exposed_random () {
        ++_counts[3];
    }

    /// Increment counter for transitions from susceptible to exposed (control)
    void increment_susceptible_to_exposed_controlled () {
        ++_counts[4];
    }

    /// Increment counter for transitions from exposed to infected
    void increment_exposed_to_infected () {
        ++_counts[5];
    }

    /// Increment counter for transitions from infected to recovered
    void increment_infected_to_recovered () {
        ++_counts[6];
    }

    /// Increment counter for transitions from infected to deceased
    void increment_infected_to_deceased () {
        ++_counts[7];
    }

    /// Increment counter for transitions from recovered to susceptible
    void increment_recovered_to_susceptible () {
        ++_counts[8];
    }

    /// Increment counter for random movement events
    void increment_move_randomly () {
        ++_counts[9];
    }

    /// Increment counter for movement events away from an infected agent
    void increment_move_away_from_infected () {
        ++_counts[10];
    }
};


}  // namespace Utopia::Models::SEIRD

#endif  // UTOPIA_MODELS_SEIRD_COUNTERS_HH
