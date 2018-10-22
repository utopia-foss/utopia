#ifndef UTOPIA_MODELS_FORESTFIRE_HH
#define UTOPIA_MODELS_FORESTFIRE_HH

#include <functional>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>


namespace Utopia {
namespace Models {
namespace ForestFire {

enum StateEnum { empty=0, tree=1 };

struct State {
    StateEnum state;
    unsigned int cluster_tag;

    State() :
        state(empty),  cluster_tag(0)
    { }
    State(StateEnum s) :
        state(s),  cluster_tag(0)
    { }
};

struct Param {
    const double growth_rate;
    const double lightning_frequency;
    const bool light_bottom_row;
    const double resistance;

    Param(double growth_rate, double lightning_frequency, bool light_bottom_row, double resistance)
    :
        growth_rate(growth_rate), lightning_frequency(lightning_frequency),
        light_bottom_row(light_bottom_row), resistance(resistance)
    {
        if (growth_rate > 1 || growth_rate < 0)
        {
            throw std::invalid_argument("growth rate is a probability per cell. "\
                                        "Should have value in [0,1]! "\
                                        "1 corresponds to empty turns to tree in 1 step. "\
                                        "0.1 every 10th step. 0 never. ");
        }
        if (lightning_frequency > 1 || lightning_frequency < 0)
        {
            throw std::invalid_argument("lightning frequency is a probability per cell. "\
                                        "Should have value in [0,1]! "\
                                        "1 corresponds to tree hit by lightning in one step. "\
                                        "0.1 every 10th step. 0 never. ");
        }
        if (resistance > 1 || resistance < 0)
        {
            throw std::invalid_argument("Resistance is a probability per burning neighbor. " \
                                        "Should have value in [0,1]! " \
                                        "0 corresponds to no resistance to fire. "\
                                        "1 to total resistance.");
        }
    }
};

/// Typehelper to define data types of ForestFire model
using ForestFireTypes = ModelTypes<>;


/// The ForestFire model
/** The ForestFire model simulates the development of a forest under influence of forest fires. 
 *  Trees grow on a random basis and fires cause their death
 *  for a whole cluster instantaneously (two state model).
 */
template<class ManagerType>
class ForestFire:
    public Model<ForestFire<ManagerType>, ForestFireTypes>
{
public:
    /// The base model type
    using Base = Model<ForestFire<ManagerType>, ForestFireTypes>;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Rule function type
    using RuleFunc = typename std::function<State(std::shared_ptr<CellType>)>;

    // Alias the neighborhood classes to make access more convenient
    using Neighbor = Neighborhoods::MooreNeighbor;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// Model parameters
    const Param _param;
    double _initial_density;      // density of trees

    // -- Temporary objects -- //
    unsigned int _cluster_tag_cnt;

    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_state;
    std::shared_ptr<DataSet> _dset_cluster_id;

    // -- Helper functions // 
    std::function<double()> _calculate_density = [this]() {
        double density = 0;
        for (const auto& cell : this->_manager.cells()) {
            if (cell->state().state == tree) {
                density += 1;
            }
        }
        density /= double( std::distance(this->_manager.cells().begin(), 
                                        this->_manager.cells().end()) );
        return density;
    };


    // -- Rule functions -- //
    // initialise trees random with density
    // update following the FFM set of rules

    /// Sets the given cell to state "tree" with probability p, else to "empty"
    RuleFunc _set_initial_state = [this](const auto& cell){
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
        std::uniform_real_distribution<> dist(0., 1.);

        // burn cluster of trees
        // asynchronous update needed!
        std::vector<decltype(cell)> cluster = { cell };
        cell->state().state = empty;

        for (unsigned int i = 0; i < cluster.size(); ++i)
        {
            auto cluster_member = cluster[i];
            for (auto&& cluster_potential_member : 
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

        return cell->state();
    };

    /// update follwoing set of rules
    /** states: 0: empty, 1: tree
     * Percolation spread (PM)
     *    empty -> tree with probability growth_rate
     *    tree -> burning with probability lightning_frequency
     *    or tree -> burning if connected to cluster -> empty instantaneously (two state FFM, percolation)
     */
    RuleFunc _update = [this](auto cell){
        auto state = cell->state();
        state.cluster_tag = 0; // reset

        std::uniform_real_distribution<> dist(0., 1.);
        
        // if state is empty, empty -> tree by growth
        if (state.state == empty && dist(*this->_rng) < _param.growth_rate) {
            state.state = tree;
        }

        // state is tree, tree -> burning by lighning or by burning neighbors
        else if (state.state == tree)
        {
            // tree -> burning by lightning
            // in Percolation connected cluster catches fire -> percolation
            if (dist(*this->_rng) < _param.lightning_frequency) {
                state = _burn_cluster(cell);
            }

            // ignite bottom row
            else if (_param.light_bottom_row && cell->position()[1]==0.5) 
            {
                state = _burn_cluster(cell);
            }
        }

        return state;
    };

    /// identify cluster
    /* get the identity of each cluster of trees, -1 for grass
     * run a percolation on a cell, that has no id
     * give all cells of that percolation the same id
     * _cluster_tag_cnt keeps track of given ids
     */
    RuleFunc _identify_cluster = [this](auto cell){
        if (cell->state().cluster_tag == 0 && cell->state().state == tree) // else already labeled
        {
            _cluster_tag_cnt++;

            std::vector<decltype(cell)> cluster = { cell };
            cell->state().cluster_tag = _cluster_tag_cnt;
            for (unsigned int i = 0; i < cluster.size(); ++i)
            {
                auto cluster_member = cluster[i];
                for (auto&& cluster_potential_member : Neighbor::neighbors(cluster_member,this->_manager))
                {
                    if (cluster_potential_member->state().cluster_tag == 0 &&
                        cluster_potential_member->state().state == tree)
                    {
                        cluster_potential_member->state().cluster_tag = _cluster_tag_cnt;
                        cluster.push_back(cluster_potential_member);
                    }
                }
            }
        }

        return cell->state();
    };



public:
    /// Construct the ForestFire model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    ForestFire (const std::string name,
                     ParentModel &parent,
                     ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),

        // Now initialize members specific to this class
        _manager(manager),
        _param(as_double(this->_cfg["growth_rate"]),
               as_double(this->_cfg["lightning_frequency"]),
               as_bool(this->_cfg["light_bottom_row"]),
               as_double(this->_cfg["resistance"])
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

        // Write initial state
        this->write_data();
    }


    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // check user config
        if (_initial_density < 0. || _initial_density > 1.)
        {
            throw std::invalid_argument("The initial state is not valid! Must be value between 0 and 1");
        }

        // Apply a rule to all cells depending on the config value
        else 
        {
            apply_rule(_set_initial_state, _manager.cells(), *this->_rng);
            apply_rule(_identify_cluster, _manager.cells(), *this->_rng);
        }
        
        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }


    // Runtime functions ......................................................

    /// Perform step
    void perform_step ()
    {   
        // reset tmp counter for cluster_id
        _cluster_tag_cnt = 0;

        /// apply rules: update and identify cluster
        apply_rule(_update, _manager.cells(), *this->_rng);
        apply_rule(_identify_cluster, _manager.cells(), *this->_rng);
    }

    /// Monitor model information
    void monitor ()
    {
        double density = _calculate_density();
        this->_monitor.set_by_value("tree_density", density);
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
};

} // namespace ForestFire
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_FORESTFIRE_HH
