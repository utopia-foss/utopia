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
    
    /// Resource limits [min, max] for the resource level
    std::pair<double, double> resource_limits;

    // .. Reproduction ........................................................
    /// Cost of reproduction
    double cost_of_repr;
    
    /// Reproduction probability
    double p_repro;

    // .. Constructors ........................................................
    /// Construct a species from a configuration node
    Species(const Utopia::DataIO::Config& cfg){
        cost_of_living = get_as<double>("cost_of_living", cfg);
        resource_intake = get_as<double>("resource_intake", cfg);
        resource_limits = get_as<std::pair<double, double>>("resource_limits",
                                                            cfg);
        cost_of_repr = get_as<double>("cost_of_repr", cfg);
        p_repro = get_as<double>("p_repro", cfg);
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
    Prey(const Utopia::DataIO::Config& cfg) : Species(cfg){};

    Prey() = delete;
};

}
}
}


#endif // UTOPIA_MODELS_PREDATORPREY_SPECIES_HH
