#ifndef UTOPIA_MODELS_PREDATORPREY_SPECIES_HH
#define UTOPIA_MODELS_PREDATORPREY_SPECIES_HH

#include <utility>

#include "utopia/core/types.hh"

namespace Utopia {
namespace Models {
namespace PredatorPrey {

/// Struct that holds all species states
struct SpeciesState {
    /// The internal resources reservoir
    double resources;

    /// Whether the species is on a cell
    bool on_cell;

    // .. Constructors ........................................................
    /// The default constructor
    SpeciesState() : 
    resources{0},
    on_cell{false}
    {}
};

/// Struct that holds all species-specific parameters
struct SpeciesBaseParams {
    // .. Living ..............................................................
    /// Cost of living that is taken each time step
    double cost_of_living;

    /// Resource intake from eating
    double resource_intake;
    
    /// Minimal reproduction resources requirements
    double repro_resource_requ;

    /// Maximal resource level
    double resource_max;

    // .. Reproduction ........................................................
    /// Cost of reproduction
    double repro_cost;
    
    /// Reproduction probability
    double repro_prob;

    // .. Constructors ........................................................
    /// Load species parameters from a configuration node
    SpeciesBaseParams(const Utopia::DataIO::Config& cfg)
    :
        cost_of_living(get_as<double>("cost_of_living", cfg)),
        resource_intake(get_as<double>("resource_intake", cfg)),
        repro_resource_requ(get_as<double>("repro_resource_requ", cfg)),
        resource_max(get_as<double>("resource_max", cfg)),
        repro_cost(get_as<double>("repro_cost", cfg)),
        repro_prob(get_as<double>("repro_prob", cfg))
    {
        // Perform some checks
        if (repro_cost >= repro_resource_requ) {
            throw std::invalid_argument("repro_cost needs to be smaller than "
                "or equal to the minimal reproduction requirements of "
                "resources!");
        }
    }

    SpeciesBaseParams() = delete;
};

/// Struct that holds all predator-specific parameters
struct PredatorParams : public SpeciesBaseParams{
    // .. Constructors ........................................................
    /// Construct a predator object from a configuration node
    PredatorParams(const Utopia::DataIO::Config& cfg) : SpeciesBaseParams(cfg){};

    /// The default constructor
    PredatorParams() = delete;
};

/// Struct that holds all prey-species specific parameters
struct PreyParams : public SpeciesBaseParams {
    // .. Interaction .........................................................
    // Probability to flee from a predator if on the same cell
    double p_flee;

    // .. Constructors ........................................................
    /// Construct a prey object from a configuration node
    PreyParams(const Utopia::DataIO::Config& cfg) : SpeciesBaseParams(cfg){
        p_flee = get_as<double>("p_flee", cfg);
    };

    /// The default constructor
    PreyParams() = delete;
};

/// The parameter of all species
/** \detail This struct contains for each available species one species-specific
 *          parameter struct that contains all parameters belonging to that
 *          specific species.
 */
struct SpeciesParams{
    /// Prey parameters
    PreyParams prey;

    /// Predator parameters
    PredatorParams predator;

    // .. Constructors ........................................................
    /// Construct through a configuration file
    SpeciesParams(const Utopia::DataIO::Config& cfg)
    :
        prey(cfg["prey"]),
        predator(cfg["predator"])
    {};

    /// Default constructor
    SpeciesParams() = delete;
};

}
}
}


#endif // UTOPIA_MODELS_PREDATORPREY_SPECIES_HH
