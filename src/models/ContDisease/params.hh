#ifndef UTOPIA_MODELS_CONTDISEASE_PARAMS_HH
#define UTOPIA_MODELS_CONTDISEASE_PARAMS_HH

#include <utopia/core/types.hh>
#include <utopia/data_io/cfg_utils.hh>

namespace Utopia::Models::ContDisease
{

/// Parameters of the ContDisease
struct Params {
    /// Probability per site and time step to transition from state empty to tree
    const double p_growth;

    /// Probability per site and time step for a tree cell to become infected
    /// if an infected cell is in the neighborhood.
    const double p_infect;

    /// Probability per site and time step for a random point infection of a
    // tree cell
    const double p_random_infect;

    /// Infection source - set true to activate a constant row of infected
    /// cells at the bottom boundary
    const bool infection_source;

    // Extract if stones are activated
    const bool stones;

    // Extract how stones are to be initialized
    const std::string stone_init;

    // Extract stone density for stone_init = random
    const double stone_density;

    // Extract clustering weight for stone_init = random
    const double stone_cluster;

    /// Construct the parameters from the given configuration node
    Params(const DataIO::Config& cfg)
    :
        p_growth(get_as<double>("p_growth", cfg)),
        p_infect(get_as<double>("p_infect", cfg)),
        p_random_infect(get_as<double>("p_random_infect", cfg)),
        infection_source(get_as<bool>("infection_source", cfg)),
        stones(get_as<bool>("stones", cfg)),
        stone_init(get_as<std::string>("stone_init", cfg)),
        stone_density(get_as<double>("stone_density", cfg)),
        stone_cluster(get_as<double>("stone_cluster", cfg))
    {
        if ((p_growth > 1) or (p_growth < 0)) {
            throw std::invalid_argument("Invalid p_growth; need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a tree. Was: "
                + std::to_string(p_growth));
        }
        if ((p_infect > 1) or (p_infect < 0)) {
            throw std::invalid_argument("Invalid p_infect! Need be in range "
                "[0, 1], was " + std::to_string(p_infect));
        }
        if ((p_random_infect > 1) or (p_random_infect < 0)) {
            throw std::invalid_argument("Invalid p_random_infect; Need be a value "
                "in range [0, 1], was " + std::to_string(p_random_infect));
        }
        if ((stone_density > 1) or (stone_density < 0)) {
            throw std::invalid_argument("Invalid stone_density! Need be a "
                "value in range [0, 1], was " + std::to_string(stone_density));
        }
        if ((stone_cluster > 1) or (stone_cluster < 0)) {
            throw std::invalid_argument("Invalid stone_cluster! Need be a "
                "value in range [0, 1], was " + std::to_string(stone_cluster));
        }
    }
};

}


#endif // UTOPIA_MODELS_CONTDISEASE_PARAMS_HH