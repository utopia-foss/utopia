#ifndef UTOPIA_MODELS_SEIRD_STATE_HH
#define UTOPIA_MODELS_SEIRD_STATE_HH

#include <algorithm>

#include <utopia/core/types.hh>


namespace Utopia::Models::SEIRD
{

// -- Kind enum (and related constants) ---------------------------------------

/// The kind of the cell
enum class Kind : char
{
    /// Unoccupied
    empty = 0,
    /// Cell represents a susceptible
    susceptible = 1,
    /// Cell is exposed to the dease but not yet infected
    exposed = 2,
    /// Cell is infected
    infected = 3,
    /// Cell is recovered
    recovered = 4,
    /// Cell is deceased
    deceased = 5,
    /// Cell is an infection source: constantly infected, spreading infection
    source = 6,
    /// Cell does not partake in the dynamics
    inert = 7,

    /// The number of kinds (COUNT)
    /** \attention The COUNT should _always_ be the last member of the enum
     *  class to ensure that it reflects the correct number of kinds.
     */
    COUNT = 8,
};

/// Map the Kind name given as a string to the actual Kind
const std::map<const std::string, Kind> kind_from_string {
    {"empty", Kind::empty},
    {"susceptible", Kind::susceptible},
    {"exposed", Kind::exposed},
    {"infected", Kind::infected},
    {"recovered", Kind::recovered},
    {"deceased", Kind::deceased},
    {"source", Kind::source},
    {"inert", Kind::inert}
};

/// The inverse of the `kind_from_string` mapping
/** This can be used to retrieve a string corresponding to a certain Kind value
  */
const std::map<const Kind, std::string> string_from_kind =
    [](){
        auto sfk = std::map<const Kind, std::string>{};

        for (auto i = 0u; i < static_cast<char>(Kind::COUNT); i++) {
            const Kind kind = static_cast<Kind>(i);
            const auto item =
                std::find_if(kind_from_string.begin(), kind_from_string.end(),
                             [&](const auto& kv){ return kv.second == kind; });
            sfk[kind] = item->first;
        }

        return sfk;
    }();

/// The associated string names of each Kind enum entry
/** The indices of this array correspond to the char value used when writing
  * out data. It thus is a mapping from char to Kind names, which is a mapping
  * that is useful to have on frontend side.
  */
const std::array<std::string, static_cast<char>(Kind::COUNT)> kind_names =
    [](){
        auto names = std::array<std::string, static_cast<char>(Kind::COUNT)>{};
        for (auto i = 0u; i < static_cast<char>(Kind::COUNT); i++) {
            names[i] = string_from_kind.at(static_cast<Kind>(i));
        }
        return names;
    }();


// -- State -------------------------------------------------------------------

/// The full cell struct for the SEIRD model
struct State
{
    /// The cell state
    Kind kind;

    /// Whether the agent is immune
    bool immune;

    /// The probability to transmit the infection to others if exposed or
    /// infected
    double p_transmit;

    /// The time passed since first being exposed
    unsigned exposed_time;

    /// The age of the cell
    unsigned age;

    /// The number of recoveries
    unsigned num_recoveries;

    /// An ID denoting to which cluster this cell belongs
    unsigned int cluster_id;

    /// Construct the cell state from a configuration and an RNG
    template<class RNG>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        kind(Kind::empty),
        immune(false),
        p_transmit(1),
        exposed_time(0),
        age(0),
        num_recoveries(0),
        cluster_id(0)
    {
        // Check if p_susceptible is available to set up cell state
        if (cfg["p_susceptible"]) {
            const auto init_density = get_as<double>("p_susceptible", cfg);

            if (init_density < 0. or init_density > 1.) {
                throw std::invalid_argument("p_susceptible needs to be in "
                                            "interval [0., 1.], but was " +
                                            std::to_string(init_density) + "!");
            }

            // With this probability, the cell state is a susceptible
            if (std::uniform_real_distribution<double>(0., 1.)(*rng) <
                init_density) {
                kind = Kind::susceptible;
            }

            // If cells are susceptible check whether they are immune
            // Check if p_immune is available to set up cell state
            if (cfg["p_immune"]) {
                const auto init_density = get_as<double>("p_immune", cfg);

                if (init_density < 0. or init_density > 1.) {
                    throw std::invalid_argument("p_immune needs to be in "
                                                "interval [0., 1.], but was " +
                                                std::to_string(init_density) +
                                                "!");
                }

                // With this probability, the cell state is immune
                if (std::uniform_real_distribution<double>(0., 1.)(*rng) <
                    init_density) {
                    immune = true;
                }
                else {
                    immune = false;
                }
            }
        }
        // Check if p_transmit is available to set up cell state
        if (cfg["p_transmit"]) {
            p_transmit = initialize_p_transmit(cfg["p_transmit"], rng);
        }
    }

    /// Initialize p_transmit from a configuration node.
    template<typename RNG>
    static double initialize_p_transmit(const DataIO::Config& cfg,
                                        const std::shared_ptr<RNG>& rng)
    {
        const auto mode = get_as<std::string>("mode", cfg);

        if (mode == "value") {
            // Return the default value
            return get_as<double>("default",
                                  get_as<DataIO::Config>("value", cfg));
        }
        else if (mode == "uniform") {
            const auto range = get_as<std::pair<double, double>>(
                "range",
                get_as<DataIO::Config>("uniform", cfg));

            // Create a uniform real distribution from the specified range
            std::uniform_real_distribution<double> distr(range.first,
                                                         range.second);

            // Draw a random number from the range and return it
            return distr(*rng);
        }
        else {
            throw std::invalid_argument(fmt::format(
                "Invalid mode! Need be either 'value' or 'uniform', was '{}'!",
                mode)
            );
        }
    };
};

}  // namespace Utopia::Models::SEIRD

#endif  // UTOPIA_MODELS_SEIRD_STATE_HH
