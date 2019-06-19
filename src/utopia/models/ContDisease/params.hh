#ifndef UTOPIA_MODELS_CONTDISEASE_PARAMS_HH
#define UTOPIA_MODELS_CONTDISEASE_PARAMS_HH

#include <queue>

#include <utopia/core/types.hh>
#include <utopia/data_io/cfg_utils.hh>

namespace Utopia::Models::ContDisease
{


/// Parameters specifying the infection control
struct InfectionContParams {
    // -- Type definitions ----------------------------------------------------
    /// Type of the times queue
    using TimesQueue = std::queue<std::size_t>;

    /// The type of the change p_infection pairs
    using TimesValuesQueue = std::queue<std::pair<std::size_t, double>>;

    // -- Parameters ----------------------------------------------------
    /// Whether infection control is enabled
    const bool enabled;

    /// The number of infections added to the default p_infect
    const std::size_t num_additional_infections;

    /// Add additional infections at these time steps
    mutable TimesQueue at_times;

    /// Change p_infect to new value at given times
    /** Each element of this container provides a pair of [time, new_value].
    *   If the iteration step (time) of the simulation is reached 
    *   p_infect is set to new_value
    */
    mutable TimesValuesQueue change_p_infect;

    /// Configuration constructor
    /** Construct an InfectionContParams object with required parameters being
     *  extracted from a configuration node with the same parameter names.
     */
    InfectionContParams(const DataIO::Config& cfg)
    :
    enabled(get_as<bool>("enabled", cfg)),
    num_additional_infections{get_as<std::size_t>(
        "num_additional_infections", cfg, 0)},
    at_times{[&](){
        auto cont = get_as<std::vector<std::size_t>>("at_times", cfg, {});

        std::sort(cont.begin(), cont.end());

        // Copy elements into the queue
        TimesQueue q{};
        for (const auto& v : cont){
            q.push(v);
        }

        return q;
    }()},
    change_p_infect{[&](){
        // Check the parameter
        if (not cfg["change_p_infect"] or not cfg["change_p_infect"].size()) {
            // Key did not exist or was empty; return empty queue.
            return TimesValuesQueue{};
        }
        else if (not cfg["change_p_infect"].IsSequence()) {
            // Inform about bad type of the given configuration entry
            throw std::invalid_argument("Parameter change_p_infect need be "
                "a sequence of pairs, but was not! Given infection control "
                "parameters:\n" + DataIO::to_string(cfg));
        }

        auto cont = get_as<std::vector<std::pair<std::size_t, double>>>(
            "change_p_infect", cfg);
        
        // Sort such that times low times are in the beginning of the queue
        std::sort(cont.begin(), cont.end(),
            [](const auto& a, const auto& b){
                return a.first < b.first;
            }
        );

        // Copy elements into the queue
        TimesValuesQueue q{};
        for (const auto& v : cont){
            q.push(v);
        }

        return q;
    }()}
    {};
};


/// Parameters of the ContDisease
struct Params {
    /// Probability per site and time step to go from state empty to tree
    const double p_growth;

    /// Probability per site and time step for a tree cell to not become
    /// infected if an infected cell is in the neighborhood.
    const double p_immunity;

    /// Probability per site and time step for a random point infection of a
    /// tree cell
    mutable double p_infect;

    /// Infection control parameters
    const InfectionContParams infection_control;

    /// Construct the parameters from the given configuration node
    Params(const DataIO::Config& cfg)
    :
        p_growth(get_as<double>("p_growth", cfg)),
        p_immunity(get_as<double>("p_immunity", cfg)),
        p_infect(get_as<double>("p_infect", cfg)),
        infection_control(get_as<DataIO::Config>("infection_control", cfg))
    {
        if ((p_growth > 1) or (p_growth < 0)) {
            throw std::invalid_argument("Invalid p_growth! Need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a tree. Was: "
                + std::to_string(p_growth));
        }
        if ((p_immunity > 1) or (p_immunity < 0)) {
            throw std::invalid_argument("Invalid p_immunity! Need be in range "
                "[0, 1], was " + std::to_string(p_immunity));
        }
        if ((p_infect > 1) or (p_infect < 0)) {
            throw std::invalid_argument("Invalid p_infect! Need be a value "
                "in range [0, 1], was " + std::to_string(p_infect));
        }
    }
};

} // namespace Utopia::Models::ContDisease

#endif // UTOPIA_MODELS_CONTDISEASE_PARAMS_HH
