#ifndef UTOPIA_MODELS_SAVANNA_HH
#define UTOPIA_MODELS_SAVANNA_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>


namespace Utopia {
namespace Models {
namespace Savanna {

/// Enum that will be part of the internal state of a cell
// enum Density : unsigned short int {G, S, T}; // Grass, Sapplings, Trees

/// State struct for Savanna model.
struct State {
    double G;
    double T;
    double S() const { return 1 - G - T; };

    State() : G(1),T(0) 
    {}
    State(double g, double s, double t) : G(g), T(t)
    {
        if (g+s+t > 1+1e-6 || g+s+t < 1-1e-6) {
            std::cout << "WARNING: initial state not a density: G+S+T must be 1.";
        }
    }
};

/// Boundary condition type
struct Boundary {};

double omega(const State& state, const double gamma, const double omega0, const double omega1, const double theta1, const double s1) {
    return omega0 + (omega1-omega0)/(1+exp(-(state.G+gamma*(state.S()+state.T)-theta1)/s1));
}


/// Typehelper to define data types of Savanna model 
using SavannaModelTypes = ModelTypes<State, Boundary>;
// NOTE if you do not use the boundary condition type, you can delete the
//      definition of the struct above and the passing to the type helper


/// The Savanna Model
/** Add your model description here.
 *  This model's only right to exist is to be a template for new models. 
 *  That means its functionality is based on nonsense but it shows how 
 *  actually useful functionality could be implemented.
 */
template<class ManagerType>
class SavannaModel:
    public Model<SavannaModel<ManagerType>, SavannaModelTypes>
{
public:
    /// The base model type
    using Base = Model<SavannaModel<ManagerType>, SavannaModelTypes>;
    
    /// Data type of the state
    using Data = typename Base::Data;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

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
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// A model parameter I need
    const double _dt;

    const double _alpha;
    const double _beta;
    const double _gamma;
    const double _omega0;
    const double _omega1;
    const double _theta1;
    const double _theta2;
    const double _s1;
    const double _s2;
    const double _phi0;
    const double _phi1;
    const double _mu;
    const double _nu;


    // -- Temporary objects -- //
    

    // -- Datasets -- //
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in the constructor.
    std::shared_ptr<DataSet> _dset_density_G;
    std::shared_ptr<DataSet> _dset_density_S;
    std::shared_ptr<DataSet> _dset_density_T;


    // -- Rule functions -- //
    // Define functions that can be applied to the cells of the grid
    // NOTE The below are examples; delete and/or adjust them to your needs!

    /// Sets the given cell to state "A"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_G = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = State(0.99,0.,0.01);

        return state;
    };

    /// Sets the given cell to state "B"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_S = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = State(0.,1.,0.);

        return state;
    };


    /// Sets the given cell to state "B"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_T = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = State(0.,0.,1.);

        return state;
    };


    /// Sets the given cell to state "A"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_rand = [this](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();
        std::uniform_real_distribution<> dist(0., 1.);
        double tmp = dist(*this->_rng);
        double G,S,T;
        if (tmp<0.4) 
        {
            G = dist(*this->_rng);
            std::uniform_real_distribution<> dist_adapt(0, 1.-G);
            T = dist_adapt(*this->_rng);
            S = 1-G-T;
        }
        else if (tmp < 0.8) 
        {
            T = dist(*this->_rng);
            std::uniform_real_distribution<> dist_adapt(0, 1.-T);
            G = dist_adapt(*this->_rng);
            S = 1-G-T;
        }
        else 
        {
            S = dist(*this->_rng);
            std::uniform_real_distribution<> dist_adapt(0, 1.-S);
            G = dist_adapt(*this->_rng);
            T = 1-G-S;
        }

        // Set the internal variables
        state = State(G,S,T);

        return state;
    };


    /// Define the update rule for a cell
    std::function<State(std::shared_ptr<CellType>)> _update = [this](const auto cell){
        // Here, you c[an write some update rule description

        // Get the state of the cell
        auto state = cell->state();

        double dG = _mu*state.S() + _nu*state.T - _beta*state.G*state.T;
		double dT = omega(state, _gamma, _omega0, _omega1, _theta1, _s1)*state.S()-_nu*state.T;
		state.G = state.G+dG*_dt;
		state.T = state.T+dT*_dt;

        // Return the new state cell
        return state;
    };


public:
    /// Construct the Savanna model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    SavannaModel (const std::string name,
                 ParentModel &parent,
                 ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),
        _dt(as_double(this->_cfg["dt"])),
        _alpha(as_double(this->_cfg["alpha"])),
        _beta(as_double(this->_cfg["beta"])),
        _gamma(as_double(this->_cfg["gamma"])),
        _omega0(as_double(this->_cfg["omega_0"])),
        _omega1(as_double(this->_cfg["omega_1"])),
        _theta1(as_double(this->_cfg["theta_1"])),
        _theta2(as_double(this->_cfg["theta_2"])),
        _s1(as_double(this->_cfg["s_2"])),
        _s2(as_double(this->_cfg["s_2"])),
        _phi0(as_double(this->_cfg["phi_0"])),
        _phi1(as_double(this->_cfg["phi_1"])),
        _mu(as_double(this->_cfg["mu"])),
        _nu(as_double(this->_cfg["nu"])),
        // create datasets
        _dset_density_G(this->_hdfgrp->open_dataset("density_G")),
        _dset_density_S(this->_hdfgrp->open_dataset("density_S")),
        _dset_density_T(this->_hdfgrp->open_dataset("density_T"))     
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
        _dset_density_G->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_density_S->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_density_T->set_capacity({this->get_time_max() + 1, num_cells});

        // Write initial state
        this->write_data();
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto initial_state = as_str(this->_cfg["initial_state"]);

        // Apply a rule to all cells depending on the config value
        if (initial_state == "init_Grass")
        {
            apply_rule(_set_initial_state_G, _manager.cells());
        }
        else if (initial_state == "init_Trees")
        {
            apply_rule(_set_initial_state_T, _manager.cells());
        }
        else if (initial_state == "init_random")
        {
            apply_rule(_set_initial_state_rand, _manager.cells());
        }
        else
        {
            throw std::runtime_error("The initial state is not valid!");
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
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_update, _manager.cells());
    }


    /// Write data
    void write_data ()
    {   
        // Grass
        _dset_density_G->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().G;
                                });

        // Savanna
        _dset_density_S->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().S();
                                });
        // Tree
        _dset_density_T->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().T;
                                });
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};

} // namespace Savanna
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_SAVANNA_HH
