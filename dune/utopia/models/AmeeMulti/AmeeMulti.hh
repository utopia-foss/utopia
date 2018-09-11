#ifndef UTOPIA_MODELS_COPYME_HH
#define UTOPIA_MODELS_COPYME_HH

#include "agentstate.hh"
#include "cellstate.hh"
#include <dune/utopia/base.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/types.hh>

#include <functional>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/// The AmeeMulti Model
/** Add your model description here.
 *  This model's only right to exist is to be a template for new models.
 *  That means its functionality is based on nonsense but it shows how
 *  actually useful functionality could be implemented.
 */
template <typename Cell, typename CellManager, typename AgentManager, typename Modeltypes, bool construction, bool decay>
class AmeeMulti
    : public Model<AmeeMulti<Cell, CellManager, AgentManager, Modeltypes, construction, decay>, Modeltypes>
{
public:
    using AgentType = typename AgentManager::Agent;
    using CellType = typename CellManager::Cell;
    /// CellState
    using Cellstate = typename CellType::State;

    /// Agentstate
    using Agentstate = typename AgentType::State;

    // cell traits typedefs
    using CT = typename Cellstate::Celltraits;
    using CTV = typename CT::value_type;

    using AP = typename Agentstate::Phenotype;
    using APV = typename AP::value_type;

    using AG = typename Agentstate::Genotype;
    using AGV = typename AG::value_type;

    using Types = Modeltypes;

    /// The base model type
    using Base =
        Model<AmeeMulti<Cell, CellManager, AgentManager, Modeltypes, construction, decay>, Modeltypes>;

    /// Data type that holds the configuration
    using Config = typename Base::Config;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

    // Alias the neighborhood classes to make access more convenient
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;

private:
    // function types
    using AgentFunction = std::function<void(const std::shared_ptr<AgentType>)>;

    using CellFunction = std::function<void(const std::shared_ptr<CellType>)>;

    using Adaptionfunction = std::function<double(const std::shared_ptr<AgentType>)>;

public:
};

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYME_HH
