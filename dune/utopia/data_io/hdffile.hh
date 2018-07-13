/**
 * @brief This file provides a class for creating and managing HDF5 files.
 *
 * @file hdffile.hh
 */
#ifndef HDFFILE_HH
#define HDFFILE_HH

#include "hdfdataset.hh"
#include "hdfgroup.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Utopia
{
namespace DataIO
{
// forward declaration of HDFGroup -> exchange later for the real thing.

class HDFFile
{
protected:
    hid_t _file;
    std::string _path;
    std::shared_ptr<std::unordered_map<haddr_t, int>> _referencecounter;
    std::shared_ptr<HDFGroup> _base_group;

public:
    /**
     * @brief      Function for exchanging states
     *
     * @param      other  The other
     */
    void swap(HDFFile& other)
    {
        using std::swap;
        using Utopia::DataIO::swap;
        swap(_file, other._file);
        swap(_path, other._path);
        swap(_base_group, other._base_group);
        swap(_referencecounter, other._referencecounter);
    }

    /**
     * @brief      closes the hdffile
     */
    void close()
    {
        if (H5Iis_valid(_file))
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
     *                     (read/write if exists, create otherwise; default)
     */
    void open(std::string path, std::string access)
    {
        if (H5Iis_valid(_file))
        {
            throw std::runtime_error(
                "File still bound to another HDF5 file when trying to call "
                "'open'");
        }

        if (access == "w")
        {
            _file = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

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
            _file = H5Fcreate(path.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
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
                file_test = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
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
            throw std::invalid_argument(
                "wrong type of access specifier, "
                "see documentation for allowed "
                "values");
        }

        _path = path;

        _referencecounter = std::make_shared<std::unordered_map<haddr_t, int>>();
        _base_group = std::make_shared<HDFGroup>(*this, "/");
        ++(*_referencecounter)[_base_group->get_address()];
    }

    /**
     * @brief      Get the referencecounter object
     *
     * @return     auto
     */
    auto get_referencecounter()
    {
        return _referencecounter;
    }

    /**
     * @brief      Get the id object
     *
     * @return     hid_t
     */
    hid_t get_id()
    {
        return _file;
    }

    /**
     * @brief      Get the path object
     *
     * @return     std::string
     */
    std::string get_path()
    {
        return _path;
    }

    /**
     * @brief      Get the basegroup object via shared ptr
     *
     * @return     std::shared_ptr<HDFGroup>
     */
    std::shared_ptr<HDFGroup> get_basegroup()
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
    std::shared_ptr<HDFGroup> open_group(std::string&& path)
    {
        return _base_group->open_group(std::forward<std::string&&>(path));
    }

    /**
     * @brief      open dataset
     *
     * @param      path The path to the dataset
     *
     * @return     std::shared_ptr<HDFDataset<HDFGroup>>
     */
    std::shared_ptr<HDFDataset<HDFGroup>> open_dataset(std::string path,
                                                       std::vector<hsize_t> capacity = {},
                                                       std::vector<hsize_t> chunksizes = {},
                                                       std::size_t compresslevel = 0)
    {
        return _base_group->open_dataset(path, capacity, chunksizes, compresslevel);
    }

    /**
     * @brief      deletes the group pointed to by absolute path 'path'
     *
     * @param      path  absolute path to the group to be deleted
     */
    void delete_group(std::string&& path)
    {
        _base_group->delete_group(std::forward<std::string&&>(path));
    }

    /**
     * @brief      Initiates an immediate write to disk of the data of the file
     */
    void flush()
    {
        if (H5Iis_valid(_file))
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
     * @brief Copy assignment operator, explicitly deleted, hence cannot be used.
     *
     * @param other
     * @return HDFFile&
     */
    HDFFile& operator=(const HDFFile& other) = delete;

    /**
     * @brief Move assigment operator
     *
     * @param      other  reference to HDFFile object
     *
     * @return     HDFFile&
     */
    HDFFile& operator=(HDFFile&& other)
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
     *                     (read/write if exists, create otherwise; default)
     */
    HDFFile(std::string path, std::string access)
        : _file([&]() {
              //   H5Eset_auto(0, 0, NULL);

              if (access == "w")
              {
                  return H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
                  if (_file < 0)
                  {
                      throw std::runtime_error(
                          "File creation failed with access specifier w");
                  }
              }
              else if (access == "r")
              {
                  return H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                  if (_file < 0)
                  {
                      throw std::runtime_error(
                          "File creation failed with access specifier r");
                  }
              }
              else if (access == "r+")
              {
                  return H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
                  if (_file < 0)
                  {
                      throw std::runtime_error(
                          "File creation failed with access specifier r+");
                  }
              }
              else if (access == "x")
              {
                  hid_t file = H5Fcreate(path.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
                  if (_file < 0)
                  {
                      throw std::runtime_error(
                          "File creation failed with access specifier x");
                  }
                  return file;
              }
              else if (access == "a")
              {
                  hid_t file_test = H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
                  if (file_test < 0)
                  {
                      file_test = H5Fcreate(path.c_str(), H5F_ACC_TRUNC,
                                            H5P_DEFAULT, H5P_DEFAULT);

                      if (_file < 0)
                      {
                          throw std::runtime_error(
                              "File creation failed with access specifier a");
                      }
                  }
                  return file_test;
              }
              else
              {
                  throw std::invalid_argument(
                      "Wrong type of access specifier, "
                      "see documentation for allowed "
                      "values");
              }
          }())
    {
        if (H5Iis_valid(_file))
        {
            _path = path;
            _referencecounter = std::make_shared<std::unordered_map<haddr_t, int>>();
            _base_group = std::make_shared<HDFGroup>(*this, "/");
            ++(*_referencecounter)[_base_group->get_address()];
        }
    }

    /**
     * @brief      Destroy the HDFFile object
     */
    virtual ~HDFFile()
    {
        if (H5Iis_valid(_file))
        {
            H5Fflush(_file, H5F_SCOPE_GLOBAL);
            H5Fclose(_file);
        }
    }
};
} // namespace DataIO
} // namespace Utopia
#endif
