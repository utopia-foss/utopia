#ifndef HDFGROUP_HH
#define HDFGROUP_HH

#include <string>
#include <sstream>
#include <unordered_map>

#include <hdf5.h>
#include <hdf5_hl.h>
#include "hdfmockclasses.hh"

namespace Utopia
{
    
namespace DataIO
{

class HDFFile;

/**
 *
*/
class HDFGroup
{
protected:
    hid_t _group;
    std::string _path;
    std::unordered_map<std::string, std::shared_ptr<HDFGroup>> _open_groups;
    std::unordered_map<std::string, std::shared_ptr<HDFGroup>> _open_datasets;

public:
    void info()
    {
        // TODO
        // H5Gget_info(_group);
    }

    hid_t get_id()
    {
        return _group;
    }

    std::string get_path()
    {
        return _path;
    }


    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data) {

        HDFAttribute<HDFGroup, Attrdata> attribute(
            *this, std::forward<std::string &&>(name));

        attribute.write(attribute_data);
    }

    std::vector<std::shared_ptr<HDFGroup>> get_open_groups()
    {
        std::vector<std::shared_ptr<HDFGroup>> open_groups;
        for (auto group : _open_groups)
        {
            open_groups.push_back(group.second);
        }
        return open_groups;
    }

    std::shared_ptr<HDFGroup> open_group(std::string path)
    {
        auto found = _open_groups.find(path);
        // Assure that the group is not yet open
        if (found == _open_groups.end())
        {
            auto it = _open_groups.insert( 
                std::make_pair(path, std::make_shared<HDFGroup>(HDFGroup(*this, path))));
            return it.first->second;
        }
    }

    void close_group(std::string path)
    {
        auto found = _open_groups.find(path);
        if (found != _open_groups.end())
        {
            _open_groups.erase(found);
        }
        else
        {
            throw std::runtime_error("Trying to delete a nonexistant or closed group!");
        }

    }

    void close() 
    { 
        // TODO close only if 
        if (_open_groups.size() == 0 && _open_datasets.size() == 0)
        {
            H5Gclose(_group); 
        }
        else
        {
            throw std::runtime_error("The group still contains child groups or datasets");
        }
    }



    void open_dataset(std::string path)
    {
        // TODO
    }

    void close_dataset(std::string path)
    {
        // TODO
    }

    friend void swap(HDFGroup& group1, HDFGroup& group2)
    {
        using std::swap;

        swap(group1._group, group2._group);
        swap(group1._path, group2._path);
        swap(group1._open_groups, group2._open_groups);
        swap(group1._open_datasets, group2._open_datasets);
    }

    // default constructor
    HDFGroup() = default;

    // copy constructor
    HDFGroup(const HDFGroup& group) :   _group(group._group),
                                        _path(group._path),
                                        _open_groups(group._open_groups),
                                        _open_datasets(group._open_datasets)
    {};

    // move constructor
    HDFGroup(HDFGroup&& group):     _group(std::move(group._group)),
                                    _path(std::move(group._path)),
                                    _open_groups(std::move(group._open_groups)),
                                    _open_datasets(std::move(group._open_datasets))
    {};

    HDFGroup& operator=(HDFGroup group)
    {
        swap(group, *this);
        return *this;
    }
    
    // constructor
    template <typename Object>
    HDFGroup(Object &object, std::string name) : _path(name) 
    {
        if (std::is_same<Object, HDFFile>::value) 
        {
            if (_path == "/") 
            {
                _group = H5Gopen(object.get_id(), "/", H5P_DEFAULT);
            } else 
            {
                if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) == 1) 
                {
                    _group =
                        H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
                } else 
                {
                    _group = H5Gcreate(object.get_id(), _path.c_str(),
                                       H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                }
            }
        } else if (std::is_same<Object, HDFGroup>::value) 
        {
            if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) == 1) 
            {
                _group = H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
            } else 
            {
                _group = H5Gcreate(object.get_id(), _path.c_str(), H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT);
            }
        }
    }

    virtual ~HDFGroup()
    {
        if (H5Iis_valid(_group) == 0) { 
            H5Gclose(_group);
        }
    }

}; // class HDFGroup




} // namespace DataIO 

} // namespace Utopia

#endif // HDFGROUP_HH
