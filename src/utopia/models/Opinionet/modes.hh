#ifndef UTOPIA_MODELS_OPINIONET_MODES
#define UTOPIA_MODELS_OPINIONET_MODES

namespace Utopia::Models::Opinionet::Modes{

// This header provides the various model modes

enum Interaction_type {
    Deffuant,
    HegselmannKrause
};

enum Opinion_space_type {
    continuous,
    discrete
};

enum Rewiring {
    RewiringOn,
    RewiringOff
};

// Enable printing model modes as strings in logger

std::string InteractionTypes[] = {
    "Deffuant",
    "HegselmannKrause"
};

std::string OpinionSpaceTypes[] = {
    "continuous",
    "discrete"
};


} //namespace

#endif // UTOPIA_MODELS_OPINIONET_MODES
