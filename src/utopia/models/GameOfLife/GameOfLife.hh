#ifndef UTOPIA_MODELS_GAMEOFLIFE_HH
#define UTOPIA_MODELS_GAMEOFLIFE_HH

// standard library includes
#include <random>
#include <string>
#include <algorithm>

// third-party library includes

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>
#include <utopia/core/apply.hh>
#include <utopia/data_io/cfg_utils.hh>

namespace Utopia::Models::GameOfLife
{
// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The type of a cell's state
struct CellState
{
    /// Whether a cell lives or not
    bool living;

    /// Construct the cell state from a configuration and an RNG
    template<class RNG>
    CellState(const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    {
        // Initialize cells randomly
        // NOTE This check can be improved if the string comparison is replaced
        //      e.g. by an enum class comparisson. However, reading in the
        //      configuration directly as enum class generates errors because
        //      because of not allowed type conversions.
        if (get_as<std::string>("mode", cfg) == "random") {
            if (not get_as<DataIO::Config>("random", cfg)) {
                // Is set, but is false. Just return:
                return;
            }

            // Get the probability to be living
            auto p_living = get_as<double>("p_living", cfg["random"]);

            // Create a uniform real distribution
            auto dist = std::uniform_real_distribution<double>(0., 1.);

            // With probability p_living, a cell lives
            if (dist(*rng) < p_living) {
                living = true;
            }
            else {
                living = false;
            }
        }
        // else: the config option was not available
    }
};

/// Specialize the CellTraits type helper for this model
/** Specifies the type of each cells' state as first template argument and the
 * update mode as second.
 *
 * See \ref Utopia::CellTraits for more information.
 */
using CellTraits = Utopia::CellTraits<CellState, Update::manual>;

/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;

// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The GameOfLife Model
/** This model implement Conways Game of in the Utopia way.
 */
class GameOfLife : public Model<GameOfLife, ModelTypes>
{
  public:
    /// The type of the Model base class of this derived class
    using Base = Model<GameOfLife, ModelTypes>;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CellTraits, GameOfLife>;
    // NOTE that it requires the model's type as second template argument

    /// Type of container to store the number of neighbors for the life rule
    using NbLifeRule = std::unordered_set<unsigned short>;

    /// Extract the type of the rule function from the CellManager
    /** This is a function that receives a reference to a cell and returns the
     * new cell state. For more details, check out \ref Utopia::CellManager
     *
     * \note   Whether the cell state gets applied directly or
     *         requires a synchronous update depends on the update mode
     *         specified in the cell traits.
     */
    using RuleFunc = typename CellManager::RuleFunc;

  private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _log, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;

    /// The rule in Mirek's Cellebration notation
    const std::string _rule;

    /// The number of neighbors required to get born
    const NbLifeRule _birth;

    /// The number of neighbors required to survive
    const NbLifeRule _survive;

    // .. Temporary objects ...................................................

    // .. Datasets ............................................................
    /// A dataset for storing all cells living or dead status
    std::shared_ptr<DataSet> _dset_living;

  public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the GameOfLife model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    GameOfLife(const std::string name, ParentModel& parent) :
        // Initialize first via base model
        Base(name, parent),

        // Now initialize the cell manager
        _cm(*this),

        // Initialize the rule and extract the number of neighbors required
        // for birth and survival
        _rule(get_as<std::string>("rule", this->_cfg)),
        _birth(this->extract_birth_from_rule()),
        _survive(this->extract_survive_from_rule()),

        // Datasets
        // For setting up datasets that store CellManager data, you can use the
        // helper functions to take care of setting them up:
        _dset_living(this->create_cm_dset("living", _cm))
    {
        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);
    }

  private:
    // .. Setup functions .....................................................
    /// Extract the number of neighbors required for birth from the rule
    NbLifeRule extract_birth_from_rule()
    {
        const std::string delimiter = "/";
        const std::string birth_s   = _rule.substr(0, _rule.find(delimiter));

        NbLifeRule birth;
        for (auto b_char : birth_s) {
            // Subtracting '0' is needed to get from the ASCII number stored as
            // char to the corresponding unsigned short value.
            auto b = static_cast<unsigned short>(b_char - '0');
            birth.emplace(b);
        }
        return birth;
    }

    /// Extract the number of neighbors required to survive from the rule
    NbLifeRule extract_survive_from_rule()
    {
        // Get the substring specifying `survive` which is the substring that
        // starts after the delimiter and ends with the end of the rule string.
        const std::string delimiter = "/";
        const auto pos_delimiter    = _rule.find(delimiter);
        const std::string survive_s =
            _rule.substr(pos_delimiter + 1, _rule.size() - pos_delimiter);

        NbLifeRule survive;
        for (auto b_char : survive_s) {
            // Subtracting '0' is needed to get from the ASCII number stored as
            // char to the corresponding unsigned short value.
            auto b = static_cast<unsigned short>(b_char - '0');
            survive.emplace(b);
        }
        return survive;
    }

    // .. Helper functions ....................................................

    /// Calculate the mean of all cells' some_state
    double calculate_living_cell_density() const
    {
        double sum = 0.;
        for (const auto& cell : _cm.cells()) {
            sum += cell->state.living;
        }
        return sum / _cm.cells().size();
    }

    // .. Rule functions ......................................................
    /// Implement the general life-like rule
    const RuleFunc _life_rule = [this](const auto& cell) {
        // Get the current state of the cell
        auto state = cell->state;

        // Calculate the number of living neighbors
        auto num_living_nbs {0u};
        for (auto nb : this->_cm.neighbors_of(cell)) {
            if (nb->state.living) {
                ++num_living_nbs;
            }
        }

        const bool survive =
            std::find(_survive.begin(), _survive.end(), num_living_nbs) !=
            _survive.end();
        // Die if the number of living neighbors is not in the survival
        // container
        if (not survive) {
            state.living = false;
        }

        // Give birth if the number of living neighbors is in the birth
        // container
        const bool birth =
            std::find(_birth.begin(), _birth.end(), num_living_nbs) !=
            _birth.end();
        if (birth) {
            state.living = true;
        }

        // Return the new cell state
        return state;
    };

  public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    void perform_step()
    {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule<Update::sync>(_life_rule, _cm.cells());
    }

    /// Monitor model information
    /** \details Here, functions and values can be supplied to the monitor that
     *           are then available to the frontend. The monitor() function is
     *           _only_ called if a certain emit interval has passed; thus, the
     *           performance hit is small.
     *           With this information, you can then define stop conditions on
     *           frontend side, that can stop a simulation once a certain set
     *           of conditions is fulfilled.
     */
    void monitor()
    {
        this->_monitor.set_entry("living_cell_density",
                                 calculate_living_cell_density());
    }

    /// Write data
    /** \details This function is called to write out data.
     *          The configuration determines the times at which it is invoked.
     *          See \ref Utopia::DataIO::Dataset::write
     */
    void write_data()
    {
        // Write out the some_state of all cells
        _dset_living->write(_cm.cells().begin(),
                            _cm.cells().end(),
                            [](const auto& cell) {
                                return static_cast<char>(cell->state.living);
                            });
    }

    // .. Getters and setters .................................................
    // Add getters and setters here to interface with other models
};

}  // namespace Utopia::Models::GameOfLife

#endif  // UTOPIA_MODELS_GAMEOFLIFE_HH
