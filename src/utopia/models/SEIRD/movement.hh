#ifndef UTOPIA_MODELS_SEIRD_MOVEMENT
#define UTOPIA_MODELS_SEIRD_MOVEMENT

#include "state.hh"

namespace Utopia::Models::SEIRD
{
/// Check whether the nb_state kind is empty in if true move towards it
/** Return true if the agent successfully moved to the neighboring cell.
 * If the neighboring cell was not empty return false.
 */
auto move_to_empty_cell = [](auto& state, auto& nb_state) {
    if (nb_state.kind == Kind::empty) {
        // Update the neighboring state
        nb_state.kind           = state.kind;
        nb_state.num_recoveries = state.num_recoveries;
        nb_state.immune         = state.immune;

        // Update the cell state
        state.kind           = Kind::empty;
        state.num_recoveries = 0;
        state.immune         = false;

        // Moving to an empty cell was succesful :)
        return true;
    }
    else {
        return false;
    }
};

}  // namespace Utopia::Models::SEIRD

#endif  // namespace UTOPIA_MODELS_SEIRD_MOVEMENT