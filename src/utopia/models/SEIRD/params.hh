#ifndef UTOPIA_MODELS_SEIRD_PARAMS_HH
#define UTOPIA_MODELS_SEIRD_PARAMS_HH

#include <queue>

#include <utopia/core/types.hh>
#include <utopia/data_io/cfg_utils.hh>

namespace Utopia::Models::SEIRD
{
/// Parameters specifying the exposure control
struct ExposureContParams
{
    // -- Type definitions ----------------------------------------------------
    /// Type of the times queue
    using TimesQueue = std::queue<std::size_t>;

    /// The type of the change p_exposure pairs
    using TimesValuesQueue = std::queue<std::pair<std::size_t, double>>;

    // -- Parameters ----------------------------------------------------
    /// Whether exposure control is enabled
    const bool enabled;

    /// The number of exposures added to the default p_expose
    const std::size_t num_additional_exposures;

    /// Add additional exposures at these time steps
    mutable TimesQueue at_times;

    /// Change p_expose to new value at given times
    /** Each element of this container provides a pair of [time, new_value].
     *   If the iteration step (time) of the simulation is reached
     *   p_expose is set to new_value
     */
    mutable TimesValuesQueue change_p_expose;

    /// Configuration constructor
    /** Construct an ExposureContParams object with required parameters being
     *  extracted from a configuration node with the same parameter names.
     */
    ExposureContParams(const DataIO::Config& cfg) :
        enabled(get_as<bool>("enabled", cfg)),
        num_additional_exposures {
            get_as<std::size_t>("num_additional_exposures", cfg, 0)},
        at_times {[&]() {
            auto cont = get_as<std::vector<std::size_t>>("at_times", cfg, {});

            std::sort(cont.begin(), cont.end());

            // Copy elements into the queue
            TimesQueue q {};
            for (const auto& v : cont) {
                q.push(v);
            }

            return q;
        }()},
        change_p_expose {[&]() {
            // Check the parameter
            if (not cfg["change_p_expose"] or
                not cfg["change_p_expose"].size()) {
                // Key did not exist or was empty; return empty queue.
                return TimesValuesQueue {};
            }
            else if (not cfg["change_p_expose"].IsSequence()) {
                // Inform about bad type of the given configuration entry
                throw std::invalid_argument(
                    "Parameter change_p_expose need be "
                    "a sequence of pairs, but was not! Given infection control "
                    "parameters:\n" +
                    DataIO::to_string(cfg));
            }

            auto cont = get_as<std::vector<std::pair<std::size_t, double>>>(
                "change_p_expose",
                cfg);

            // Sort such that times low times are in the beginning of the queue
            std::sort(cont.begin(),
                      cont.end(),
                      [](const auto& a, const auto& b) {
                          return a.first < b.first;
                      });

            // Copy elements into the queue
            TimesValuesQueue q {};
            for (const auto& v : cont) {
                q.push(v);
            }

            return q;
        }()} {};
};

/// Parameters specifying the immunity control
struct ImmunityContParams
{
    // -- Type definitions ----------------------------------------------------
    /// Type of the times queue
    using TimesQueue = std::queue<std::size_t>;

    /// The type of the change p_immune pairs
    using TimesValuesQueue = std::queue<std::pair<std::size_t, double>>;

    // -- Parameters ----------------------------------------------------
    /// Whether immunity control is enabled
    const bool enabled;

    /// The number of immunities added to the default p_expose
    const std::size_t num_additional_immunities;

    /// Add additional immunities at these time steps
    mutable TimesQueue at_times;

    /// Change p_immune to new value at given times
    /** Each element of this container provides a pair of [time, new_value].
     *   If the iteration step (time) of the simulation is reached
     *   p_immune is set to new_value
     */
    mutable TimesValuesQueue change_p_immune;

    /// Configuration constructor
    /** Construct an ImmunityContParams object with required parameters being
     *  extracted from a configuration node with the same parameter names.
     */
    ImmunityContParams(const DataIO::Config& cfg) :
        enabled(get_as<bool>("enabled", cfg)),
        num_additional_immunities {
            get_as<std::size_t>("num_additional_immunities", cfg, 0)},
        at_times {[&]() {
            auto cont = get_as<std::vector<std::size_t>>("at_times", cfg, {});

            std::sort(cont.begin(), cont.end());

            // Copy elements into the queue
            TimesQueue q {};
            for (const auto& v : cont) {
                q.push(v);
            }

            return q;
        }()},
        change_p_immune {[&]() {
            // Check the parameter
            if (not cfg["change_p_immune"] or
                not cfg["change_p_immune"].size()) {
                // Key did not exist or was empty; return empty queue.
                return TimesValuesQueue {};
            }
            else if (not cfg["change_p_immune"].IsSequence()) {
                // Inform about bad type of the given configuration entry
                throw std::invalid_argument(
                    "Parameter change_p_immune need be "
                    "a sequence of pairs, but was not! Given infection control "
                    "parameters:\n" +
                    DataIO::to_string(cfg));
            }

            auto cont = get_as<std::vector<std::pair<std::size_t, double>>>(
                "change_p_immune",
                cfg);

            // Sort such that low times are in the beginning of the queue
            std::sort(cont.begin(),
                      cont.end(),
                      [](const auto& a, const auto& b) {
                          return a.first < b.first;
                      });

            // Copy elements into the queue
            TimesValuesQueue q {};
            for (const auto& v : cont) {
                q.push(v);
            }

            return q;
        }()} {};
};

/// Parameters of the SEIRD
struct Params
{
    /// Probability per site and time step to go from state empty to susceptible
    const double p_susceptible;

    /// Probability per transition to susceptible via p_susceptible to be immune
    mutable double p_immune;

    /// Probability per site and time step for a tree cell to not become
    /// infected if an infected cell is in the neighborhood.
    const double p_random_immunity;

    /// Probability per susceptible cell and  time step to transition to exposed
    /// state
    mutable double p_exposure;

    /// Probability per exposed cell and time step to transition to infected
    /// state
    /** This probability will define the typical incubation period of the
     * desease.
     */
    mutable double p_infected;

    /// Probability for a cell to recover
    double p_recover;

    /// Probability for a cell to desease
    double p_decease;

    /// Probability for a cell to become empty
    double p_empty;

    /// The probability to loose immunity if a cell is recovered
    double p_lose_immunity;

    /// Whether to globally allow moving away from infected neighboring cells
    bool move_away_from_infected;

    /// Probability to move randomly if the neighboring cell is empty
    double p_move_randomly;

    /// Exposure control parameters
    const ExposureContParams exposure_control;

    /// Immunity control parameters
    const ImmunityContParams immunity_control;

    /// Construct the parameters from the given configuration node
    Params(const DataIO::Config& cfg) :
        p_susceptible(get_as<double>("p_susceptible", cfg)),
        p_immune(get_as<double>("p_immune", cfg)),
        p_random_immunity(get_as<double>("p_random_immunity", cfg)),
        p_exposure(get_as<double>("p_exposure", cfg)),
        p_infected(get_as<double>("p_infected", cfg)),
        p_recover(get_as<double>("p_recover", cfg)),
        p_decease(get_as<double>("p_decease", cfg)),
        p_empty(get_as<double>("p_empty", cfg)),
        p_lose_immunity(get_as<double>("p_lose_immunity", cfg)),
        move_away_from_infected(get_as<bool>("move_away_from_infected", cfg)),
        p_move_randomly(get_as<double>("p_move_randomly", cfg)),
        exposure_control(get_as<DataIO::Config>("exposure_control", cfg)),
        immunity_control(get_as<DataIO::Config>("immunity_control", cfg))
    {
        if ((p_susceptible > 1) or (p_susceptible < 0)) {
            throw std::invalid_argument(
                "Invalid p_susceptible! Need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a susceptible "
                "one. Was: " +
                std::to_string(p_susceptible));
        }
        if ((p_immune > 1) or (p_immune < 0)) {
            throw std::invalid_argument(
                "Invalid p_immune! Need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a susceptible "
                "one. Was: " +
                std::to_string(p_immune));
        }
        if ((p_random_immunity > 1) or (p_random_immunity < 0)) {
            throw std::invalid_argument(
                "Invalid p_random_immunity! Need be in range "
                "[0, 1], was " +
                std::to_string(p_random_immunity));
        }
        if ((p_exposure > 1) or (p_exposure < 0)) {
            throw std::invalid_argument("Invalid p_exposure! Need be a value "
                                        "in range [0, 1], was " +
                                        std::to_string(p_exposure));
        }
        if ((p_infected > 1) or (p_infected < 0)) {
            throw std::invalid_argument("Invalid p_infected! Need be a value "
                                        "in range [0, 1], was " +
                                        std::to_string(p_infected));
        }
        if ((p_recover > 1) or (p_recover < 0)) {
            throw std::invalid_argument("Invalid p_recover! Need be a value "
                                        "in range [0, 1], was " +
                                        std::to_string(p_recover));
        }
        if ((p_decease > 1) or (p_decease < 0)) {
            throw std::invalid_argument("Invalid p_decease! Need be a value "
                                        "in range [0, 1], was " +
                                        std::to_string(p_decease));
        }
        if (((p_decease + p_recover) > 1)) {
            throw std::invalid_argument(
                "Invalid p_decease and p_recover! The sum needs to be a value "
                "smaller than 1, was " +
                std::to_string(p_decease + p_recover));
        }
        if ((p_empty > 1) or (p_empty < 0)) {
            throw std::invalid_argument("Invalid p_empty! Need be a value "
                                        "in range [0, 1], was " +
                                        std::to_string(p_empty));
        }
        if ((p_lose_immunity > 1) or (p_lose_immunity < 0)) {
            throw std::invalid_argument(
                "Invalid p_lose_immunity! Need be a value "
                "in range [0, 1], was " +
                std::to_string(p_lose_immunity));
        }
        if ((p_move_randomly > 1) or (p_move_randomly < 0)) {
            throw std::invalid_argument(
                "Invalid p_move_randomly! Need be a value "
                "in range [0, 1], was " +
                std::to_string(p_move_randomly));
        }
    }
};

}  // namespace Utopia::Models::SEIRD

#endif  // UTOPIA_MODELS_SEIRD_PARAMS_HH
