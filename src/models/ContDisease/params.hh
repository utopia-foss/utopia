#ifndef UTOPIA_MODELS_CONTDISEASE_PARAMS_HH
#define UTOPIA_MODELS_CONTDISEASE_PARAMS_HH

#include <utopia/core/types.hh>
#include <utopia/data_io/cfg_utils.hh>

namespace Utopia::Models::ContDisease
{

/// Parameters for stone initialization
struct StoneInitParams {
    /// The mode in which to initialize stones
    const std::string mode;

    /// The probability with which to add stones randomly
    const double p_random;

    /// The probability with which to additionally form clusters
    const double p_cluster;

    /// Configuration constructor
    /** \details This constructor constructs a stone parameter object that
     *           also checks whether the given parameters fullfil parameter-
     *           specific requirements.
     */
    StoneInitParams(const DataIO::Config& cfg)
    :
    mode(get_as<std::string>("mode", cfg)),
    p_random(get_as<double>("p_random", cfg)),
    p_cluster(get_as<double>("p_cluster", cfg))
    {
        // Check whether the parameters are valid
        if ((p_random > 1) or (p_random < 0)) {
            throw std::invalid_argument("Invalid p_random! Need be a "
                "value in range [0, 1], was " + std::to_string(p_random));
        }
        if ((p_cluster > 1) or (p_cluster < 0)) {
            throw std::invalid_argument("Invalid p_cluster! Need be a "
                "value in range [0, 1], was " + std::to_string(p_cluster));
        }
        if (mode != "random" and mode != "cluster"){
            throw std::invalid_argument("The stone initialization mode is "
                "is not valid! Needs to be 'random' or 'cluster' but was: "
                + mode + "!");
        }
    }
};

/// Parameters defining the stone behavior
struct StoneParams {
    /// Whether stones are placed on the grid
    const bool on;

    /// Stone initialization parameters
    const StoneInitParams init;

    /// Config-constructor
    StoneParams(const DataIO::Config& cfg)
    :
    on(get_as<bool>("on", cfg)),
    // Use the get_as function to show a meaningful message if option is set
    // wrongly
    init(get_as<DataIO::Config>("initialization", cfg))
    {};
};


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

    // Stone-related parameters
    const StoneParams stones; 

    /// Construct the parameters from the given configuration node
    Params(const DataIO::Config& cfg)
    :
        p_growth(get_as<double>("p_growth", cfg)),
        p_infect(get_as<double>("p_infect", cfg)),
        p_random_infect(get_as<double>("p_random_infect", cfg)),
        infection_source(get_as<bool>("infection_source", cfg)),
        // Use the get_as function to show a meaningful message if option is set
        // wrongly
        stones(get_as<DataIO::Config>("stones", cfg)) 
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
    }
};

}


#endif // UTOPIA_MODELS_CONTDISEASE_PARAMS_HH