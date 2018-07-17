/**
 * @brief This file provides a class for creating and managing groups in a
 *        HDF5 file, which then can create other objects (groups and datasets)
 *        inside of them.
 *
 * @file hdfgroup.hh
 */
#ifndef HDFGROUP_HH
#define HDFGROUP_HH

#include "hdfattribute.hh"
#include "hdfdataset.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <sstream>
#include <string>
#include <vector>

namespace Utopia
{
namespace DataIO
{
class HDFGroup
{
protected:
    hid_t _group;

    std::string _path;

    H5O_info_t _info;

    haddr_t _address;

    std::shared_ptr<std::unordered_map<haddr_t, int>> _referencecounter;

public:
    /**
     * @brief      switch state of object with other's state
     *
     * @param      other  The other
     */
    void swap(HDFGroup& other)
    {
        using std::swap;
        swap(_group, other._group);
        swap(_path, other._path);
        swap(_info, other._info);
        swap(_address, other._address);
        swap(_referencecounter, other._referencecounter);
    }

    /**
     * @brief      Get the path
     *
     * @return     std::string
     */
    std::string get_path()
    {
        return _path;
    }

    /**
     * @brief      Get the object ID
     *
     * @return     the ID of the group
     */
    hid_t get_id()
    {
        return _group;
    }

    /**
     * @brief      Get the referencecounter object
     *
     * @return     the reference counter
     */
    auto get_referencecounter()
    {
        return _referencecounter;
    }

    /**
     * @brief      Get the address object
     *
     * @return     the address
     */
    auto get_address()
    {
        return _address;
    }

    /**
     * @brief      prints info about the group to std::cout
     */
    void info()
    {
        // get information from the hdf5 group
        H5G_info_t info;
        herr_t err = H5Gget_info(_group, &info);

        // if information is succesfully retrieved print out the information
        if (err < 0)
        {
            throw std::runtime_error(
                "Getting group information by calling "
                "H5Gget_info failed!");
        }

        std::cout << "Group information:" << std::endl
                  << "- Group id: " << _group << std::endl
                  << "- Group path: " << _path << std::endl
                  << "- Number of links in group: " << info.nlinks << std::endl
                  << "- Current max. creation order value: " << info.max_corder
                  << std::endl
                  << "- Mounted files on the group: " << info.mounted << std::endl;
    }

    /**
     * @brief      write attribute
     *
     * @param      name            The name
     * @param      attribute_data  The attribute data
     *
     * @tparam     Attrdata        type of attribute data
     */
    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data)
    {
        HDFAttribute<HDFGroup>(*this, name).write(attribute_data);
    }

    /**
     * @brief      close group
     */
    void close()
    {
        if (H5Iis_valid(_group))
        {
            if ((*_referencecounter)[_address] == 1)
            {
                H5Gclose(_group);
                _group = -1;
                _referencecounter->erase(_referencecounter->find(_address));
            }
            else
            {
                (*_referencecounter)[_address] -= 1;
                _group = -1;
            }
        }
    }

    /**
     * @brief Bind the object to a new HDF5 Group, either opening existing or
     * creating a new one at path 'path' in the HDF5 group 'parent'.
     *
     * @param parent
     * @param path
     */
    template <typename HDFObject>
    void open(HDFObject& parent, std::string path)
    {
        _path = path;
        _info = H5O_info_t();
        _address = 0;

        _referencecounter = parent.get_referencecounter();

        if (H5Lexists(parent.get_id(), _path.c_str(), H5P_DEFAULT) > 0)
        {
            // open the already existing group
            _group = H5Gopen(parent.get_id(), _path.c_str(), H5P_DEFAULT);

            if (_group < 0)
            {
                throw std::runtime_error("Group opening for path" + path +
                                         " failed");
            }

            H5Oget_info(_group, &_info);
            _address = _info.addr;
            ++(*_referencecounter)[_address];
        }
        else
        { // group does not exist yet
            // create the group and intermediates
            hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(group_plist, 1);
            _group = H5Gcreate(parent.get_id(), _path.c_str(), group_plist,
                               H5P_DEFAULT, H5P_DEFAULT);
            if (_group < 0)
            {
                throw std::runtime_error("Group creation for path" + path +
                                         " failed");
            }
            // get info and update reference counter
            H5Oget_info(_group, &_info);
            _address = _info.addr;
            (*_referencecounter)[_address] = 1;
        }
    }

    /**
     * @brief      Opens a group.
     *
     * @param      path  The path to open
     *
     * @return     std::shared_ptr<HDFGroup>
     */
    std::shared_ptr<HDFGroup> open_group(std::string path)
    {
        return std::make_shared<HDFGroup>(*this, path);
    }

    /**
     * @brief      open a HDFDataset
     * @tparam     HDFDataset<HDFGroup>  dataset type with parent type
     *
     * @param      path  The path of the HDFDataset
     *
     * @return     A std::shared_ptr pointing at the newly created dataset
     */
    std::shared_ptr<HDFDataset<HDFGroup>> open_dataset(std::string path,
                                                       std::vector<hsize_t> capacity = {},
                                                       std::vector<hsize_t> chunksizes = {},
                                                       std::size_t compresslevel = 0)
    {
        return std::make_shared<HDFDataset<HDFGroup>>(
            *this, path, capacity, chunksizes, compresslevel);
    }

    /**
     * @brief      Delete the group at the given relative path
     *
     * @param      path  relative path to group to delete
     */
    void delete_group(std::string path)
    {
        // check if group exists in file, if does delete link
        herr_t status = 1;
        if (H5Lexists(_group, path.c_str(), H5P_DEFAULT))
        {
            // group exists, can be deleted

            status = H5Ldelete(_group, path.c_str(), H5P_DEFAULT);
            if (status < 0)
            {
                throw std::runtime_error("Deletion of group at path '" + path +
                                         "' failed! Wrong path?");
            }
        }
        // group does not exist, do nothing
    }

    /**
     * @brief      Default constructor
     */
    HDFGroup() = default;

    /**
     * @brief      Construct a new HDFGroup object
     *
     * @param      other  The other
     */
    HDFGroup(const HDFGroup& other)
        : _group(other._group),
          _path(other._path),
          _info(other._info),
          _address(other._address),
          _referencecounter(other._referencecounter)
    {
        (*_referencecounter)[_address] += 1;
    }

    /**
     * @brief      Construct a new HDFGroup object
     *
     * @param      other The other
     */
    HDFGroup(HDFGroup&& other) : HDFGroup()
    {
        this->swap(other);
    }

    /**
     * @brief      assignment operator
     *
     * @param      other  The other
     *
     * @return     HDFGroup&
     */
    HDFGroup& operator=(HDFGroup other)
    {
        this->swap(other);
        return *this;
    }

    /**
     * @brief      Construct a new HDFGroup object
     *
     * @param      parent     The parent group to create or open the group in
     * @param      name       The name of the group
     *
     * @tparam     HDFObject  { description }
     */
    template <typename HDFObject>
    HDFGroup(HDFObject& parent, std::string path)
    {
        open(parent, path);
    }

    /**
     * @brief      Destroy the HDFGroup object
     */
    virtual ~HDFGroup()
    {
        close();
    }
}; // namespace Utopia

/**
 * @brief      Swap lhs and rhs
 *
 * @param      lhs   The left hand side
 * @param      rhs   The right hand side
 */
void swap(HDFGroup& lhs, HDFGroup& rhs)
{
    lhs.swap(rhs);
}

} // namespace DataIO
} // namespace Utopia

#endif
