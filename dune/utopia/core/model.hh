#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/types.hh>
#include <dune/utopia/core/types.hh>
#include <yaml-cpp/yaml.h>

namespace Utopia {


/// Wrapper struct for defining base class data types
/** \tparam DataType Type of the data the model operates on (state)
 *  \tparam BoundaryConditionType Data type of the boundary condition
 */
template<typename DataType,
         typename BoundaryConditionType,
         typename RNGType=std::mt19937,
         typename ConfigType=Utopia::DataIO::Config,
         typename DataGroupType=Utopia::DataIO::HDFGroup,
         typename DataSetType=Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>
         >
struct ModelTypes
{
    using Data = DataType;
    using BCType = BoundaryConditionType;
    using RNG = RNGType;
    using Config = ConfigType;
    using DataGroup = DataGroupType;
    using DataSet = DataSetType;
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
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename ModelTypes::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename ModelTypes::DataSet;

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
    const std::shared_ptr<DataGroup> hdfgrp;

    /// The RNG shared between models
    const std::shared_ptr<RNG> rng;

public:
    // -- Constructor -- //

    /// Constructor
    /** \detail Uses information from a parent model to create an instance of
     *          this model.
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
           const ParentModel &parent_model)
    :
        time(0),
        name(name),
        //extract the other information from the parent model object
        cfg(parent_model.get_cfg()[this->name]),
        hdfgrp(parent_model.get_hdfgrp()->open_group(this->name)),
        rng(parent_model.get_rng())
    {
        // TODO add informative log messages here
    }


    // -- Getters -- //

    /// Return the current time of this model
    unsigned int get_time() const {
        return this->time;
    }

    /// Return the config node of this model
    Config get_cfg() const {
        return this->cfg;
    }
    
    /// Return a pointer to the HDF group this model stores data in
    std::shared_ptr<DataGroup> get_hdfgrp() const {
        return this->hdfgrp;
    }
    
    /// Return a pointer to the shared RNG
    std::shared_ptr<RNG> get_rng() const {
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
/** \detail It is especially useful when initializing a top-level model as then
 *          the model constructor that expects a Model-class-like object can
 *          be used.
 *          This class also takes care to load and hold a configuration file,
 *          to create a HDFFile for output, and to initialize a shared RNG. A
 *          template parameter exists that allows customization of the RNG
 *          class.
 *
 *  \tparam RNG The RNG type to use
 */
template<typename RNG=std::mt19937>
class PseudoParent
{
protected:
    // Convenience type definitions
    using Config = Utopia::DataIO::Config;
    using HDFFile = Utopia::DataIO::HDFFile;
    using HDFGroup = Utopia::DataIO::HDFGroup;

    /// The config node
    Config cfg;

    /// Pointer to the HDF5 file where data is written to
    const std::shared_ptr<HDFFile> hdffile;

    /// Pointer to a RNG that can be shared between models
    const std::shared_ptr<RNG> rng;

public:
    /// Constructor that only requires path to a config file
    /** \detail From the config file, all necessary information is extracted,
     *          i.e.: the path to the output file ('output_path') and the seed
     *          of the shared RNG ('seed'). These keys have to be located at
     *          the top level of the configuration file.
     *
     *  \param cfg_path The path to the YAML-formatted configuration file
     */
    PseudoParent (const std::string cfg_path)
    :
    // Initialize the config node from the path to the config file
    cfg(YAML::LoadFile(cfg_path)),
    // Create a file at the specified output path and store the shared pointer
    hdffile(std::make_shared<HDFFile>(cfg["output_path"].template as<std::string>(), "w")),
    // Initialize the RNG from a seed
    rng(std::make_shared<RNG>(cfg["seed"].template as<int>()))
    {
        std::cout << "Initialized pseudo parent from config file:  "
                  << cfg_path << std::endl;
        // TODO add some informative log messages here
    }
    

    /// Constructor that allows granular control over config parameters
    /**
     *  \param cfg_path The path to the YAML-formatted configuration file
     *  \param output_path Where the HDF5 file is to be located
     *  \param seed The seed the RNG is initialized with (default: 42)
     *  \param output_file_mode The access mode of the HDF5 file (default: w)
     */
    PseudoParent (const std::string cfg_path,
                  const std::string output_path,
                  const int seed=42,
                  const std::string output_file_mode="w")
    :
    // Initialize the config node from the path to the config file
    cfg(YAML::LoadFile(cfg_path)),
    // Create a file at the specified output path
    hdffile(std::make_shared<HDFFile>(output_path, output_file_mode)),
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
    Config get_cfg() const {
        return this->cfg;
    }

    /// Return a pointer to the HDF data file
    std::shared_ptr<HDFFile> get_hdffile() const {
        return this->hdffile;
    }
    
    /// Return a pointer to the HDF basegroup
    std::shared_ptr<HDFGroup> get_hdfgrp() const {
        return this->hdffile->get_basegroup();
    }
    
    /// Return a pointer to the RNG
    std::shared_ptr<RNG> get_rng() const {
        return this->rng;
    }
};


} // namespace Utopia

#endif // UTOPIA_MODEL_HH
