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
class HDFFile;
class HDFGroup
{
private:
protected:
    hid_t _group;
    std::string _path;
    H5O_info_t _info;
    haddr_t _address;
    std::shared_ptr<std::unordered_map<haddr_t, int>> _refcounts;

public:
    /**
     * @brief switch state of object with other's state
     *
     * @param other
     */
    void swap(HDFGroup& other)
    {
        using std::swap;
        swap(_group, other._group);
        swap(_path, other._path);
        swap(_info, other._info);
        swap(_address, other._address);
        swap(_refcounts, other._refcounts);
    }

    /**
     * @brief Get the path object
     *
     * @return std::string
     */
    std::string get_path()
    {
        return _path;
    }

    /**
     * @brief Get the id object
     *
     * @return hid_t
     */
    hid_t get_id()
    {
        return _group;
    }

    /**
     * @brief Get the referencecounter object
     *
     * @return auto
     */
    auto get_referencecounter()
    {
        return _refcounts;
    }

    /**
     * @brief Get the address object
     *
     * @return auto
     */
    auto get_address()
    {
        return _address;
    }

    /**
     * @brief get info about group
     *
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
                "Getting the information by calling H5Gget_info failed!");
        }
        else
        {
            std::cout << "Printing information of the group:" << std::endl
                      << "- Group id: " << _group << std::endl
                      << "- Group path: " << _path << std::endl
                      << "- Number of links in group: " << info.nlinks << std::endl
                      << "- Current maximum creation order value for group: "
                      << info.max_corder << std::endl
                      << "- There are mounted files on the group: " << info.mounted
                      << std::endl;
        }
    }

    /**
     * @brief write attribuet
     *
     * @param name
     * @param attribute_data
     */
    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data)
    {
        HDFAttribute<HDFGroup>(*this, std::forward<std::string&&>(name)).write(attribute_data);
    }

    /**
     * @brief close group
     *
     */
    void close()
    {
        if (H5Iis_valid(_group))
        {
            if ((*_refcounts)[_address] == 1)
            {
                H5Gclose(_group);
                _group = -1;
                _refcounts->erase(_refcounts->find(_address));
            }
            else
            {
                (*_refcounts)[_address] -= 1;
            }
        }
    }

    /**
     * \param path
     * \return std::shared_ptr<HDFGroup>
     */
    std::shared_ptr<HDFGroup> open_group(std::string path)
    {
        return std::make_shared<HDFGroup>(*this, path);
    }

    /// Open a HDFDataset
    /**
     *  \tparam HDFDataset<HDFGroup> HDFDataset type with parent type HDFGroup
     *  \param path The path of the HDFDataset
     *  \return A std::shared_ptr pointing at the newly created dataset
     */
    std::shared_ptr<HDFDataset<HDFGroup>> open_dataset(std::string path)
    {
        return std::make_shared<HDFDataset<HDFGroup>>(*this, path);
    }

    /**
     * \param path relative path to group to delete
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
                throw std::runtime_error(
                    "deletion of group failed, wrong path?");
            }
        }
        // group does not exist, do nothing
    }

    // default constructor
    HDFGroup() = default;
    /**
     * @brief Construct a new HDFGroup object
     *
     * @param other
     */
    HDFGroup(const HDFGroup& other)
        : _group(other._group),
          _path(other._path),
          _info(other._info),
          _address(other._address),
          _refcounts(other._refcounts)
    {
        (*_refcounts)[_address] += 1;
    }

    /**
     * @brief Construct a new HDFGroup object
     *
     * @param other
     */
    HDFGroup(HDFGroup&& other) : HDFGroup()
    {
        this->swap(other);
    }

    /**
     * @brief assignment operator
     *
     * @param other
     * @return HDFGroup&
     */
    HDFGroup& operator=(HDFGroup other)
    {
        this->swap(other);
        return *this;
    }

    /**
     * @brief Construct a new HDFGroup object
     *
     * @tparam HDFObject
     * @param object
     * @param name
     */
    template <typename HDFObject>
    HDFGroup(HDFObject& object, std::string name)
        : _path(name), _refcounts(object.get_referencecounter())
    {
        if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) > 0)
        {
            _group = H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
            H5Oget_info(_group, &_info);
            _address = _info.addr;
            ++(*_refcounts)[_address];
        }
        else
        {
            hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(group_plist, 1);
            _group = H5Gcreate(object.get_id(), _path.c_str(), group_plist,
                               H5P_DEFAULT, H5P_DEFAULT);

            H5Oget_info(_group, &_info);
            _address = _info.addr;
            (*_refcounts)[_address] = 1;
        }
    }

    /**
     * @brief Destroy the HDFGroup object
     *
     */
    virtual ~HDFGroup()
    {
        if (H5Iis_valid(_group))
        {
            if ((*_refcounts)[_address] == 1)
            {
                H5Gclose(_group);
                _refcounts->erase(_refcounts->find(_address));
            }
            else
            {
                --(*_refcounts)[_address];
            }
        }
    }
}; // namespace Utopia

void swap(HDFGroup& lhs, HDFGroup& rhs)
{
    lhs.swap(rhs);
}
} // namespace DataIO
} // namespace Utopia
#endif