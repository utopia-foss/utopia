#ifndef UTOPIA_MODELS_PREDATORPREYPLANT_SPECIES_HH
#define UTOPIA_MODELS_PREDATORPREYPLANT_SPECIES_HH

#include <utility>

#include <utopia/models/PredatorPrey/PredatorPrey.hh>

namespace Utopia::Models::PredatorPreyPlant {


/// Struct that holds all plant characterising states
struct PlantState{
    /// Whether a plant is on the cell
    bool on_cell;

    /// The regeneration time counter
    /** If the plant growth model if determinstic, a plant regrows after a 
     *  deterministic regeneration time. The regeneration time counter counts
     *  the time that has passed since the last plant was removed from the 
     *  cell. If the counter reaches the regeneration time, a new plant is on
     *  the cell and the counter is reset.
     * 
     *  \note Only used for GrowthModel::deterministic
     */ 
    unsigned int regeneration_counter = 0;
};

/// The growth model to use for plants
enum class GrowthModel {
    /// Plant level is ignored; prey are always able to eat
    none,
    /// Once eaten, a plant requires ``regen_time`` time to regenerate
    deterministic,
    /// Once eaten, a plant regrows with probability ``regen_prob``
    stochastic
};

/// The parameters characterizing plants
struct PlantParams {
    /// The growth model of the plant
    const GrowthModel growth_model;

    /// The deterministic regeneration time
    const unsigned int regen_time;

    /// The regeneration probability, evaluated each time step
    const double regen_prob;
    
    // .. Constructors ........................................................
    /// Construct a species from a configuration node
    PlantParams(const Utopia::DataIO::Config& cfg)
    :
        // Set the growth model from the configuration
        growth_model([&cfg](){
            auto gm = get_as<std::string>("growth_model", cfg);
            if (gm == "deterministic") return GrowthModel::deterministic;
            else if (gm == "stochastic") return GrowthModel::stochastic;
            else return GrowthModel::none;
        }()),
        // Extract regeneration time and probability
        regen_time([&cfg](){
            const auto t = get_as<int>("regen_time", cfg);
            return t;
        }()),
        regen_prob(get_as<double>("regen_prob", cfg))
    {};

    // Do not allow the default constructor
    PlantParams() = delete;
};

/// Struct that holds all species-specific parameters
struct SpeciesBaseParams : PredatorPrey::SpeciesBaseParams{
    /// Movement limit
    unsigned int move_limit;

    // .. Constructors ........................................................
    /// Construct a species from a configuration node
    SpeciesBaseParams(const Utopia::DataIO::Config& cfg)
    :
        PredatorPrey::SpeciesBaseParams(cfg),
        move_limit([&cfg](){
            const auto lim = get_as<int>("move_limit", cfg);
            return lim;
        }())
    {}

    SpeciesBaseParams() = delete;
};

/// Struct that holds all predator-specific parameters
struct PredatorParams : public SpeciesBaseParams{
    // .. Constructors ........................................................
    /// Construct a predator object from a configuration node
    PredatorParams(const Utopia::DataIO::Config& cfg)
    :
        SpeciesBaseParams(cfg)
    {
        double repro_cost = get_as<double>("repro_cost", cfg);
        double repro_resource_requ = get_as<double>("repro_resource_requ", cfg);
        if (repro_cost > repro_resource_requ) {
            throw std::invalid_argument("Parameter repro_cost needs to be "
                "smaller than or equal to the minimal resources required for "
                "reproduction!");
        }
    };

    /// The default constructor
    PredatorParams() = delete;
};

/// Struct that holds all prey-species specific parameters
struct PreyParams : public SpeciesBaseParams {
    // .. Interaction .........................................................
    /// Probability to flee from a predator if on the same cell
    double p_flee;

    // .. Constructors ........................................................
    /// Construct a prey object from a configuration node
    PreyParams(const Utopia::DataIO::Config& cfg)
    :
        SpeciesBaseParams(cfg),
        p_flee(get_as<double>("p_flee", cfg))
    {
        double repro_cost = get_as<double>("repro_cost", cfg);
        double repro_resource_requ = get_as<double>("repro_resource_requ", cfg);
        if (repro_cost > repro_resource_requ) {
            throw std::invalid_argument("Parameter repro_cost needs to be "
                "smaller than or equal to the minimal resources required for "
                "reproduction!");
        }
    };

    /// The default constructor
    PreyParams() = delete;
};

/// The parameter of all species
/** This struct contains for each available species one species-specific
 *  parameter struct that contains all parameters belonging to that
 *  specific species and plant.
 */
struct SpeciesParams{
    /// Prey parameters
    PreyParams prey;

    /// Predator parameters
    PredatorParams predator;

    /// Plant parameters
    PlantParams plant;

    // .. Constructors ........................................................
    /// Construct through a configuration file
    SpeciesParams(const Utopia::DataIO::Config& cfg)
    :
        prey(cfg["prey"]),
        predator(cfg["predator"]),
        plant(cfg["plant"])
    {};

    /// Default constructor
    SpeciesParams() = delete;
};


} // namespace Utopia::Models::PredatorPreyPlant

#endif // UTOPIA_MODELS_PREDATORPREYPLANT_SPECIES_HH
