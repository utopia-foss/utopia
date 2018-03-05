#ifndef YAML_FILE_HH
#define YAML_FILE_HH

#include "yaml-cpp/yaml.h"


namespace Utopia
{
    
namespace DataIO
{

class YamlFile
{
protected:
    std::string _filepath;
    YAML::Node _config;

public:
    YamlFile() = default;

    YamlFile(std::string filepath)
    {
        _filepath = filepath;
        _config = YAML::LoadFile(filepath);
    }

    YamlFile(std::string filepath, YAML::Node config)
    {
        _filepath = filepath;
        _config = config;
    }

    YamlFile(const YamlFile& yaml_file) :   _filepath(yaml_file.get_filepath()), 
                                            _config(yaml_file.get_config()) 
                                            {};

    YamlFile(YamlFile&& yaml_file) :    _filepath(std::move(yaml_file._filepath)), 
                                        _config(std::move(yaml_file._config)) 
                                        {};

    YAML::Node get_config()
    {
        return _config;
    }

    void set_config(YAML::Node config)
    {
        _config = config;
    }

    // void set_filepath(const std::string filepath)
    // {
    //     _filepath = filepath;
    // }

    std::string get_filepath()
    {
        return _filepath;
    }

    friend void swap(YamlFile& yaml_file1, YamlFile& yaml_file2)
    {
        using std::swap;

        swap(yaml_file1._filepath, yaml_file2._filepath);
        swap(yaml_file1._config, yaml_file2._config);
    }

    operator=(YamlFile yaml_file)
    {

        swap(*this, yaml_file);

        return *this;
    }

    operator[](std::string name)
    {
        return YamlFile(_filepath, _config[name]);
    }

    template <typename T>
    void as(T type)
    {
        return _config.as<type>();
    }

    virtual ~YamlFile() = default;

}; // class YamlFile




} // namespace DataIO 

} // namespace Utopia

#endif // YAML_FILE_HH
