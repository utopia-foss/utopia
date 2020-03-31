#ifndef UTOPIA_DATAIO_HDFFILE_HH
#define UTOPIA_DATAIO_HDFFILE_HH

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <hdf5.h>
#include <hdf5_hl.h>

#include "hdfdataset.hh"
#include "hdfgroup.hh"
#include "utopia/data_io/hdfobject.hh"
#include "utopia/data_io/hdfutilities.hh"

namespace Utopia
{
namespace DataIO
{

// Doxygen group for dataIO backend
// ++++++++++++++++++++++++++++++++++++++++++++++
/*!
 * \addtogroup DataIO
 * \{
 */

/**
 *  \addtogroup HDF5 HDF5
 *  \{
 *
 */

/**
 * @page HDFclasses  HDF5 Backend Module
 *
 * \section what Overview
 * This backend is a replacement of the HDF5 C++ wrappers. It does not implement
 * the full HDF5 standard, but only the features we deemed useful and needed
 * for the Utopia project.
 * It was created because the C++ wrappers supplied by the HDF5 group do not
 * support STL containers and  in general no modern C++ features. Furthermore,
 * development of the pure C implementation is much faster and it is generally
 * more complete.
 *
 * \section impl Implementation
 * In this module, C++ classes are created which represent HDF5 files, groups,
 * datasets and attributes, with the associated object creation- and data I/O-
 * capabilities, limited to one and two-dimensional datasets of arrays,
 * containers or scalars. Additionally, helper classes for organizing type
 * mapping and type conversion are supplied, which normally are irelevant for
 * users. Finally a number of helper functions are supplied which are used to
 * assert correctness.
 */

/**
 * @brief Class representing a HDF5 file.
 *
 */
class HDFFile final : public HDFObject< HDFCategory::file >
{
  private:
    /**
     * @brief Pointer to base group of the file
     *
     */
    std::shared_ptr< HDFGroup > _base_group;

  public:
    /**
     * @brief      Function for exchanging states
     *
     * @param      other  The other
     */
    void
    swap(HDFFile& other)
    {
        using std::swap;
        using Utopia::DataIO::swap;
        swap(_base_group, other._base_group);
    }

    /**
     * @brief Open a file at location 'path' with access specifier 'access'.
     *        Keep in mind that if the object refers to another file, it has to
     *        be closed first before opening another.
     *
     * @param path Path to open the file at
     * @param      access  Access specifier for the new file, possible values:
     *                     'r' (readonly, file must exist), 'r+' (read/write,
     *                     file must exist), 'w' (create file, truncate if
     *                     exists), 'x' (create file, fail if exists), or 'a'
     *                     (read/write if exists, create otherwise)
     */
    void
    open(std::string path, std::string access)
    {
        this->_log->info(
            "Opening file at {} with access specifier {}", path, access);
        if (is_valid())
        {
            throw std::runtime_error(
                "File still bound to another HDF5 file when trying to call "
                "'open'. Close first.");
        }

        // create file access property list
        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);

        // set driver to stdio and close strongly, i.e., close all resources
        // with the file
        H5Pset_fapl_stdio(fapl);
        H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);

        if (access == "w")
        {
            bind_to(H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl),
                    &H5Fclose,
                    path);
        }
        else if (access == "r")
        {
            bind_to(
                H5Fopen(path.c_str(), H5F_ACC_RDONLY, fapl), &H5Fclose, path);
        }
        else if (access == "r+")
        {
            bind_to(H5Fopen(path.c_str(), H5F_ACC_RDWR, fapl), &H5Fclose);
        }
        else if (access == "x")
        {
            bind_to(H5Fcreate(path.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, fapl),
                    &H5Fclose,
                    path);
        }
        else if (access == "a")
        {
            hid_t file_test = H5Fopen(path.c_str(), H5F_ACC_RDWR, fapl);

            if (file_test < 0)
            {
                file_test =
                    H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
            }

            bind_to(file_test, &H5Fclose);
        }
        else
        {
            throw std::invalid_argument("wrong type of access specifier, "
                                        "see documentation for allowed "
                                        "values");
        }

        _base_group = std::make_shared< HDFGroup >(*this, "/");
    }

    /**
     * @brief      Get the basegroup object via shared ptr
     *
     * @return     std::shared_ptr<HDFGroup>
     */
    std::shared_ptr< HDFGroup >
    get_basegroup()
    {
        return _base_group;
    }

    /**
     * @brief      Open group at path 'path', creating all intermediate objects
     *             in the path. Separation character is: /
     *
     * @param      path The path to the group
     *
     * @return     std::shared_ptr<HDFGroup>
     */
    std::shared_ptr< HDFGroup >
    open_group(std::string path)
    {
        // this removes the '/' at the beginning, because this is
        // reserved for the basegroup
        if (path[0] == '/')
        {
            path = path.substr(1, path.size() - 1);
        }

        return _base_group->open_group(path);
    }

    /**
     * @brief      open dataset
     *
     * @param      path The path to the dataset
     *
     * @return     std::shared_ptr<HDFDataset>
     */
    std::shared_ptr< HDFDataset >
    open_dataset(std::string            path,
                 std::vector< hsize_t > capacity      = {},
                 std::vector< hsize_t > chunksizes    = {},
                 std::size_t            compresslevel = 0)
    {
        // this removes the '/' at the beginning, because this is
        // reserved for the basegroup
        if (path[0] == '/')
        {
            path = path.substr(1, path.size() - 1);
        }

        return _base_group->open_dataset(
            path, capacity, chunksizes, compresslevel);
    }

    /**
     * @brief      deletes the group pointed to by absolute path 'path'
     *
     * @param      path  absolute path to the group to be deleted
     */
    void
    delete_group(std::string&& path)
    {
        _base_group->delete_group(std::forward< std::string&& >(path));
    }

    /**
     * @brief      Initiates an immediate write to disk of the data of the file
     */
    void
    flush()
    {
        if (is_valid())
        {
            H5Fflush(get_C_id(), H5F_SCOPE_GLOBAL);
        }
    }

    /**
     * @brief      Construct a new default HDFFile object
     */
    HDFFile() = default;

    /**
     * @brief      Move constructor Construct a new HDFFile object via move
     *             semantics
     *
     * @param      other  rvalue reference to HDFFile object
     */
    HDFFile(HDFFile&& other) = default;

    /**
     * @brief Copy assignment operator, explicitly deleted, hence cannot be
     * used.
     *
     * @param other
     * @return HDFFile&
     */
    HDFFile&
    operator=(const HDFFile& other) = delete;

    /**
     * @brief Move assigment operator
     *
     * @param      other  reference to HDFFile object
     *
     * @return     HDFFile&
     */
    HDFFile&
    operator=(HDFFile&& other) = default;

    /**
     * @brief Copy constructor. Explicitly deleted, hence cannot be used
     *
     * @param other
     * @return HDFFile&
     */
    HDFFile(const HDFFile& other) = delete;

    /**
     * @brief      Construct a new HDFFile object
     *
     * @param      path    Path to the new file
     * @param      access  Access specifier for the new file, possible values:
     *                     'r' (readonly, file must exist), 'r+' (read/write,
     *                     file must exist), 'w' (create file, truncate if
     *                     exists), 'x' (create file, fail if exists), or 'a'
     *                     (read/write if exists, create otherwise)
     */
    HDFFile(std::string path, std::string access) : HDFFile()
    {
        // init the logger here because it is needed throughout the module
        // and its existence is not guaranteed when it is initialized in `core`
        _log = init_logger(log_data_io, spdlog::level::warn, false);
        open(path, access);
    }

    /**
     * @brief      Destroy the HDFFile object
     */
    virtual ~HDFFile() = default;
};

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia
#endif
