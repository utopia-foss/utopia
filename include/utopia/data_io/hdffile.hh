/**
 * @brief This file provides a class for creating and managing HDF5 files.
 *
 * @file hdffile.hh
 */
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
class HDFFile
{
  protected:
    hid_t                                                 _file;
    std::string                                           _path;
    std::shared_ptr< std::unordered_map< haddr_t, int > > _referencecounter;
    std::shared_ptr< HDFGroup >                           _base_group;

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
        swap(_file, other._file);
        swap(_path, other._path);
        swap(_base_group, other._base_group);
        swap(_referencecounter, other._referencecounter);
    }

    /**
     * @brief      Closes the hdffile
     */
    void
    close()
    {
        if (check_validity(H5Iis_valid(_file), _path))
        {
            H5Fflush(_file, H5F_SCOPE_GLOBAL);
            H5Fclose(_file);
        }
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
        if (check_validity(H5Iis_valid(_file), _path))
        {
            throw std::runtime_error(
                "File still bound to another HDF5 file when trying to call "
                "'open'");
        }

        if (access == "w")
        {
            _file = H5Fcreate(
                path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

            if (_file < 0)
            {
                throw std::runtime_error(
                    "File creation failed with access specifier w");
            }
        }
        else if (access == "r")
        {
            _file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

            if (_file < 0)
            {
                throw std::runtime_error(
                    "File creation failed with access specifier w");
            }
        }
        else if (access == "r+")
        {
            _file = H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);

            if (_file < 0)
            {
                throw std::runtime_error(
                    "File creation failed with access specifier w");
            }
        }
        else if (access == "x")
        {
            _file =
                H5Fcreate(path.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
            if (_file < 0)
            {
                throw std::runtime_error(
                    "File creation failed with access specifier x");
            }
        }
        else if (access == "a")
        {
            hid_t file_test = H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);

            if (file_test < 0)
            {
                file_test = H5Fcreate(
                    path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            }

            if (_file < 0)
            {
                throw std::runtime_error(
                    "File creation failed with access specifier w");
            }
            else
            {
                _file = file_test;
            }
        }
        else
        {
            throw std::invalid_argument("wrong type of access specifier, "
                                        "see documentation for allowed "
                                        "values");
        }

        _path = path;
        _referencecounter =
            std::make_shared< std::unordered_map< haddr_t, int > >();
        _base_group = std::make_shared< HDFGroup >(*this, "/");
        ++(*_referencecounter)[_base_group->get_address()];
    }

    /**
     * @brief      Get the referencecounter object
     *
     * @return     unorderd_map which holds object id and the number of
     * currently referencing objects for this id.
     */
    auto
    get_referencecounter()
    {
        return _referencecounter;
    }

    /**
     * @brief      Get the id object
     *
     * @return     hid_t
     */
    hid_t
    get_id()
    {
        return _file;
    }

    /**
     * @brief      Get the path object
     *
     * @return     std::string
     */
    std::string
    get_path()
    {
        return _path;
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
        return _base_group->open_group(path);
    }

    /**
     * @brief      open dataset
     *
     * @param      path The path to the dataset
     *
     * @return     std::shared_ptr<HDFDataset<HDFGroup>>
     */
    std::shared_ptr< HDFDataset< HDFGroup > >
    open_dataset(std::string            path,
                 std::vector< hsize_t > capacity      = {},
                 std::vector< hsize_t > chunksizes    = {},
                 std::size_t            compresslevel = 0)
    {
        // this kills the '/' at the beginning -> make this better
        return _base_group->open_dataset(path.substr(1, path.size() - 1),
                                         capacity,
                                         chunksizes,
                                         compresslevel);
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
        if (check_validity(H5Iis_valid(_file), _path))
        {
            H5Fflush(_file, H5F_SCOPE_GLOBAL);
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
    HDFFile(HDFFile&& other) : HDFFile()
    {
        this->swap(other);
    }

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
    operator=(HDFFile&& other)
    {
        this->swap(other);
        return *this;
    }

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
    HDFFile(std::string path, std::string access)
    {
        open(path, access);
    }

    /**
     * @brief      Destroy the HDFFile object
     */
    virtual ~HDFFile()
    {
        close();
    }
};

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia
#endif
