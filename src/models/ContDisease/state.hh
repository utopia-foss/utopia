#ifndef UTOPIA_MODELS_CONTDISEASE_STATE_HH
#define UTOPIA_MODELS_CONTDISEASE_STATE_HH

#include <utopia/core/types.hh>

namespace Utopia::Models::ContDisease {

/// The kind of the cell: empty, tree, infected, source, stone
enum class Kind : char {
    /// Unoccupied
    empty = 0,
    /// Cell represents a tree
    tree = 1,
    /// Cell is infected
    infected = 2,
    /// Cell is an infection source: constantly infected, spreading infection
    source = 3,
    /// Cell cannot be infected
    stone = 4
};

/// The full cell struct for the ContDisease model
struct State {
    /// The cell state
    Kind kind;

    /// The age of the cell
    unsigned age;

    /// An ID denoting to which cluster this cell belongs
    unsigned int cluster_id;

    /// Construct the cell state from a configuration and an RNG
    template<class RNG>
    State (const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        kind(Kind::empty),
        age(0),
        cluster_id(0)
    {
        // Check if p_tree is available to set up cell state
        if (cfg["p_tree"]) {
            const auto init_density = get_as<double>("p_tree", cfg);

            if (init_density < 0. or init_density > 1.) {
                throw std::invalid_argument("p_tree needs to be in "
                    "interval [0., 1.], but was " 
                    + std::to_string(init_density) + "!");
            }

            // With this probability, the cell state is a tree
            if (std::uniform_real_distribution<double>(0., 1.)(*rng) 
                < init_density) 
            {
                kind = Kind::tree;
            }
        }
    }
};

    
} // namespace Utopia::Models::ContDisease


#endif // UTOPIA_MODELS_CONTDISEASE_STATE_HH
