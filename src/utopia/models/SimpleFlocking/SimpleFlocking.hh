#ifndef UTOPIA_MODELS_SIMPLEFLOCKING_HH
#define UTOPIA_MODELS_SIMPLEFLOCKING_HH

#include <random>
#include <functional>

#include <utopia/core/model.hh>
#include <utopia/core/types.hh>
#include <utopia/core/agent_manager.hh>
#include <utopia/core/apply.hh>

#include "state.hh"


namespace Utopia::Models::SimpleFlocking {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;

/// Agent traits specialization using the state type
/** The first template parameter specifies the type of the cell state,
  * the second sets them to be synchronously updated.
  */
using AgentTraits = Utopia::AgentTraits<AgentState, Update::sync>;




// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/// The SimpleFlocking Model
/** TODO
 */
class SimpleFlocking:
    public Model<SimpleFlocking, ModelTypes>
{
public:
    /// The type of the Model base class of this derived class
    using Base = Model<SimpleFlocking, ModelTypes>;

    /// The type of the agent manager
    using AgentManager = Utopia::AgentManager<AgentTraits, SimpleFlocking>;

    /// Type of the update rules for agents
    using Rule = typename AgentManager::RuleFunc;

    /// Type of the update rules for agents where no state is to be returned
    using VoidRule = typename AgentManager::VoidRuleFunc;

    /// Type of spatial vectors within the domain
    using SpaceVec = typename AgentManager::SpaceVec;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;


private:
    // -- Members -------------------------------------------------------------

    /// The agent manager
    AgentManager _am;

    // .. Global parameters ...................................................

    /// The radius within which agents interact with each other
    double _interaction_radius;

    /// The amplitude of the noise applied to the orientation update
    double _noise_level;


    // .. Temporary and helper objects ........................................

    /// The distribution used for determining the orientation noise
    std::uniform_real_distribution<double> _noise_distr;


    // .. Datasets ............................................................
    std::shared_ptr<DataSet> _dset_x;
    std::shared_ptr<DataSet> _dset_y;
    std::shared_ptr<DataSet> _dset_orientation;


public:
    // -- Model Setup ---------------------------------------------------------

    /// Construct the SimpleFlocking model
    /** \param name             Name of this model instance; is used to extract
     *                          the configuration from the parent model and
     *                          set up a HDFGroup for this instance
     *  \param parent_model     The parent model this model instance resides in
     *  \param custom_cfg       A custom configuration to use instead of the
     *                          one extracted from the parent model using the
     *                          instance name
     */
    template<class ParentModel>
    SimpleFlocking (
        const std::string& name,
        ParentModel& parent_model,
        const DataIO::Config& custom_cfg = {}
    )
    :
        Base(name, parent_model, custom_cfg)

    ,   _am(*this)

    ,   _interaction_radius(get_as<double>("interaction_radius", this->_cfg))
    ,   _noise_level(get_as<double>("noise_level", this->_cfg))

    ,   _noise_distr(-_noise_level/2., +_noise_level/2.)

    ,   _dset_x(this->create_am_dset("x", _am))
    ,   _dset_y(this->create_am_dset("y", _am))
    ,   _dset_orientation(this->create_am_dset("orientation", _am))
    {}


private:
    // .. Setup functions .....................................................

    // .. Helper functions ....................................................

public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    void perform_step () {
        apply_rule(_adjust_orientation, _am.agents());
        apply_rule(_move, _am.agents());
    }


    /// Monitor model information
    void monitor () {
        // this->_monitor.set_entry("orientation_stddev", nan);  // TODO
    }


    /// Write data
    void write_data () {
        using WriteT = float;
        const auto& agents = _am.agents();

        _dset_x->write(
            agents.begin(), agents.end(),
            [](const auto& agent) {
                return static_cast<WriteT>(agent->position()[0]);
        });

        _dset_y->write(
            agents.begin(), agents.end(),
            [](const auto& agent) {
                return static_cast<WriteT>(agent->position()[1]);
        });

        _dset_orientation->write(
            agents.begin(), agents.end(),
            [](const auto& agent) {
                return static_cast<WriteT>(agent->state().get_orientation());
        });
    }


    // Getters and setters ....................................................

    std::size_t num_agents () const {
        return _am.agents().size();
    }


    // Rules ..................................................................

    /// Rule that sets agent orientation to the mean orientation (in a radius)
    const Rule _adjust_orientation = [this](const auto& agent){
        auto state = agent->state();

        // Find all agents within the interaction radius and compute their
        // average orientation, including (!) the current agent.
        // To find an average orientation, we need to separate the x- and y-
        // components and later combine them back into an angle.
        auto agg_sin = 0.;
        auto agg_cos = 0.;

        // TODO This leads to quadratic complexity in agent number!
        //      Find a better approach (on the level of AgentManager).
        for (const auto& a : this->_am.agents()) {
            if (this->_am.distance(a, agent) > this->_interaction_radius) {
                continue;
            }
            // else: is within interaction radius

            agg_sin += std::sin(a->state().get_orientation());
            agg_cos += std::cos(a->state().get_orientation());
        }

        // Can now set the orientation, including noise
        // NOTE Could divide by number of involved agents here, but that is
        //      unnecessary because it cancels out through division anyway.
        state.set_orientation(
            std::atan2(agg_sin, agg_cos) +
            (this->_noise_level > 0. ? _noise_distr(*this->_rng) : 0.)
        );

        return state;
    };

    /// Rule that applies the current displacement vector to agent position
    const VoidRule _move = [this](const auto& agent){
        this->_am.move_by(agent, agent->state().get_displacement());
    };

};


} // namespace Utopia::Models::SimpleFlocking

#endif // UTOPIA_MODELS_SIMPLEFLOCKING_HH
