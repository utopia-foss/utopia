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
    double F;
    double S() const { return 1 - G - T - F; };

    State() : 
        G(0.99),T(0.01),F(0)
    {}
    State(double g, double s, double t, double f = 0) : 
        G(g), T(t), F(f)
    {
        if (g+s+t+f > 1+1e-6 || g+s+t+f < 1-1e-6) {
            std::cout << "WARNING: initial state not a density: G+S+T must be 1.";
        }
    }
};

struct Param {
    const double alpha;
    const double beta;
    const double gamma;
    const double omega0;
    const double omega1;
    const double theta1;
    const double theta2;
    const double s1;
    const double s2;
    const double phi0;
    const double phi1;
    const double mu;
    const double nu;
    
    Param(double alpha, double beta, double gamma, 
        double omega0, double omega1, double theta1, double theta2, 
        double s1, double s2, double phi0, double phi1, double mu, double nu) 
    :
        alpha(alpha), beta(beta), gamma(gamma), 
        omega0(omega0), omega1(omega1), theta1(theta1), theta2(theta2), 
        s1(s1), s2(s2), phi0(phi0), phi1(phi1), mu(mu), nu(nu)
    { }
};

/// Boundary condition type
struct Boundary {};

double omega(const State& state, const Param param) {
    return param.omega0 + (param.omega1-param.omega0) /
        (1+exp(-(state.G+param.gamma*(state.S()+state.T)-param.theta1)/param.s1));
}

double phi(const State& state, const Param param) {
    return param.phi0 + (param.phi1-param.phi0) /
        (1+exp(-(state.G+param.gamma*(state.S()+state.T)-param.theta2)/param.s2));
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
    const Param _param;


    // -- Temporary objects -- //
    

    // -- Datasets -- //
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in the constructor.
    std::shared_ptr<DataSet> _dset_density_G;
    std::shared_ptr<DataSet> _dset_density_T;
    std::shared_ptr<DataSet> _dset_density_F;
    std::shared_ptr<DataSet> _dset_pos_x;
    std::shared_ptr<DataSet> _dset_pos_y;


    // -- Rule functions -- //
    // Define functions that can be applied to the cells of the grid
    // NOTE The below are examples; delete and/or adjust them to your needs!

    // -- Initialisation Rules -- //

    /// Sets the given cell to state "G" with small perturbation on T
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_G = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = State(0.99,0.,0.01,0.);

        return state;
    };

    /// Sets the given cell to state "S"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_S = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = State(0.,1.,0.,0.);

        return state;
    };


    /// Sets the given cell to state "T"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_T = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = State(0.,0.,1.,0.);

        return state;
    };

     /// creats a map of different equally distributed initial states
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_spatial = [this](const auto cell) {
        auto state = cell->state();
        auto position = cell->position();
        double size_x = as_double(this->_cfg["grid_size"][0]);
        double size_y = as_double(this->_cfg["grid_size"][1]);

        double G = position[0]/size_x;
        double T = position[1]/size_y;
        double S = 1 - G - T;
        if (S < 0) {
            G = 0.99;
            T = 0.01;
            S = 0;
        }

        state = State(G,S,T);

        return state;
    };

    /// Sets the given cell to random state
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

    // -- Update Rule -- //

    /// Define the update rule for a cell
    std::function<State(std::shared_ptr<CellType>)> _update = [this](const auto cell){
        // Here, you c[an write some update rule description

        // Get the state of the cell
        auto state = cell->state();

        // G ̇ =μS + νT − βGT + φ􏰀(G+γ(1−G−F)􏰁)*F − αGF
        double dG = _param.mu*state.S() + _param.nu*state.T - _param.beta*state.G*state.T 
                    + phi(state, _param)*state.F - _param.alpha*state.G*state.F;
        // T ̇ = ω(􏰀G +γ(1−G −F)􏰁)*S − νT − αTF
		double dT = omega(state, _param)*state.S() - _param.nu*state.T 
                    - _param.alpha*state.T*state.F;
        // F ̇ = [􏰀α(1−F) − φ􏰀(G+γ(1−G −F))]􏰁􏰁F
        double dF = (_param.alpha*(1-state.F) - phi(state, _param))*state.F;
		state.G = state.G + dG*_dt;
		state.T = state.T + dT*_dt;
        state.F = state.F + dF*_dt;

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
        
        _param(as_double(this->_cfg["alpha"]), as_double(this->_cfg["beta"]),
            as_double(this->_cfg["gamma"]), as_double(this->_cfg["omega_0"]),
            as_double(this->_cfg["omega_1"]), as_double(this->_cfg["theta_1"]),
            as_double(this->_cfg["theta_2"]), as_double(this->_cfg["s_2"]),
            as_double(this->_cfg["s_2"]), as_double(this->_cfg["phi_0"]),
            as_double(this->_cfg["phi_1"]), as_double(this->_cfg["mu"]),
            as_double(this->_cfg["nu"])
        ),
        // create datasets
        _dset_density_G(this->_hdfgrp->open_dataset("density_G")),
        _dset_density_T(this->_hdfgrp->open_dataset("density_T")),
        _dset_density_F(this->_hdfgrp->open_dataset("density_F")),
        _dset_pos_x(this->_hdfgrp->open_dataset("position_x")),
        _dset_pos_y(this->_hdfgrp->open_dataset("position_y"))     
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
        _dset_density_T->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_density_F->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_pos_x->set_capacity({1, num_cells});
        _dset_pos_y->set_capacity({1, num_cells});
        _dset_pos_x->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->position()[0];
                                });
        _dset_pos_y->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->position()[1];
                                });

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
        else if (initial_state == "init_spatial")
        {
            apply_rule(_set_initial_state_spatial, _manager.cells());
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
        // Tree
        _dset_density_T->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().T;
                                });
        // Forest
        _dset_density_F->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().F;
                                });
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};

} // namespace Savanna
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_SAVANNA_HH
