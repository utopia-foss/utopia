/**
 * @brief This file provides a class for creating and managing groups in a
 *        HDF5 file, which then can create other objects (groups and datasets)
 *        inside of them.
 *
 * @file hdfgroup.hh
 */
#ifndef UTOPIA_DATAIO_HDFGROUP_HH
#define UTOPIA_DATAIO_HDFGROUP_HH
#include <string>
#include <vector>

#include <hdf5.h>
#include <hdf5_hl.h>

#include "hdfattribute.hh"
#include "hdfdataset.hh"
#include "hdfobject.hh"
#include "hdfutilities.hh"

namespace Utopia
{
namespace DataIO
{
/*!
 * \addtogroup DataIO
 * \{
 */

/*!
 * \addtogroup HDF5
 * \{
 */

/**
 * @brief Class represting a HDFGroup, an object analogous to a folder for
 *          HDFFiles.
 *
 */
class HDFGroup final : public HDFObject< HDFCategory::group >
{
  public:
    using Base = HDFObject< HDFCategory::group >;
    /**
     * @brief      write attribute
     *
     * @param      name            The name
     * @param      attribute_data  The attribute data
     *
     * @tparam     Attrdata        type of attribute data
     */
    template < typename Attrdata >
    void
    add_attribute(std::string name, Attrdata attribute_data)
    {
        HDFAttribute(*this, name).write(attribute_data);
    }

    /**
     * @brief Bind the object to a new HDF5 Group, either opening existing or
     * creating a new one at path 'path' in the HDF5 group 'parent'.
     *
     * @param parent
     * @param path
     */
    template < HDFCategory cat >
    void
    open(HDFObject< cat >& parent, std::string path)
    {

        this->_log->debug(
            "Opening group with path {} at parent {}", path, parent.get_path());
        // check if given path exists in parent
        if (path_is_valid(parent.get_C_id(), path))
        {
            // manage new group using his class
            bind_to(H5Gopen(parent.get_C_id(), path.c_str(), H5P_DEFAULT),
                    &H5Gclose,
                    path);

            if (get_C_id() < 0)
            {
                throw std::runtime_error("Group opening for path '" + path +
                                         "' failed!");
            }
        }
        else
        { // group does not exist yet
            // create the group and intermediates
            hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(group_plist, 1);

            // manage new group using his class
            bind_to(H5Gcreate(parent.get_C_id(),
                              path.c_str(),
                              group_plist,
                              H5P_DEFAULT,
                              H5P_DEFAULT),
                    &H5Gclose,
                    path);

            H5Pclose(group_plist);

            if (get_C_id() < 0)
            {
                throw std::runtime_error("Group creation for path '" + path +
                                         "' failed!");
            }
        }
    }

    /**
     * @brief      Opens a group.
     *
     * @param      path  The path to open
     *
     * @return     std::shared_ptr<HDFGroup>
     */
    std::shared_ptr< HDFGroup >
    open_group(std::string path)
    {
        return std::make_shared< HDFGroup >(*this, path);
    }

    /**
     * @brief      open a HDFDataset
     * @tparam     HDFDataset  dataset type with parent type
     *
     * @param      path  The path of the HDFDataset
     *
     * @return     A std::shared_ptr pointing at the newly created dataset
     */
    std::shared_ptr< HDFDataset >
    open_dataset(std::string            path,
                 std::vector< hsize_t > capacity      = {},
                 std::vector< hsize_t > chunksizes    = {},
                 std::size_t            compresslevel = 0)
    {
        return std::make_shared< HDFDataset >(
            *this, path, capacity, chunksizes, compresslevel);
    }

    /**
     * @brief      Delete the group at the given relative path
     *
     * @param      path  relative path to group to delete
     */
    void
    delete_group(std::string path)
    {
        this->_log->debug("Deleting group {} in {}", path, _path);

        // check if group exists in file. If it does, delete the link
        herr_t status = 1;
        if (is_valid())
        {
            // group exists, can be deleted
            status = H5Ldelete(get_C_id(), path.c_str(), H5P_DEFAULT);
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
    HDFGroup(const HDFGroup& other) = default;

    /**
     * @brief      Construct a new HDFGroup object
     *
     * @param      other The other
     */
    HDFGroup(HDFGroup&& other)  = default;
    /**
     * @brief      assignment operator
     *
     * @param      other  The other
     *
     * @return     HDFGroup&
     */
    HDFGroup&
    operator=(HDFGroup&& other) = default;
    /**
     * @brief Copy assignment operator
     *
     * @param other
     * @return HDFGroup&
     */
    HDFGroup&
    operator=(const HDFGroup& other) = default;
    /**
     * @brief      Construct a new HDFGroup object
     *
     * @param      parent     The parent group to create or open the group in
     * @param      name       The name of the group
     *
     * @tparam     HDFObject  { description }
     */
    template < HDFCategory cat >
    HDFGroup(HDFObject< cat >& parent, std::string path)
    {
        open(parent, path);
    }

    /**
     * @brief Destroy the HDFGroup object
     *
     */
    virtual ~HDFGroup() = default;
}; // namespace Utopia

/**
 * @brief      Swap lhs and rhs
 *
 * @param      lhs   The left hand side
 * @param      rhs   The right hand side
 */
void
swap(HDFGroup& lhs, HDFGroup& rhs)
{
    lhs.swap(rhs);
}
/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif
