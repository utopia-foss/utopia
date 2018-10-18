#ifndef UTOPIA_MODELS_FORESTFIREMODEL_HH
#define UTOPIA_MODELS_FORESTFIREMODEL_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>


namespace Utopia {
namespace Models {
namespace ForestFireModel {

enum StateEnum { empty=0, tree=1, burning=2 };

struct State {
    StateEnum state;
    int cluster_tag;

    State() :
        state(empty),  cluster_tag(-1)
    { }
    State(StateEnum s) :
        state(s),  cluster_tag(-1)
    { }
};

struct Param {
    const double growth_rate;
    const double lightning_frequency;
    const double resistance;

    Param(double growth_rate, double lightning_frequency, double resistance)
    :
        growth_rate(growth_rate), lightning_frequency(lightning_frequency),
        resistance(resistance)
    {
        if (growth_rate > 1 || growth_rate < 0)
        {
            throw std::runtime_error("growth rate is a probability per cell. \
                Should have value in [0,1]! \
                1 corresponds to empty turns to tree in 1 step. \
                0.1 every 10th step. 0 never. ");
        }
        if (lightning_frequency > 1 || lightning_frequency < 0)
        {
            throw std::runtime_error("lightning frequency is a probability per cell. \
                Should have value in [0,1]! \
                1 corresponds to tree hit by lightning in one step. \
                0.1 every 10th step. 0 never. ");
        }
        if (resistance > 1 || resistance < 0)
        {
            throw std::runtime_error("Resistance is a probability per burning neighbor. \
                Should have value in [0,1]! \
                0 corresponds to no resistance to fire. 1 to total resistance.");
        }
    }
};

struct ModelFeature {
    const bool two_state_FFM;
    const bool light_bottom_row;

    ModelFeature(bool two_state_FFM, bool light_bottom_row)
    :
        two_state_FFM(two_state_FFM), light_bottom_row(light_bottom_row)
    { }
};

/// Typehelper to define data types of ForestFireModel model 
using ForestFireModelTypes = ModelTypes<>;
// NOTE if you do not use the boundary condition type, you can delete the
//      definition of the struct above and the passing to the type helper


/// The ForestFireModel Model
/** Add your model description here.
 *  This model's only right to exist is to be a template for new models. 
 *  That means its functionality is based on nonsense but it shows how 
 *  actually useful functionality could be implemented.
 */
template<class ManagerType>
class ForestFireModel:
    public Model<ForestFireModel<ManagerType>, ForestFireModelTypes>
{
public:
    /// The base model type
    using Base = Model<ForestFireModel<ManagerType>, ForestFireModelTypes>;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Rule function type
    using RuleFunc = typename std::function<State(std::shared_ptr<CellType>)>;

    // Alias the neighborhood classes to make access more convenient
    using Neighbor = Utopia::Neighborhoods::MooreNeighbor;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// Model parameters
    const Param _param;
    const ModelFeature _model_feature;  // 0: three state model, contagious disease, 1: two state model, percolation
                                        // 0 implies sync update, 1 async update
    const double _initial_density; // initial density of trees

    // -- Temporary objects -- //
    int _cluster_tag_cnt;

    // -- Datasets -- //
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in the constructor.
    std::shared_ptr<DataSet> _dset_state;
    std::shared_ptr<DataSet> _dset_cluster_id;


    // -- Rule functions -- //
    // initialise all empty or all tree
    // update following the FFM set of rules

    /// Sets the given cell to state "empty"
    RuleFunc _set_initial_state_empty = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state.state = empty;

        return state;
    };

    /// Sets the given cell to state "tree" with probability p, else to "empty"
    RuleFunc _set_initial_density_tree = [this](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        std::uniform_real_distribution<> dist(0., 1.);

        // Set the internal variables
        if (dist(*this->_rng) < _initial_density)
        {
            state.state = tree;
        }
        else
        {
            state.state = empty;
        }

        return state;
    };

    RuleFunc _burn_cluster = [this](auto cell) {
        if constexpr (!ManagerType::Cell::is_sync()) {
            std::uniform_real_distribution<> dist(0., 1.);

            // burn cluster of trees
            // asynchronous update needed!
            std::vector<decltype(cell)> cluster = { cell };
            cell->state().state = empty;

            for (unsigned long i = 0; i < cluster.size(); ++i)
            {
                auto cluster_member = cluster[i];
                for (auto cluster_potential_member : 
                        Neighbor::neighbors(cluster_member,this->_manager))
                {
                    if (cluster_potential_member->state().state == tree &&
                        dist(*this->_rng) > _param.resistance)
                    {
                        cluster.push_back(cluster_potential_member);									
                        cluster_potential_member->state().state = empty;
                    }
                }
            }
        }
        else {
            (void)this;
        }

        return cell->state();
    };

    /// update follwoing set of rules
    /**    states: 0: empty, 1: tree (, 2: burning)
        * contagious disease spread (CDM)
        *    empty -> tree with probability growth_rate
        *    tree -> burning with probability lightning_frequency
        *    tree -> burning with probability 1 - resistance if i' in Neighborhood of i burning
        *    burning -> empty
        * Percolation spread (PM)
        *    empty -> tree with probability growth_rate
        *    tree -> burning with probability lightning_frequency
        *    or tree -> burning if connected to cluster -> empty instantaneously (two state FFM, percolation)
        */
    RuleFunc _update = [this](auto cell){
        auto state = cell->state();
        state.cluster_tag = -1; // reset

        std::uniform_real_distribution<> dist(0., 1.);
        
        // if state is empty, empty -> tree by growth
        if (state.state == empty && dist(*this->_rng) < _param.growth_rate) {
            state.state = tree;
        }

        // state is tree, tree -> burning by lighning or by burning neighbors
        else if (state.state == tree)
        {
            // tree -> burning by lightning
            // in PercolationM connecte cluster catches fire -> percolation
            if (dist(*this->_rng) < _param.lightning_frequency) {
                if (_model_feature.two_state_FFM) {
                    state = _burn_cluster(cell);
                }
                else {
                    state.state = burning;
                }
            }

            // ignite bottom row
            else if (_model_feature.light_bottom_row && cell->position()[1]==0.5) 
            {
                if (_model_feature.two_state_FFM) {
                    state = _burn_cluster(cell);
                }
                else {
                    state.state = burning;
                }
            }
                        
            // catch fire from Neighbors in CDM
            else if (!_model_feature.two_state_FFM) 
            {
                for (auto nb : Neighbor::neighbors(cell,this->_manager)) {
                    if (nb->state().state == burning && dist(*this->_rng) > _param.resistance) {
                        state.state = burning;
                    }
                }
            }
        }

        // stop burning, turn empty in CDM
        else if (state.state == burning) {
            state.state = empty;
        }

        return state;
    };

    RuleFunc _identify_cluster = [this](auto cell){
        if constexpr (!ManagerType::Cell::is_sync())
        {
            if (cell->state().cluster_tag == -1 && cell->state().state == tree) // else already labeled
            {
                std::vector<decltype(cell)> cluster = { cell };
                cell->state().cluster_tag = _cluster_tag_cnt;
                for (unsigned long i = 0; i < cluster.size(); ++i)
                {
                    auto cluster_member = cluster[i];
                    for (auto cluster_potential_member : Neighbor::neighbors(cluster_member,this->_manager))
                    {
                        if (cluster_potential_member->state().cluster_tag == -1 &&
                            cluster_potential_member->state().state == tree)
                        {
                            cluster_potential_member->state().cluster_tag = _cluster_tag_cnt;
                            cluster.push_back(cluster_potential_member);
                        }
                    }
                }
                _cluster_tag_cnt++;
            }
        }

        return cell->state();
    };



public:
    /// Construct the ForestFireModel model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    ForestFireModel (const std::string name,
                 ParentModel &parent,
                 ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),
        _param(as_double(this->_cfg["growth_rate"]),
            as_double(this->_cfg["lightning_frequency"]),
            as_double(this->_cfg["resistance"])
        ),
        _model_feature(Utopia::as_bool(this->_cfg["two_state_FFM"]),
            Utopia::as_bool(this->_cfg["light_bottom_row"])
        ),
        _initial_density(as_double(this->_cfg["initial_density"])),
        // temporary members
        _cluster_tag_cnt(0),
        // create datasets
        _dset_state(this->_hdfgrp->open_dataset("state")),
        _dset_cluster_id(this->_hdfgrp->open_dataset("cluster_id"))
    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Set the capacity of the datasets
        // We know the maximum number of steps (== #rows), and the number of
        // grid cells (== #columns); that is the final extend of the dataset.
        const hsize_t num_cells = std::distance(_manager.cells().begin(),
                                                _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                          this->get_time_max() + 1, num_cells);
        _dset_state->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_cluster_id->set_capacity({this->get_time_max() + 1, num_cells});
        // get cluster

        // Write initial state
        this->write_data();
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto initial_density = as_double(this->_cfg["initial_density"]);

        // Apply a rule to all cells depending on the config value
        if (initial_density == 0)
        {
            if constexpr (ManagerType::Cell::is_sync()) {
                apply_rule(_set_initial_state_empty, _manager.cells());
                apply_rule(_identify_cluster, _manager.cells());
            }
            else {
                apply_rule(_set_initial_state_empty, _manager.cells(), *this->_rng);
                apply_rule(_identify_cluster, _manager.cells(), *this->_rng);
            }
        }
        else if (initial_density > 0. && initial_density <= 1.)
        {
            if constexpr (ManagerType::Cell::is_sync()) {
                apply_rule(_set_initial_density_tree, _manager.cells());
                apply_rule(_identify_cluster, _manager.cells());
            }
            else {
                apply_rule(_set_initial_density_tree, _manager.cells(), *this->_rng);
                apply_rule(_identify_cluster, _manager.cells(), *this->_rng);
            }
        }
        else
        {
            throw std::runtime_error("The initial state is not valid! Must be value between 0 and 1");
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }


    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail Here you can add a detailed description what exactly happens 
     *          in a single iteration step
     */
    void perform_step ()
    {
        _cluster_tag_cnt = 0;
        if constexpr (ManagerType::Cell::is_sync()) {
            apply_rule(_update, _manager.cells());
            apply_rule(_identify_cluster, _manager.cells());
        }
        else {
            apply_rule(_update, _manager.cells(), *this->_rng);
            apply_rule(_identify_cluster, _manager.cells(), *this->_rng);
        }
    }


    /// Write data
    void write_data ()
    {   
        // state
        _dset_state->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return static_cast<unsigned short int>(cell->state().state);
                                });

        // cluster id
        _dset_cluster_id->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().cluster_tag;
                                });
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};

} // namespace ForestFireModel
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_FORESTFIREMODEL_HH
