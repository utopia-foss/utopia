/**
 * @brief This file provides metafunctions for automatically determining the
 *        nature of a C/C++ types at compile time (container or not, string or
 *         not, fixed-size array ), and getting the base type of pointer and
 *         cv-qualified types.
 * @file hdfutilities.hh
 */
#ifndef UTOPIA_DATAIO_HDFUTILITIES_HH
#define UTOPIA_DATAIO_HDFUTILITIES_HH

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utopia/core/utils.hh>
#include <vector>

#include <hdf5.h>

// Functions for determining if a type is an STL-container are provided here.
// This is used if we wish to make hdf5 types for storing such data in an
// hdf5 dataset.
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
 * @brief Check for validity of a hdf5 htri_t type or similar
 * @param [in] valid parameter to check
 * @param [in] object name Name of object to be referenced in thrown exceptions
              if valid <= 0
 * @return  The valid argument for further use
 * @details This function is necessary because for instance H5Iis_valid does not
 *           return a boolean (non existant in C), but a value which is > 0 if
 *           everything is fine, < 0 if some error occurred during checking and
 *           0 if the object to check is invalid. This has to be taken into
 *           account in order to be able to track bugs or wrong usage properly.
 *           See here:
 *           https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.6/hdf5-1.6.7/src/unpacked/src/H5public.h
 *           which yields the following snippet:
 *
 *           Boolean type.  Successful return values are zero (false) or
 *           positive (true). The typical true value is 1 but don't bet on it.
 *           Boolean functions cannot fail.  Functions that return `htri_t'
 *           however return zero (false), positive (true), or negative
 *           (failure).
 *           The proper way to test for truth from a htri_t function is:
 *
 * 	         if ((retval = H5Tcommitted(type))>0) {
 *	             printf("data type is committed\n");
 *	         } else if (!retval) {
 * 	                printf("data type is not committed\n");
 *	         } else {
 * 	             printf("error determining whether data type is committed\n");
 *	         }
 */
bool
check_validity(htri_t valid, const std::string_view object_name)
{
    if (valid > 0)
    {
        return true;
    }

    else if (valid < 0)
    {
        throw std::runtime_error("Object " + std::string(object_name) + ": " +
                                 "Error in validity check");
        return false;
    }
    else
    {
        return false;
    }
}

/// Checks iteratively if each segment of a path exists

/**
 * @brief Checks if a given path exists in a hdf5 object identitifed by its id.

 * @param loc_id Identifier of the file or group to query.
 * @param path The path of the link to check. This can be a relative or
 *                an absolute path, but (as with H5Lexists) it can NOT use
 *                the ``../`` syntax to go to the parent object. For such
 *                cases, an absolute path needs to be given.
 * @param link_property_list  Link access property list identifier.
 * @return htri_t Variable which tells if the given path exists (true > 0, false
 = 0, error < 0)
 */
htri_t
path_exists(hid_t       loc_id,
            std::string path,
            hid_t       link_property_list = H5P_DEFAULT)
{
    // Position of the segment cursor; all characters before are checked
    // For absolute paths, search always needs to start behind index 1
    std::size_t seg_pos = (path.find("/") == 0) ? 1 : 0;

    // A buffer for the return value of H5Lexists
    htri_t rv;

    // Go over all segments until the whole string is
    while (seg_pos != std::string::npos)
    {
        // Find the position of the next "/", strictly after the current
        // cursor position
        seg_pos = path.find("/", seg_pos + 1);

        // Check for existence of the subpath. If seg_pos is string::npos,
        // the substring is the full path and this is the last loop iteration.
        rv = H5Lexists(
            loc_id, path.substr(0, seg_pos).c_str(), link_property_list);

        // If this segment does not exists, need to return already
        if (rv <= 0)
        {
            return rv;
        }
    }

    // Checked the full path. Can return the last return value now:
    return rv;
}
/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia
#endif
