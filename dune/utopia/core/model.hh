#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>


namespace Utopia {


/// Wrapper struct for defining base class data types
/** \tparam DataType Type of the data the model operates on (state)
 *  \tparam BoundaryConditionType Data type of the boundary condition
 */
template<typename DataType,
         typename BoundaryConditionType,
         typename RNGType=std::mt19937,
         typename ConfigType=Utopia::DataIO::Config,
         typename DataGroupType=Utopia::DataIO::HDFGroup
         >
struct ModelTypes
{
    using Data = DataType;
    using BCType = BoundaryConditionType;
    using RNG = RNGType;
    using Config = ConfigType;
    using DataGroup = DataGroupType;
};


/// Base class interface for Models using the CRT Pattern
/** \tparam Derived Type of the derived model class
 *  \tparam ModelTypes Convenience wrapper for extracting model data types
 */
template<class Derived, typename ModelTypes>
class Model
{
public:
    // -- Data types uses throughout the model class -- //
    // NOTE: these are also available to derived classes

    /// Data type of the state
    using Data = typename ModelTypes::Data;
    
    /// Data type of the boundary condition
    using BCType = typename ModelTypes::BCType;
    
    /// Data type that holds the configuration
    using Config = typename ModelTypes::Config;
    
    /// Data type that is used for writing data
    using DataGroup = typename ModelTypes::DataGroup;

    /// Data type of the shared RNG
    using RNG = typename ModelTypes::RNG;

protected:
    // -- Member declarations -- //
    /// Model internal time stamp
    unsigned int time;
    
    /// Name of the model instance
    const std::string name;

    /// Config node belonging to this model instance
    Config cfg;

    /// The HDF group this model instance should write its data to
    std::shared_ptr<DataGroup> hdfgrp;

    /// The RNG shared between models
    std::shared_ptr<RNG> rng;

public:
    // -- Constructors -- //

    /// Granular model constructor
    /** \detail creates an instance of model and extracts the relevant info
     *          from the passed arguments. This constructor should be used when
     *          initializing a model instance directly with all its parts.
     *
     *  \param name          The name of this model instance, ideally used only
     *                       once on the current hierarchical level
     *  \param parent_cfg    The parent config node. This node has to contain
     *                       a key that matches the name of the instance
     *  \param parent_group  The parent HDFGroup in which a new group with
     *                       the name of the model instance is created
     *  \param shared_rng    A pointer to the RNG object shared between models
     */
    Model (const std::string name,
           Config &parent_cfg,
           std::shared_ptr<DataGroup> parent_group,
           std::shared_ptr<RNG> shared_rng)
    :
        time(0),
        name(name),
        // extract the relevant config by instance name
        cfg(parent_cfg[this->name]),
        // create a group with the instance name
        hdfgrp(parent_group->open_group(this->name)),
        // the shared RNG can just be stored
        rng(shared_rng)
    { }


    /// Construct model with information from parent model
    /** \detail creates an instance of model using the passed reference to the
     *          parent model: extracts a 
     *
     *  \tparam ParentModel The parent model's type
     *
     *  \param name         The name of this model instance, ideally used only
     *                      once on the current hierarchical level
     *  \param parent_model The parent model object from which the
     *                      corresponding config node, the group, and the RNG
     *                      are extracted
     */
    template<class ParentModel>
    Model (const std::string name,
           ParentModel &parent_model)
    :
        time(0),
        name(name),
        //extract the other information from the parent model object
        cfg(parent_model.get_cfg()[this->name]),
        hdfgrp(parent_model.get_hdfgrp()->open_group(this->name)),
        rng(parent_model.get_rng())
    { }


    // -- Getters -- //

    /// Return the current time of this model
    unsigned int get_time() {
        return this->time;
    }

    /// Return the config node of this model
    Config get_cfg() {
        return this->cfg;
    }
    
    /// Return a pointer to the HDF group this model stores data in
    std::shared_ptr<DataGroup> get_hdfgrp() {
        return this->hdfgrp;
    }
    
    /// Return a pointer to the shared RNG
    std::shared_ptr<RNG> get_rng() {
        return this->rng;
    }


    // -- Default implementations -- //

    /// Iterate one (time) step of this model
    /** Increment time, perform step, then write data
     */
    void iterate () {
        // TODO add low-priority log messages here
        increment_time();
        perform_step();
        write_data();
    }


    // -- User-defined implementations -- //

    /// Perform the computation of a step
    void perform_step ()
    {
        impl().perform_step();
    }
    
    /// Write data
    void write_data () {
        impl().write_data();
    }

    /// Return const reference to stored data
    const Data& data () const {
        return impl().data();
    }
    
    /// Set model boundary condition
    void set_boundary_condition (const BCType& bc)
    {
        impl().set_boundary_condition(bc);
    }

    /// Set model initial condition
    void set_initial_condition (const Data& ic)
    {
        impl().set_initial_condition(ic);
    }

protected:
    /// Increment time
    /** \param dt Time increment
     */
    void increment_time (unsigned int dt=1) { time += dt; }

    /// cast to the derived class
    Derived& impl () { return static_cast<Derived&>(*this); }
    
    /// const cast to the derived interface
    const Derived& impl () const { return static_cast<const Derived&>(*this); }
};


/// A class to use at the top level of the model hierarchy as a mock parent
/** \detail It is especially useful when desiring to use the model constructor
 *          that uses a parent model.
 *
 *  \tparam RNG The RNG type to use 
 */
template<typename RNG=std::mt19937>
class PseudoParent
{
protected:
    using Config = Utopia::DataIO::Config;
    using DataFile = Utopia::DataIO::HDFFile;
    using DataGroup = Utopia::DataIO::HDFGroup;

    Config cfg;
    DataFile hdffile;
    std::shared_ptr<RNG> rng;

public:
    /// Constructor that only requires path to a config file
    PseudoParent (const std::string cfg_path)
    :
    // Initialize the config node from the path to the config file
    cfg{cfg_path},
    // Create a file at the specified output path
    hdffile{cfg["output_path"].as<std::string>(), "x"},
    // Initialize the RNG from a seed
    rng(std::make_shared<RNG>(cfg["seed"].as<int>()))
    {
        std::cout << "Initialized pseudo parent from config file:  "
                  << cfg_path << std::endl;
        // TODO add some informative log messages here
    }
    

    /// Constructor that allows granular control over config parameters
    PseudoParent (const std::string cfg_path,
                  const std::string output_path,
                  const int seed=42,
                  const std::string output_file_mode="a")
    :
    // Initialize the config node from the path to the config file
    cfg{cfg_path},
    // Create a file at the specified output path
    hdffile{output_path, output_file_mode},
    // Initialize the RNG from a seed
    rng(std::make_shared<RNG>(seed))
    {
        std::cout << "Initialized pseudo parent." << std::endl
                  << "  cfg_path:     " << cfg_path << std::endl
                  << "  output_path:  " << output_path
                  << "  (mode: " << output_file_mode << ")" << std::endl
                  << "  seed:         " << seed << std::endl;
        // TODO add some informative log messages here
    }


    // -- Getters -- //

    /// Return the config node of the Pseudo model, i.e. the root node
    Config get_cfg() {
        return this->cfg;
    }
    
    /// Return a pointer to the HDF file
    std::shared_ptr<DataFile> get_hdffile() {
        return this->hdffile;
    }
    
    /// Return a pointer to the HDF basegroup
    std::shared_ptr<DataGroup> get_hdfgrp() {
        return this->hdffile.get_basegroup();
    }
    
    /// Return a pointer to the RNG
    std::shared_ptr<RNG> get_rng() {
        return this->rng;
    }
};


} // namespace Utopia

#endif // UTOPIA_MODEL_HH
