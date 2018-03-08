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


/// An object representing a HDFGroup 
/** 
 * 
*/
class HDFGroup
{
protected:
    hid_t _group; /// Storage of group id
    std::string _path; /// Storage of the path
    std::unordered_map<std::string, std::shared_ptr<HDFGroup>> _open_groups; /// Storage of child groups and their path
    std::unordered_map<std::string, std::shared_ptr<HDFGroup>> _open_datasets; /// Storage of child datasets and their path

public:
    void info()
    {
        // TODO
        // H5Gget_info(_group);
    }

    /// Get the id
    /**
    *  \return The hdf5 id
    */
    hid_t get_id()
    {
        return _group;
    }

    /// Get the path
    /**
    *  \return The hdf5 path
    */
    std::string get_path()
    {
        return _path;
    }


    /// Add an attribute to the HDFGroup object
    /** Adds an attribute to a group
    *  \tparam Attrdata Attribute type 
    *  \param name Attribute name
    *  \param attribute_data Attribute data
    */
    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data) {

        HDFAttribute<HDFGroup, Attrdata> attribute(
            *this, std::forward<std::string &&>(name));

        attribute.write(attribute_data);
    }

    /// Get open groups 
    /** Adds an attribute to a group
    *  return open_groups A vector containing std::shared_pointer pointing at the HDFGroup objects
    */
    std::vector<std::shared_ptr<HDFGroup>> get_open_groups()
    {
        std::vector<std::shared_ptr<HDFGroup>> open_groups;
        for (auto group : _open_groups)
        {
            open_groups.push_back(group.second);
        }
        return open_groups;
    }

    /// Open a new HDFGroup
    /**
    * \param path The path
    * \return A pointer to the newly created HDFGroup
    */
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

    /// Close a group
    /** Closes a child group if it is a child. 
    * \param path The path
    */
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

    /// Close the group if there are no more child groups or datasets
    void close() 
    {         
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

    /// Swap two HDFGroup objects
    /**
    *  \param group1 First HDFGroup object
    *  \param group2 Second HDFGroup object
    */
    friend void swap(HDFGroup& group1, HDFGroup& group2)
    {
        using std::swap;

        swap(group1._group, group2._group);
        swap(group1._path, group2._path);
        swap(group1._open_groups, group2._open_groups);
        swap(group1._open_datasets, group2._open_datasets);
    }

    /// Default constructor
    HDFGroup() = default;

    /// Copy constructor
    /** Constructs an HDFGroup object by copying another HDFGroup object
    *  \param group The HDFGroup object that is copied
    */
    HDFGroup(const HDFGroup& group) :   _group(group._group),
                                        _path(group._path),
                                        _open_groups(group._open_groups),
                                        _open_datasets(group._open_datasets)
    {};

    /// Move constructor
    /** 
    * \param group The rvalue reference
    */
    HDFGroup(HDFGroup&& group):     _group(std::move(group._group)),
                                    _path(std::move(group._path)),
                                    _open_groups(std::move(group._open_groups)),
                                    _open_datasets(std::move(group._open_datasets))
    {};

    /// Assign a HDFGroup object using an assignment operator
    /** Implement assignment operation using the copy-and-swap idiom
    *
    *  \return the copied HDFGroup object
    */ 
    HDFGroup& operator=(HDFGroup group)
    {
        swap(group, *this);
        return *this;
    }
    
    /// Constructor
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

    /// Destructor
    virtual ~HDFGroup()
    {
        if (H5Iis_valid(_group) == 0) { 
            // H5Gclose(_group);
            close();
        }
    }

}; // class HDFGroup

} // namespace DataIO 

} // namespace Utopia

#endif // HDFGROUP_HH
