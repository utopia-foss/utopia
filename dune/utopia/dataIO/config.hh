#ifndef CONFIG_HH
#define CONFIG_HH

#include "yaml-cpp/yaml.h"

namespace Utopia
{
    
namespace DataIO
{

/// This class manages loading YAML configuration files and accessing the configuration parameters
/** 
 * 
 */
class Config
{
protected:
    std::string _filepath; /// filepath storage
    YAML::Node _yaml_config; /// YAML::Node configuration storage

    /// Construct a new Config object
    /** The YAML configuration file is read in from the given filepath
     *  
     *  \param filepath Path to the YAML configuration file. This should correspond to the location of the YAML configuration
     *  \param yaml_yaml_config Configuration as a YAML::Node type
     *  \return Config object
     */
    Config(std::string filepath, YAML::Node yaml_config)
    {
        _filepath = filepath;
        _yaml_config = yaml_config;
    }

public:
    /// Default constructor
    Config() = default;

    /// Construct a new Config object
    /** The YAML configuration file is read in from the given filepath
     *  
     *  \param filepath Path to the YAML configuration file
     *  \return Config object
     */
    Config(std::string filepath)
    {
        _filepath = filepath;
        _yaml_config = YAML::LoadFile(filepath);
    }

    /// Copy constructor
    Config(const Config& config) :  _filepath(config._filepath), 
                                    _yaml_config(config._yaml_config) 
                                    {};
    
    /// Copy constructor
    Config(Config&& config) :       _filepath(std::move(config._filepath)), 
                                    _yaml_config(std::move(config._yaml_config)) 
                                    {};

    /// Get the underlying YAML configuration
    /**
    *  \return The configuration as YAML::Node
    */
    YAML::Node get_yaml_config()
    {
        return _yaml_config;
    }

    /// Get the filepath
    /**
    *  \return The filepath of the original YAML file
    */
    std::string get_filepath()
    {
        return _filepath;
    }

    /// Swap two Config objects
    /**
    *  \param config1 First Config object
    *  \param config2 Second Config object
    */
    friend void swap(Config& config1, Config& config2)
    {
        using std::swap;

        swap(config1._filepath, config2._filepath);
        swap(config1._yaml_config, config2._yaml_config);
    }

    /// Assign a Config object using an assignment operator
    /** Implement assignment operation using the copy-and-swap idiom
    *
    *  \return the copied Config object
    */
    Config& operator=(Config& config)
    {
        swap(*this, config);
        return *this;
    }

    /// Get a parameter subset of the Config object
    /** Access a hierarchically lower configuration lewel using the bracket operators. 
    *  The subset of parameters is provided corresponding to the provided keyword.
    *  In the case of no additional hierarchical levels access the parameter provided by the keyword.
    *  
    *  \param keyword Keyword accessing a lower hierarchical configuration level or a configuration paramter.
    *  \return Config object
    */ 
    Config operator[](std::string keyword)
    {
        Config cfg(_filepath, _yaml_config[keyword]);
        return cfg;
    }

    /// Provide a parameter type and return the parameter
    /** 
    *  
    *  \tparam T Return type of the input parameter
    *  \return Parameter in the provided type
    */
    template <typename T>
    T as()
    {
        return _yaml_config.as<T>();
    }

    /// Destructor
    virtual ~Config() = default;

}; // class Config

} // namespace DataIO 

} // namespace Utopia

#endif // CONFIG_HH
