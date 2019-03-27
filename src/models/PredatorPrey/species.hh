#ifndef UTOPIA_MODELS_PREDATORPREY_SPECIES_HH
#define UTOPIA_MODELS_PREDATORPREY_SPECIES_HH

#include <utility>

#include "utopia/core/types.hh"

namespace Utopia {
namespace Models {
namespace PredatorPrey {

/// Struct that holds all species-specific parameters
struct Species {
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
    /// Construct a species from a configuration node
    Species(const Utopia::DataIO::Config& cfg){
        cost_of_living = get_as<double>("cost_of_living", cfg);
        resource_intake = get_as<double>("resource_intake", cfg);
        repro_resource_requ = get_as<double>("repro_resource_requ", cfg);
        resource_max = get_as<double>("resource_max", cfg);
        repro_cost = get_as<double>("repro_cost", cfg);
        repro_prob = get_as<double>("repro_prob", cfg);
    }

    Species() = delete;
};

/// Struct that holds all predator-specific parameters
struct Predator : public Species{
    // .. Constructors ........................................................
    /// Construct a predator object from a configuration node
    Predator(const Utopia::DataIO::Config& cfg) : Species(cfg){};

    Predator() = delete;
};

/// Struct that holds all prey-species specific parameters
struct Prey : public Species {
    // .. Interaction .........................................................
    // Probability to flee from a predator if on the same cell
    double p_flee;

    // .. Constructors ........................................................
    /// Construct a prey object from a configuration node
    Prey(const Utopia::DataIO::Config& cfg) : Species(cfg){
        p_flee = get_as<double>("p_flee", cfg);
    };

    Prey() = delete;
};

}
}
}


#endif // UTOPIA_MODELS_PREDATORPREY_SPECIES_HH
