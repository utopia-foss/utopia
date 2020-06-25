/**
 * @brief This is the central file of the HDF5 dataIO module of Utopia and
 * provides a class for writing to, reading from and creating almost arbitrary
 * data to a dataset in a HDF5 file.
 * @file hdfdataset.hh
 * @todo Should we append in the first dimensions instead of the last?
 *       Think about what is better/more performant/ expected by users?
 *
 */
#ifndef UTOPIA_DATAIO_HDFDATASET_HH
#define UTOPIA_DATAIO_HDFDATASET_HH

#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <hdf5.h>
#include <hdf5_hl.h>

#include "../core/type_traits.hh"

#include "hdfattribute.hh"
#include "hdfbufferfactory.hh"
#include "hdfchunking.hh"
#include "hdfdataspace.hh"
#include "hdfobject.hh"
#include "hdftype.hh"
#include "hdfutilities.hh"
#include "utopia/data_io/hdfidentifier.hh"

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
 * @brief      Class representing a HDFDataset, wich reads and writes
 * data and attributes
 *
 * @tparam     HDFObject  The type of the parent object
 */
class HDFDataset final : public HDFObject< HDFCategory::dataset >
{
  private:
    /**
     * @brief      helper function for making a non compressed dataset
     *
     * @param      typesize Size of the C type to write in bytes
     *
     * @tparam     Datatype        The data type stored in this dataset
     *
     * @return     The created dataset
     */
    template < typename Datatype >
    void
    __create_dataset__(std::size_t typesize)
    {

        this->_log->debug("Creating dataset with typesize {} at path {} ...",
                          typesize,
                          _path);
        this->_log->trace("refcount before creation: {}", get_refcount());

        // create group property list and (potentially) intermediate groups
        hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(group_plist, 1);

        _type.close();

        _type.open< Datatype >("datatype of " + _path, typesize);

        // this is something different than typesize, which has meaning for
        // arrays only
        if (_capacity != _current_extent)
        {
            if (_chunksizes.size() != _rank)
            {
                this->_log->debug("Computing chunksizes ...");
                _chunksizes =
                    calc_chunksize(_type.size(), _current_extent, _capacity);
            }
        }

        hid_t plist = H5Pcreate(H5P_DATASET_CREATE);

        // distinguish by chunksize; chunked dataset needed for compression
        if (_chunksizes.size() > 0)
        {
            // create creation property list, set chunksize and compress level

            this->_log->debug("Setting given chunksizes ...");
            H5Pset_chunk(plist, _rank, _chunksizes.data());

            if (_compress_level > 0)
            {
                H5Pset_deflate(plist, _compress_level);
            }

            _filespace.close();
            // make dataspace
            _filespace.open(
                _path + " file dataspace", _rank, _current_extent, _capacity);

            // create dataset and return
            this->_log->debug(
                "Creating actual dataset and binding it to object class ...");

            bind_to(H5Dcreate(_parent_identifier.get_id(),
                              _path.c_str(),
                              _type.get_C_id(),
                              _filespace.get_C_id(),
                              group_plist,
                              plist,
                              H5P_DEFAULT),
                    &H5Dclose);

            if (not is_valid())
            {
                throw std::runtime_error("Invalid dataset id " + _path + " " +
                                         std::to_string(__LINE__));
            }
        }
        else
        {

            // make dataspace
            _filespace.open(
                _path + "file dataspace", _rank, _current_extent, _capacity);

            this->_log->debug(
                "Creating actual dataset and binding it to object class ...");
            // can create the dataset right away
            bind_to(H5Dcreate(_parent_identifier.get_id(),
                              _path.c_str(),
                              _type.get_C_id(),
                              _filespace.get_C_id(),
                              group_plist,
                              H5P_DEFAULT,
                              H5P_DEFAULT),
                    &H5Dclose);

            if (not is_valid())
            {
                throw std::runtime_error("Invalid dataset id " + _path + " " +
                                         std::to_string(__LINE__));
            }
        }

        this->_log->debug(
            "refcount of dataset after creation {}: {}", _path, get_refcount());
    }

    /**
     * @brief Adds attributes for rank, current_extent and capacity
     *
     */
    void
    __add_topology_attributes__()
    {
        this->_log->debug(
            "Adding topology attributes to dataset {} {}", _path, get_C_id());
        add_attribute("rank", _rank);
        this->_log->debug("refcount after rank {}", get_refcount());

        add_attribute("current_extent", _current_extent);
        this->_log->debug("refcount after current_extent {}", get_refcount());

        add_attribute("capacity", _capacity);
        this->_log->debug("refcount after capacity {}", get_refcount());
    }

    /**
     * @brief Writes containers to the dataset
     *
     * @tparam T automatically determined
     * @param data data to write, can contain almost everything, also other
     * containers
     * @param memspace memory dataspace
     * @param filespace file dataspace: how the data shall be represented in the
     * file
     * @return herr_t status variable indicating if write was successful or not
     */
    template < typename T >
    herr_t
    __write_container__(T&& data)
    {
        this->_log->debug("Writing container data to dataset {}...", _path);

        this->_log->debug(
            "Dataset {} 's refcount write begin {}", _path, get_refcount());

        using value_type_1 =
            typename Utils::remove_qualifier_t< T >::value_type;
        using base_type = Utils::remove_qualifier_t< value_type_1 >;

        // we can write directly if we have a plain vector, no nested or
        // stringtype.
        if constexpr (std::is_same_v< T, std::vector< value_type_1 > > and
                      not Utils::is_container_v< value_type_1 > and
                      not Utils::is_string_v< value_type_1 >)
        {
            this->_log->debug("... of simple vectortype");

            // check if attribute has been created, else do
            if (not is_valid())
            {
                this->_log->debug("... dataset not yet existing, creating it "
                                  "for simple vectortype");
                __create_dataset__< base_type >(0);
            }
            else
            {
                this->_log->debug(
                    "... dataset existing, reading out type and writing data");
                // check if datatypes are compatible

                HDFType temp_type;
                temp_type.open< base_type >("testtype", 0);

                if (temp_type != _type)
                {
                    throw std::runtime_error(
                        "Error, cannot write container data of a "
                        "different type into dataset " +
                        _path);
                }
            }
            this->_log->debug("Dataset {} 's refcount before write {}",
                              _path,
                              get_refcount());

            return H5Dwrite(get_C_id(),
                            _type.get_C_id(),
                            _memspace.get_C_id(),
                            _filespace.get_C_id(),
                            H5P_DEFAULT,
                            data.data());
        }
        // when stringtype or containertype is stored in a container, then
        // we have to buffer. bufferfactory handles how to do this in detail
        else
        {
            this->_log->debug("... of nontrivial containertype");

            std::size_t typesize = 0;
            // check if array, if yes, get typesize, else typesize is 0 and
            // typefactory creates vlen data or string data
            if constexpr (Utils::is_container_v< base_type > and
                          Utils::is_array_like_v< base_type >)
            {
                // get_size is a metafunction defined in hdfutilities.hh
                typesize = Utils::get_size< base_type >::value;
            }

            if (not is_valid())
            {
                this->_log->debug(
                    "... dataset not yet existing, creating it for array type");
                __create_dataset__< base_type >(typesize);
            }
            else
            {
                // check if datatypes are compatible
                this->_log->debug("... dataset existing, reading out type");

                HDFType temp_type;
                temp_type.open< base_type >("testtype", typesize);

                if (temp_type != _type)
                {
                    throw std::runtime_error(
                        "Error, cannot write fixedsize container data of a "
                        "different type into dataset " +
                        _path);
                }
            }

            this->_log->debug(
                "... buffering data into vectortype appropriate for writing");
            // the reference is needed here, because addresses of underlaying
            // data arrays are needed.
            auto buffer = HDFBufferFactory::buffer(
                std::begin(data),
                std::end(data),
                [](auto& value) -> value_type_1& { return value; });

            this->_log->debug("Dataset {} 's refcount before write {}",
                              _path,
                              get_refcount());

            this->_log->debug("... writing data");
            return H5Dwrite(get_C_id(),
                            _type.get_C_id(),
                            _memspace.get_C_id(),
                            _filespace.get_C_id(),
                            H5P_DEFAULT,
                            buffer.data());
        }
    }

    /**
     * @brief writes stringtypes
     *
     * @param data data to write, (const) char* or std::string
     * @param memspace memory dataspace
     * @param filespace file dataspace: how the data shall be represented in the
     * file
     * @return herr_t status variable indicating if write was successful or not
     */
    template < typename T >
    herr_t
    __write_stringtype__(T data)
    {
        this->_log->debug("Writing string data to dataset {}...", _path);

        // Since std::string cannot be written directly,
        // (only const char*/char* can), a buffer pointer has been added
        // to handle writing in a clearer way and with less code
        auto        len    = 0;
        const char* buffer = nullptr;

        if constexpr (std::is_pointer_v< T >) // const char* or char* -> strlen
                                              // needed
        {
            this->_log->debug("... stringtype is pointer-valued");
            len    = std::strlen(data);
            buffer = data;
        }
        else // simple for strings
        {
            this->_log->debug("... stringtype is of not pointer-valued");
            len    = data.size();
            buffer = data.c_str();
        }

        // check if dataset has been created, else do
        if (not is_valid())
        {
            this->_log->debug(
                "... dataset not yet existing, creating it for stringtypee");
            // check if datatypes are compatible

            __create_dataset__< const char* >(len);
        }
        else
        {
            this->_log->debug("... dataset existing, reading out type");
            // check if datatypes are compatible
            HDFType temp_type;
            temp_type.open< const char* >("testtype", len);

            if (temp_type != _type)
            {
                throw std::runtime_error("Error, cannot write string data of a "
                                         "different type into dataset " +
                                         _path);
            }
        }

        this->_log->debug(" ... writing data");
        // use that strings store data in consecutive memory
        return H5Dwrite(get_C_id(),
                        _type.get_C_id(),
                        _memspace.get_C_id(),
                        _filespace.get_C_id(),
                        H5P_DEFAULT,
                        buffer);
    }

    /**
     * @brief Writes pointers, shape is like numpy shape arg
     *
     * @tparam T automatically determined
     * @param data data to write. Can contain only plain old data
     * @param data data to write
     * @param memspace memory dataspace
     * @param filespace file dataspace: how the data shall be represented in the
     * file
     * @return herr_t status variable indicating if write was successful or not
     */
    template < typename T >
    herr_t
    __write_pointertype__(T data)
    {
        this->_log->debug("Writing pointer data to dataset {}...", _path);

        // result types removes pointers, references, and qualifiers
        using basetype = Utils::remove_qualifier_t< T >;

        if (not is_valid())
        {
            this->_log->debug(
                "... dataset not yet existing, creating it for pointertype");

            __create_dataset__< basetype >(0);
        }
        else
        {
            // check if datatypes are compatible
            this->_log->debug("... dataset existing, reading out type");

            HDFType temp_type;
            temp_type.open< basetype >("testtype", 0);

            if (temp_type != _type)
            {
                throw std::runtime_error(
                    "Error, cannot write pointer data of a "
                    "different type into dataset " +
                    _path);
            }
        }
        this->_log->debug(" ... writing data");

        return H5Dwrite(get_C_id(),
                        _type.get_C_id(),
                        _memspace.get_C_id(),
                        _filespace.get_C_id(),
                        H5P_DEFAULT,
                        data);
    }

    /**
     * @brief Writes simple scalars, which are not pointers, containers or
     * strings
     *
     * @tparam T automatically determined
     * @param data data to write
     * @param memspace  memory data space
     * @param filespace dataspace representing the shape of the data in memory
     * @return herr_t status telling if write was successul, < 0 if not.
     */
    template < typename T >
    herr_t
    __write_scalartype__(T data)
    {
        this->_log->debug("Writing scalar data to dataset {}...", _path);
        // because we just write a scalar, the shape tells basically that
        // the attribute is pointlike: 1D and 1 entry.
        if (not is_valid())
        {
            this->_log->debug(
                "... dataset not yet existing, creating it for pointertype");

            __create_dataset__< std::decay_t< T > >(0);
        }
        else
        {
            // check if datatypes are compatible
            this->_log->debug("... dataset existing, reading out type");

            HDFType temp_type;
            temp_type.open< std::decay_t< T > >("testtype", 0);

            if (temp_type != _type)
            {
                throw std::runtime_error("Error, cannot write scalar data of a "
                                         "different type into dataset " +
                                         _path);
            }
        }

        this->_log->debug(" ... writing data");

        return H5Dwrite(get_C_id(),
                        _type.get_C_id(),
                        _memspace.get_C_id(),
                        _filespace.get_C_id(),
                        H5P_DEFAULT,
                        &data);
    }

    /// Read a cointainer
    // We could want to read into a predefined buffer for some reason (frequent
    // reads), and thus this and the following functions expect an argument
    // 'buffer' to store their data in. The function 'read(..)' is then
    // overloaded to allow for automatic buffer creation or a buffer argument.
    template < typename Type >
    herr_t
    __read_container__(Type& buffer)
    {

        this->_log->debug("Reading container data from dataset {}...", _path);

        using value_type_1 =
            Utils::remove_qualifier_t< typename Type::value_type >;

        // when the value_type of Type is a container again, we want nested
        // arrays basically. Therefore we have to check if the desired type
        // Type is suitable to hold them, read the nested data into a hvl_t
        // container, assuming that they are varlen because this is the more
        // general case, and then turn them into the desired type again...
        if constexpr (Utils::is_container_v< value_type_1 > ||
                      Utils::is_string_v< value_type_1 >)
        {
            this->_log->debug(
                "... reading nested container or container of strings ...");
            using value_type_2 =
                Utils::remove_qualifier_t< typename value_type_1::value_type >;

            // if we have nested containers of depth larger than 2, throw a
            // runtime error because we cannot handle this
            // TODO extend this to work more generally
            if constexpr (Utils::is_container_v< value_type_2 >)
            {
                throw std::runtime_error(
                    "Dataset" + _path +
                    ": Cannot read data into nested containers with depth > 3 "
                    "in attribute " +
                    _path + " into vector containers!");
            }
            if constexpr (!std::is_same_v< std::vector< value_type_1 >, Type >)
            {
                throw std::runtime_error("Dataset" + _path +
                                         ": Can only read data"
                                         " into vector containers!");
            }

            // everything is fine.

            // check if type given in the buffer is std::array.
            // If it is, the user knew that the data stored there
            // has always the same length, otherwise she does not
            // know and thus it is assumed that the data is variable
            // length.
            if (_type.type_category() == H5T_ARRAY)
            {
                this->_log->debug("... nested type is array-like...");
                // check if std::array is given as value_type,
                // if not adjust sizes
                if constexpr (!Utils::is_array_like_v< value_type_1 >)
                {
                    // if yes, throw exception is size is insufficient because
                    // the size cannot be adjusted

                    throw std::invalid_argument(
                        "Dataset " + _path +
                        ": Cannot read into container of non arrays "
                        "when data type in file is fixed array type");
                }

                return H5Dread(get_C_id(),
                               _type.get_C_id(),
                               _memspace.get_C_id(),
                               _filespace.get_C_id(),
                               H5P_DEFAULT,
                               buffer.data());
            }
            else if (_type.type_category() == H5T_STRING)
            {
                this->_log->debug("... nested type is string-like...");

                if constexpr (!Utils::is_string_v< value_type_1 >)
                {
                    throw std::invalid_argument(
                        "Dataset " + _path +
                        ": Can only read stringdata into string elements");
                }
                else
                {
                    /*
                     * we have two possibilities, which have to be treated
                     * sparatly, thanks to fucking hdf5 being the most crappy
                     * designed library I ever came accross: 1): dataset
                     * contains variable length strings 2): dataset contains
                     * fixed size strings
                     *
                     * logic:
                     *       - check if we have a stringtype
                     *       - make a variable length stringtype
                     *       - check if the type of the dataset is  varlen
                     * string
                     *           - yes:
                     *               - read into char** buffer,
                     *               - then put into container<std::string>
                     *           - no:
                     *               - get size of type
                     *               - make string (=> char array) of size
                     * bufferlen*typesize
                     *               - read into it
                     *               - split the long string each typesize chars
                     * -> get entries
                     *               - put them into final buffer
                     * Mind that the buffer is preallocated to the correct size
                     */
                    HDFType vlentype;
                    vlentype.open< std::string >("vlentype_temporary", 0ul);

                    if (H5Tequal(vlentype.get_C_id(), _type.get_C_id()))
                    {
                        this->_log->debug(
                            "... nested type of variable length type ...");

                        std::vector< char* > temp_buffer(buffer.size());
                        herr_t               err = H5Dread(get_C_id(),
                                             _type.get_C_id(),
                                             _memspace.get_C_id(),
                                             _filespace.get_C_id(),
                                             H5P_DEFAULT,
                                             &temp_buffer[0]);

                        /* README:
                        - hdf5 uses `NULL` as fill value for string entries
                        which are not written per default, and setting another
                        fillvalue did not succeed for variable length data.
                        - The NULL produces a segmentation fault when trying to
                          turn it into an std::string.
                        - Hence, as a workaround, teh `NULL`s are treated
                        explicitly when postprocessing the data into their final
                        form, which is what the code below does.
                        */
                        for (auto [b, tb] = std::make_tuple(
                                 buffer.begin(), temp_buffer.begin());
                             b != buffer.end();
                             ++b, ++tb)
                        {
                            if (*tb != NULL)
                            {
                                *b = *tb;
                            }
                            else
                            {
                                *b = "\0";
                            }
                        }
                        return err;
                    }
                    else
                    {

                        this->_log->debug(
                            "... nested type of fixed length type ...");

                        // get size of the type, set up intermediate string
                        // buffer, adjust its size
                        auto        s = _type.size() / sizeof(char);
                        std::string temp_buffer;

                        temp_buffer.resize(buffer.size() * s);

                        // actual read
                        herr_t err = H5Dread(get_C_id(),
                                             _type.get_C_id(),
                                             _memspace.get_C_id(),
                                             _filespace.get_C_id(),
                                             H5P_DEFAULT,
                                             &temp_buffer[0]);

                        // content of dataset is now one consectuive line of
                        // stuff in temp_buffer. Use read size s to cut out the
                        // strings we want. definitly not elegant and fast, but
                        // strings are ugly to work with in general, and this is
                        // the most simple solution I can currently come up with

                        std::size_t i       = 0;
                        std::size_t buffidx = 0;
                        while (i < temp_buffer.size())
                        {
                            buffer[buffidx] = temp_buffer.substr(i, s);
                            i += s;
                            buffidx += 1;
                        }

                        // return
                        return err;
                    }
                }
            }
            // variable length arrays
            else if (_type.type_category() == H5T_VLEN)
            {
                this->_log->debug(
                    "... nested type of variable length array type ... ");

                std::vector< hvl_t > temp_buffer(buffer.size());

                herr_t err = H5Dread(get_C_id(),
                                     _type.get_C_id(),
                                     _memspace.get_C_id(),
                                     _filespace.get_C_id(),
                                     H5P_DEFAULT,
                                     temp_buffer.data());

                // turn the varlen buffer into the desired type
                // Cumbersome, but necessary...

                this->_log->debug("... transforming the read data to the "
                                  "actually desired type ... ");

                for (std::size_t i = 0; i < buffer.size(); ++i)
                {
                    if constexpr (!Utils::is_array_like_v< value_type_1 >)
                    {
                        buffer[i].resize(temp_buffer[i].len);
                    }

                    // I consider this more elegant than using std::for_each
                    // and defining the 'j' index outside of the predicate
                    for (auto [it, j] =
                             std::make_tuple(std::begin(buffer[i]), 0ul);
                         it != std::end(buffer[i]);
                         ++it, ++j)
                    {
                        *it = static_cast< value_type_2* >(temp_buffer[i].p)[j];
                    }
                }
                return err;
            }
            else
            {
                throw std::runtime_error(
                    "Dataset " + _path +
                    ": Unknown kind of datatype in dataset when requesting to "
                    "read into container");
            }
        }

        else // no nested container or container of strings, but one containing
             // simple types
        {
            this->_log->debug("... no nested type to read");
            return H5Dread(get_C_id(),
                           _type.get_C_id(),
                           _memspace.get_C_id(),
                           _filespace.get_C_id(),
                           H5P_DEFAULT,
                           buffer.data());
        }
    }

    /// read attirbute data which contains a single string.
    /** \details this is always read into std::strings, and hence we can use
     *         'resize'
     */
    template < typename Type >
    auto
    __read_stringtype__(Type& buffer)
    {
        this->_log->debug("Reading string data from dataset {}...", _path);

        buffer.resize(buffer.size() * _type.size());
        // read data
        return H5Dread(get_C_id(),
                       _type.get_C_id(),
                       _memspace.get_C_id(),
                       _filespace.get_C_id(),
                       H5P_DEFAULT,
                       buffer.data());
    }

    /// read pointertype.
    /** \details Either this is given by the user, or it is assumed to be 1d,
     *         thereby flattening Nd attributes
     */
    template < typename Type >
    auto
    __read_pointertype__(Type buffer)
    {
        this->_log->debug("Reading pointer data from dataset {}...", _path);

        return H5Dread(get_C_id(),
                       _type.get_C_id(),
                       _memspace.get_C_id(),
                       _filespace.get_C_id(),
                       H5P_DEFAULT,
                       buffer);
    }

    /// read scalar type, trivial
    template < typename Type >
    auto
    __read_scalartype__(Type& buffer)
    {
        this->_log->debug("Reading scalar data from dataset {}...", _path);

        return H5Dread(get_C_id(),
                       _type.get_C_id(),
                       _memspace.get_C_id(),
                       _filespace.get_C_id(),
                       H5P_DEFAULT,
                       &buffer);
    }

    /// write out the attribute buffer
    void
    __write_attribute_buffer__()
    {
        auto log = spdlog::get("data_io");

        log->debug("Writing attribute buffer of  dataset {}...", _path);

        // do nothing if the buffer is empty;
        if (_attribute_buffer.size() == 0)
        {
            return;
        }

        // write out the attributes from the attribute buffer.
        for (auto& [path, variant] : _attribute_buffer)
        {
            log->debug("... currently at attribute {}", path);

            HDFAttribute attr(static_cast< Base& >(*this), path);

            // Use visiting syntax on the variant to write the attribute value
            std::visit(
                // this is a universal reference and hence perfect
                // forwarding can be employed via std::forward
                [&attr](auto&& arg) {
                    attr.write(std::forward<
                               std::remove_reference_t< decltype(arg) > >(arg));
                },
                variant);
        }

        // free up memory.
        _attribute_buffer.clear();
    }

    /**
     *  @brief Identifier of the parent object
     */
    HDFIdentifier _parent_identifier;

    /**
     *  @brief number of dimensions of the dataset
     */
    hsize_t _rank;

    /**
     *  @brief the currently occupied size of the dataset in number of elements
     */
    std::vector< hsize_t > _current_extent;

    /**
     * @brief  the maximum number of elements which can be stored in the dataset
     */
    std::vector< hsize_t > _capacity;

    /**
     * @brief the chunksizes per dimensions if dataset is extendible or
     * compressed
     */
    std::vector< hsize_t > _chunksizes;

    /**
     * @brief offset of the data
     *
     */
    std::vector< hsize_t > _offset;

    /**
     * @brief buffer for extent update
     *
     */
    std::vector< hsize_t > _new_extent;

    /**
     * @brief the level of compression, 0 to 10
     */
    std::size_t _compress_level;

    /**
     * @brief  A buffer for storing attributes before the dataset exists
     *
     * @details A vector holding very large variant types to store attributes
     *         in before the dataset exists physically.
     *         The string in the held pairs is for path of the attribute, the
     *         variant for the data.
     */
    std::vector< std::pair< std::string, typename HDFType::Variant > >
        _attribute_buffer;

    /**
     * @brief Type of the data the dataset holds
     *
     */
    HDFType _type;

    /**
     * @brief file dataspace identifier
     *
     */
    HDFDataspace _filespace;

    /**
     * @brief memory dataspace identifier
     *
     */
    HDFDataspace _memspace;

  public:
    /**
     * @brief Base class alias
     *
     */
    using Base = HDFObject< HDFCategory::dataset >;

    /**
     * @brief Get the type object
     *
     * @return hid_t
     */
    auto
    get_type()
    {
        return _type;
    }

    /**
     * @brief Get the memory dataspace id
     *
     * @return hid_t
     */
    HDFDataspace
    get_memspace()
    {
        return _memspace;
    }

    /**
     * @brief Get the file dataspace id
     *
     * @return hid_t
     */
    HDFDataspace
    get_filespace()
    {
        return _filespace;
    }

    /**
     * @brief Returns the attribute buffer of this dataset
     */
    auto
    get_attribute_buffer()
    {
        return _attribute_buffer;
    }

    /**
     * @brief get a shared_ptr to the parent_object
     *
     * @return std::shared_ptr<HDFObject>
     */
    HDFIdentifier
    get_parent_id()
    {
        return _parent_identifier;
    }

    /**
     * @brief get the rank of the dataset, i.e. the dimensionality
     *
     * @return std::size_t
     */
    std::size_t
    get_rank()
    {
        return _rank;
    }

    /**
     * @brief get the current extend of the dataset
     *
     * @return std::vector<hsize_t>
     */
    auto
    get_current_extent()
    {
        return _current_extent;
    }

    /**
     * @brief Get the offset object
     *
     * @return std::vector<hsize_t>
     */
    auto
    get_offset()
    {
        return _offset;
    }
    /**
     * @brief get the maximum extend of the dataset
     *
     * @return std::vector<hsize_t>
     */
    auto
    get_capacity()
    {
        return _capacity;
    }

    /**
     * @brief Get the chunksizes vector
     *
     * @return auto
     */
    auto
    get_chunksizes()
    {
        return _chunksizes;
    }

    /**
     * @brief Get the compress level object
     *
     * @return auto
     */
    auto
    get_compresslevel()
    {
        return _compress_level;
    }

    /**
     * @brief Set the capacity object, and sets rank of dataset to capacity.size
     *
     * @param capacity
     */
    void
    set_capacity(std::vector< hsize_t > capacity)
    {
        if (is_valid())
        {
            throw std::runtime_error(
                "Dataset " + _path +
                ": Cannot set capacity after dataset has been created");
        }
        else
        {
            _rank     = capacity.size();
            _capacity = capacity;
        }
    }

    /**
     * @brief Set the chunksize object
     *
     * @param chunksizes
     */
    void
    set_chunksize(std::vector< hsize_t > chunksizes)
    {
        if (is_valid())
        {
            throw std::runtime_error(
                "Dataset " + _path +
                ": Cannot set chunksize after dataset has been created");
        }

        // if chunksizes = {} then it will be automatically determined
        if (chunksizes.size() != _rank and chunksizes.size() != 0)
        {
            throw std::runtime_error(
                "Dataset " + _path +
                ": Chunksizes size has to be equal to dataset rank");
        }

        _chunksizes = chunksizes;
    }

    /**
     * @brief      add attribute to the dataset
     *
     * @details     If the dataset is not opened already, the attribute is
     *             stored in the _attribute_buffer and written on close.
     *
     * @note       Attributes stored when the dataset was not yet opened will
     *             only become available after the dataset was closed.
     *
     * @param      attribute_path  The attribute path
     * @param      data  The attribute data
     *
     * @tparam     Attrdata        The type of the attribute data
     */
    template < typename Attrdata >
    void
    add_attribute(std::string attribute_path, Attrdata data)
    {
        // Can only write directly, if the dataset is valid
        if (is_valid())
        {
            this->_log->debug(
                "Add attribute {} to valid dataset {}", attribute_path, _path);
            // make attribute and write
            HDFAttribute attr(*this, attribute_path);
            attr.write(data);
        }
        else
        {

            this->_log->debug("Add atttribute {} to attribute buffer of {} "
                              "because it has not yet been created on disk",
                              attribute_path,
                              _path);
            // The dataset was not opened yet. Need to write to buffer

            // For non-vector container data, need to convert to vector
            if constexpr (Utils::is_container_v< Attrdata >)
            {
                if constexpr (not std::is_same_v<
                                  std::vector< typename Attrdata::value_type >,
                                  Attrdata >)
                {
                    // Make it a vector and write to buffer
                    _attribute_buffer.push_back(std::make_pair(
                        attribute_path,
                        std::vector< typename Attrdata::value_type >(
                            std::begin(data), std::end(data))));
                }
                else
                {
                    // Can write directly
                    _attribute_buffer.push_back(
                        std::make_pair(attribute_path, data));
                }
            }
            else
            {
                // Can write directly
                _attribute_buffer.push_back(
                    std::make_pair(attribute_path, data));
            }
        }
    }

    /**
     * @brief      Close the dataset
     *
     * @details     This function is called by the destructor and also takes
     *             care that the attribute buffer is written out, ensuring that
     *             a correctly closed dataset contains all specified attributes
     */
    void
    close()
    {
        auto log = spdlog::get("data_io");

        // write the attributebuffer out
        if (is_valid())
        {
            __write_attribute_buffer__();
        }

        // employ the object base class' close function to close the dataset,
        // then write attributes and close the filespaces
        Base::close();

        // close dataspaces
        _filespace.close();
        _memspace.close();
        _type.close();
    }

    /**
     * @brief Open the dataset in parent_object with relative path 'path'.
     *
     * @param parent_object The HDFGroup/HDFFile into which the dataset shall be
     * created
     * @param path The path of the dataset in the parent_object
     * @param capacity The maximum size of the dataset in each dimension. Give
     *                 H5S_UNLIMITED if unlimited size is desired. Then you have
     *                 to give chunksizes.
     * @param chunksize The chunksizes in each dimension to use
     * @param compress_level The compression level to use, 0 to 10 (0 = no
     * compression, 10 highest compression)
     */
    template < HDFCategory cat >
    void
    open(const HDFObject< cat >& parent_object,
         std::string             path,
         std::vector< hsize_t >  capacity       = {},
         std::vector< hsize_t >  chunksizes     = {},
         hsize_t                 compress_level = 0)
    {

        this->_log->debug(
            "Opening dataset {} within {}", path, parent_object.get_path());

        open(parent_object.get_id_object(),
             path,
             capacity,
             chunksizes,
             compress_level);
    }

    /**
     * @brief Open the dataset in parent_object with relative path 'path'.
     *
     * @param parent_object The HDFIdentifier  of the object into which the
     * dataset shall be created
     * @param path The path of the dataset in the parent_object
     * @param capacity The maximum size of the dataset in each dimension. Give
     *                 H5S_UNLIMITED if unlimited size is desired. Then you have
     *                 to give chunksizes.
     * @param chunksize The chunksizes in each dimension to use
     * @param compress_level The compression level to use, 0 to 10 (0 = no
     * compression, 10 highest compression)
     */
    void
    open(const HDFIdentifier&   parent_identifier,
         std::string            path,
         std::vector< hsize_t > capacity       = {},
         std::vector< hsize_t > chunksizes     = {},
         hsize_t                compress_level = 0)
    {

        if (not parent_identifier.is_valid())
        {
            throw std::runtime_error("parent id not valid for dataset " + path);
        }

        _parent_identifier = parent_identifier;
        _path              = path;

        _filespace.close();
        _memspace.close();
        // open with H5S_ALL
        _filespace.open();
        _memspace.open();
        // Try to find the dataset in the parent_object
        // If it is there, open it.
        // Else: postphone the dataset creation to the first write
        // the attribute buffer has to be written in both cases,
        // as its existence is independent from the existence of the
        // dataset in the file. We could use a dataset object repeatedly
        // to represent different datasets in the file via calling close
        // and open over and over, writing attributes to it while
        // it is closed. Therefore, the attribute buffer is written
        // out at the end of this function
        if (path_is_valid(_parent_identifier.get_id(), _path.c_str()))
        { // dataset exists
            // open it

            this->_log->debug("... binding existing dataset to object");

            bind_to(H5Dopen(_parent_identifier.get_id(),
                            _path.c_str(),
                            H5P_DEFAULT),
                    &H5Dclose);

            _type.close();
            _type.open(*this);

            // get dataspace and read out rank, extend, capacity
            _filespace.open(*this);

            _rank = _filespace.rank();

            _chunksizes.resize(_rank, 0);
            // get chunksizes
            hid_t creation_plist = H5Dget_create_plist(get_C_id());
            hid_t layout         = H5Pget_layout(creation_plist);
            if (layout == H5D_CHUNKED)
            {
                herr_t err =
                    H5Pget_chunk(creation_plist, _rank, _chunksizes.data());
                if (err < 0)
                {
                    throw std::runtime_error(
                        "Dataset " + _path +
                        ": Error in reading out chunksizes while opening.");
                }
            }
            H5Pclose(creation_plist);

            // temporary workaround for type inconsistentcy:
            // arma::row used by dataspace and std::vector by dataset, and
            // chunksize algo
            auto [size, capacity] = _filespace.get_properties();

            _current_extent.assign(size.begin(), size.end());
            _capacity.assign(capacity.begin(), capacity.end());
            _offset = _current_extent;
        }
        else
        {
            this->_log->debug("... dataset not yet existing, have to wait 'til "
                              "data becomes available");

            // it is not expected that the _attribute_buffer will become big
            // and reallocate often, hence a reserve is foregone here,
            // which one might otherwise consider.
            // The size to reserve would be a rather wild guess however.
            if (capacity.size() == 0)
            {
                _rank     = 1;
                _capacity = std::vector< hsize_t >(_rank, H5S_UNLIMITED);
                _offset   = std::vector< hsize_t >(_rank, 0);
            }
            else
            {
                _capacity = capacity;
                _rank     = _capacity.size();
                _offset   = std::vector< hsize_t >(_rank, 0);
            }

            // if chunksizes is given, everything is fine, if not, it is empty
            // here and we will check in write method if calculation of
            // chunksize is needed
            _chunksizes = chunksizes;

            _compress_level = compress_level;

            _id.set_id(-1);
        }
    }

    /**
     * @brief      swap the state of the objects
     *
     * @param      other  The other
     */
    void
    swap(HDFDataset& other)
    {
        using std::swap;
        using Utopia::DataIO::swap;
        swap(static_cast< Base& >(*this), static_cast< Base& >(other));
        swap(_parent_identifier, other._parent_identifier);
        swap(_rank, other._rank);
        swap(_current_extent, other._current_extent);
        swap(_capacity, other._capacity);
        swap(_chunksizes, other._chunksizes);
        swap(_offset, other._offset);
        swap(_new_extent, other._new_extent);
        swap(_compress_level, other._compress_level);
        swap(_attribute_buffer, other._attribute_buffer);
        swap(_filespace, other._filespace);
        swap(_memspace, other._memspace);
        swap(_type, other._type);
    }

    /**
     * @brief Writes data of arbitrary type
     *
     * @tparam T automatically determined
     * @param data data to write
     * @param shape shape array, only useful currently if pointer data given
     */
    template < typename T >
    void
    write(T&& data, [[maybe_unused]] std::vector< hsize_t > shape = {})
    {
        this->_log->debug("Writing data to dataset {}", _path);
        this->_log->debug("... current extent {}", Utils::str(_current_extent));
        this->_log->debug("... current offset {}", Utils::str(_offset));
        this->_log->debug("... capacity {}", Utils::str(_capacity));
        this->_log->debug("... refcount {}", get_refcount());

        // dataset does not yet exist
        _memspace.close();
        _filespace.close();

        _memspace.open();
        _filespace.open();

        if (not is_valid())
        {
            // current limitation removed in future
            if (_rank > 2)
            {
                throw std::runtime_error("Rank > 2 not supported");
            }

            /*
            if dataset does not yet exist
                 Get current extend.
                 If is container:
                    if 1d:
                        current_extent = data.size()
                    else:
                        current_extent = {1, data.size()}, i.e one line in
            matrix

                if pointer:
                    current_extent is shape
                if string or scalar:
                    current_extent is 1

                then check if chunking is needed but not known and calculate it
            or throw error. this is done within the individual __write_X__
            methods because detailed type info is needed.
            */
            _current_extent.resize(_rank);

            if constexpr (Utils::is_container_v< T >)
            {
                if (_rank == 1)
                {
                    _current_extent[_rank - 1] = data.size();
                }
                else
                {
                    _current_extent[0] = 1;
                    _current_extent[1] = data.size();
                }
            }

            else if constexpr (std::is_pointer_v< T > and
                               !Utils::is_string_v< T >)
            {
                if (shape.size() == 0)
                {
                    throw std::runtime_error(
                        "Dataset " + _path +
                        ": shape has to be given explicitly when writing "
                        "pointer types");
                }
                _current_extent = shape;
            }
            else
            {
                _current_extent[_rank - 1] = 1;
            }
        }
        else
        {

            /*
            if dataset does  exist:
                - check if the type of the data given to write is compatible
            with the one of the dataset

                - make _new_extent array equalling current_extent, leave
            current_extent If is container: if 1d: _new_extent = current_extent
            + data.size() else: _new_extent = {current_extent[0]+1,
            current_extent[1]}, i.e one new line in matrix

                if pointer:
                    current_extent += shape
                if string or scalar:
                    current_extent += 1


                offset = current_extent
                buf if 2d and current_extent[1]==capacity[1](end of line):
                    offset = {current_extent[0]+1, 0};

                count = {1, data.size} if 2d, {data.size()} if 1d

                then extent data,
                select newly added line
                update current_ex
                write
            */

            // make a temporary for new extent
            std::vector< hsize_t > _new_extent = _current_extent;

            if (_capacity == _current_extent)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error, dataset cannot be extended "
                                         "because it reached its capacity");
            }
            else
            {
                // set offset array
                // this is needed because multiple writes one after the other
                // could occur without intermediate close and reopen (which
                // would set _offset correctly)
                _offset = _current_extent;

                if (_rank > 1)
                {
                    if (_current_extent[1] == _capacity[1])
                    {
                        _offset[1] = 0;
                    }
                }

                // if data is a container, then we have to add its size to
                // extend, if it is a pointer, we have to add the pointers
                // shape, else we have to add 1 because we either write
                // a single scalar or string

                if constexpr (Utils::is_container_v< T >)
                {
                    if (_rank == 1)
                    {
                        _new_extent[0] += data.size();
                    }
                    else
                    {
                        _new_extent[0] += 1;
                    }
                }
                else if constexpr (std::is_pointer_v< T > and
                                   !Utils::is_string_v< T >)
                {
                    if (shape.size() == 0)
                    {
                        throw std::runtime_error(
                            "Dataset " + _path +
                            ": shape has to be given explicitly when writing "
                            "pointer types");
                    }

                    for (std::size_t i = 0; i < _rank; ++i)
                    {
                        _new_extent[i] += shape[i];
                    }
                }
                else
                {
                    if (_rank == 1)
                    {
                        // if rank is one we can only extend into one direction
                        _new_extent[0] += 1;
                    }
                    else
                    {
                        // first fill row, then column wise increase
                        if (_current_extent[0] < _capacity[0])
                        {
                            _new_extent[0] += 1;
                        }
                        // if row is full, start a new one
                        else
                        {
                            _new_extent[1] += 1;
                        }
                    }
                }
            }
            // select counts for dataset
            // this has to be generalized and refactored
            std::vector< hsize_t > counts(_rank, 0);
            if constexpr (Utils::is_container_v< T >)
            {
                if (_rank == 1)
                {
                    counts = { data.size() };
                }
                else
                {
                    counts = { 1, data.size() };
                }
            }
            // when is pointer, the counts are given by shape
            else if constexpr (std::is_pointer_v< T > and
                               !Utils::is_string_v< T >)
            {
                counts = shape;
            }
            else
            {
                counts = { 1 };
            }

            // extent the dataset
            for (std::size_t i = 0; i < _rank; ++i)
            {
                if (_new_extent[i] > _capacity[i])
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Cannot append data, "
                                             "_new_extent larger than capacity "
                                             "in dimension " +
                                             std::to_string(i));
                }
            }

            // extend the dataset to the new size
            herr_t err = H5Dset_extent(get_C_id(), _new_extent.data());

            if (err < 0)
            {
                throw std::runtime_error(
                    "Dataset " + _path +
                    ": Error when trying to increase extent");
            }

            // get file and memory spaces which represent the selection to write
            // at
            _filespace.open(*this);

            _memspace.open(_path + "memory dataspace", _rank, counts, {});

            _filespace.select_slice(_offset, // start
                                    arma::Row< hsize_t >(_offset) +
                                        arma::Row< hsize_t >(counts), // end
                                    {}                                // stride
            );

            _current_extent = _new_extent;
        }

        this->_log->debug("New extent {}", Utils::str(_new_extent));
        this->_log->debug("New offset {}", Utils::str(_offset));
        this->_log->debug(" Refcount before write {}", get_refcount());

        // everything is prepared, we can write the data
        if constexpr (Utils::is_container_v< std::decay_t< T > >)
        {

            herr_t err = __write_container__(std::forward< T >(data));

            if (err < 0)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error in appending container");
            }
        }
        else if constexpr (Utils::is_string_v< std::decay_t< T > >)
        {
            herr_t err = __write_stringtype__(std::forward< T >(data));
            if (err < 0)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error in appending string");
            }
        }
        else if constexpr (std::is_pointer_v< std::decay_t< T > > and
                           !Utils::is_string_v< std::decay_t< T > >)
        {
            herr_t err = __write_pointertype__(std::forward< T >(data));
            if (err < 0)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error in appending pointer");
            }
        }
        else
        {
            herr_t err = __write_scalartype__(std::forward< T >(data));
            if (err < 0)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error in appending scalar");
            }
        }

        this->_log->debug(
            "Dataset {} 's refcount before writing topology attributes: {}",
            _path,
            get_refcount());

        // this adds information about the shape and properties of the dataset
        __add_topology_attributes__();

        this->_log->debug(
            "Dataset {} 's refcount after writing topology attributes: {}",
            _path,
            get_refcount());
    }

    /**
     * @brief Write function for writing iterator ranges [start, end), in
     * accordance with respective stl pattern
     *
     * @tparam Iter automatically determined
     * @tparam Adaptor automatically determined
     * @param begin start iterator of range to write
     * @param end end iteator of range to write
     * @param adaptor Modifier function which takes a reference of type
     * Iter::value_type and returns some arbitrary type, from which a buffer is
     * made which then is written to the dataset. This hence determines what is
     * written to the dataset
     */
    template < typename Iter, typename Adaptor >
    void
    write(Iter begin, Iter end, Adaptor&& adaptor)
    {
        this->_log->debug("Writing iterator range to dataset {}", _path);
        // this->_log->debug("... current offset {}", Utils::str(_offset));
        // this->_log->debug("... capacity {}", Utils::str(_capacity));

        using Type = Utils::remove_qualifier_t< decltype(adaptor(*begin)) >;

        write([&]() {
            std::vector< Type > buff(std::distance(begin, end));

            std::generate(buff.begin(), buff.end(), [&begin, &adaptor]() {
                return adaptor(*(begin++));
            });
            return buff;
        }());
    }

    /**
     * @brief Write a boost::multi_array of arbitrary type and dimension to the
     * dataset. The dataset needs to be of dimension N >= d, because dataset
     * dimensions cannot be changed after they have been created. In all other
     * regards this behaves like the normal 'write' function that accepts a
     * value.
     *
     * @warning When no custom offset vector is given, and one reuses the
     * dataset for multiple writes, it is assumed that the size of the data
     * written varies only in the first dimension. Envisage this as stacking
     * rectangular blocks of varying height but equal width and depth.
     * The reason is that it is rather difficult to automatically
     * determine the offset such that the user can do arbitrary writes
     * without any overwrites of existing data or storage inefficiencies
     * occuring.
     * @tparam T type held by the boost::multi_array, automatically determined
     * from argument
     * @tparam d dimensionality of the boost::multi_array, automatically
     * determined
     * @param data boost::multi_array to be written to dataset.
     * @param offset optional custom offset, which gives the element the zeroth
     * entry of the newly written array shall be put to.
     */
    template < typename T, std::size_t d >
    void
    write_nd(const boost::multi_array< T, d >& data,
             std::vector< hsize_t >            offset = {})
    {
        this->_log->debug("Writing N-dimensional dataset to dataset {}", _path);
        this->_log->debug("... current extent {}", Utils::str(_current_extent));
        this->_log->debug("... current offset {}", Utils::str(_offset));
        this->_log->debug("... capacity {}", Utils::str(_capacity));

        _filespace.close();
        _memspace.close();

        // create dataspaces
        _memspace.open();
        _filespace.open();

        // dataset does not yet exist
        if (not is_valid())
        {

            // two possibilities: capacity given or not:
            // if not given:
            //   use data to determine extent and capacity, correcting the
            //   assumed ones from 'open'
            // else use given values

            if (_capacity == std::vector< hsize_t >{ H5S_UNLIMITED } and
                _rank == 1)
            {
                _rank = d;
                _current_extent.resize(_rank, 0);
                _offset.resize(_rank, 0);
                for (std::size_t i = 0; i < _rank; ++i)
                {
                    _current_extent[i] = data.shape()[i];
                }
                _capacity.resize(d, H5S_UNLIMITED);
            }
            else
            {
                _current_extent.resize(_rank, 1);
                _offset.resize(_rank, 0);

                for (auto [i, j] = std::make_tuple(_rank - d, 0); i < _rank;
                     ++i, ++j)
                {
                    _current_extent[i] = data.shape()[j];
                }
            }

            _log->debug("Dataset {} does not exist yet, properties were "
                        "determined to be",
                        _path);
            _log->debug(" rank: {}", Utils::str(_capacity));
            _log->debug(" datarank: {}", Utils::str(d));
            _log->debug(" datashape: {}",
                        Utils::str(std::vector< std::size_t >(
                            data.shape(), data.shape() + d)));
            _log->debug(" capacity: {}", Utils::str(_capacity));
            _log->debug(" offset: {}", Utils::str(_offset));
            _log->debug(" current_extent: {}", Utils::str(_current_extent));
        }
        else
        {

            if (_rank < d)
            {
                throw std::invalid_argument(
                    "Error, the dimensionality of the dataset, which is " +
                    std::to_string(_current_extent.size()) +
                    ", must be >=  the dimensionality of the data to be "
                    "written, which is " +
                    std::to_string(d));
            }
            else
            {

                _log->debug("Dataset {} does  exist", _path);
                _log->debug("Properties of data to be written");
                _log->debug(" datarank: {}", Utils::str(d));
                _log->debug(" datashape: {}",
                            Utils::str(std::vector< std::size_t >(
                                data.shape(), data.shape() + d)));

                _log->debug(
                    "Properties before change for accomodating new data");
                _log->debug(" rank: {}", Utils::str(_capacity));
                _log->debug(" capacity: {}", Utils::str(_capacity));
                _log->debug(" offset: {}", Utils::str(_offset));
                _log->debug(" current_extent: {}", Utils::str(_current_extent));

                std::vector< hsize_t > _new_extent = _current_extent;

                // two cases: When offset is given, and when not. When it is
                // given, then it is assumed that the data has always the same
                // shape except in the first dimension

                if (offset.size() != 0)
                {

                    // when offset is given we use it to
                    // determine how to extent the dataset. Note that the
                    // requirement that all data written have the same shape in
                    // all dimensions but the first is not enforced here, hence
                    // the algorithm works a little differently
                    _offset = offset;
                    for (std::size_t i = 0; i < _rank - d; ++i)
                    {
                        if (_offset[i] == _current_extent[i])
                        {
                            _new_extent[i] += 1;
                        }
                    }

                    for (auto [i, j] = std::make_tuple(_rank - d, 0ul); i < d;
                         ++i)
                    {
                        if (_current_extent[i] < (_offset[i] + data.shape()[j]))
                        {
                            _new_extent[i] = _offset[i] + data.shape()[j];
                        }
                        if (_new_extent[i] > _capacity[i])
                        {
                            throw std::runtime_error(
                                "Dataset " + _path + ": Capacity[" +
                                std::to_string(i) +
                                "] = " + std::to_string(_capacity[i]) +
                                ", which is too small for a desired new "
                                "extent[" +
                                std::to_string(i) +
                                "] = " + std::to_string(_new_extent[i]));
                        }
                    }

                    // extend the dataset to the new size
                    herr_t err = H5Dset_extent(get_C_id(), _new_extent.data());

                    if (err < 0)
                    {
                        throw std::runtime_error(
                            "Dataset " + _path +
                            ": Error when trying to increase extent");
                    }
                }

                else
                {
                    // zeroth index is treated separatlye because it is used to
                    // increase the total available space in the dataset

                    _new_extent[0] += (d == _rank) ? data.shape()[0]
                                                   : 1; // add all needed slices

                    if (_new_extent[0] > _capacity[0])
                    {
                        throw std::runtime_error(
                            "Error in " + _path + ", capacity " +
                            std::to_string(_capacity[0]) + " at index " +
                            std::to_string(0) + " of " + std::to_string(d) +
                            " is too small for new extent " +
                            std::to_string(_new_extent[0]));
                    }

                    for (auto [i, j] =
                             std::make_tuple(1ul, (d == _rank) ? 1ul : 0ul);
                         i < _rank && j < d;
                         ++i, ++j)
                    {
                        if (data.shape()[j] > _current_extent[i])
                        {
                            _new_extent[i] +=
                                data.shape()[j] - _current_extent[i];
                            if (_new_extent[i] > _capacity[i])
                            {
                                throw std::runtime_error(
                                    "Error in " + _path +
                                    ", capacity at index " + std::to_string(i) +
                                    " of " + std::to_string(d) +
                                    " is too small");
                            }
                        }
                    }

                    // extend the dataset to the new size
                    herr_t err = H5Dset_extent(get_C_id(), _new_extent.data());

                    if (err < 0)
                    {
                        throw std::runtime_error(
                            "Dataset " + _path +
                            ": Error when trying to increase extent");
                    }

                    // if the algo progresses until here, it is safe to do this;

                    _offset.resize(_rank);
                    std::fill(_offset.begin(), _offset.end(), 0);
                    _offset[0] = _current_extent[0];
                }
                /*
                 * README: The count vector is needed for determining the slice
                 * to write to in the datafile. hdf5 determines slices in the
                 * dataset via [start, step, count], pattern, where the 'count'
                 * gives the number fo steps in each dimension. Hence, the
                 * counts have to be computed/assigned from the data to be
                 * written
                 */
                std::vector< hsize_t > counts(_rank, 1);

                for (auto [i, j] = std::make_tuple(_rank - d, 0); i < _rank;
                     ++i, ++j)
                {
                    counts[i] = data.shape()[j];
                }

                // get file and memory spaces which represent the selection to
                // write at
                _filespace.close();
                _memspace.close();
                _filespace.open(*this);

                _memspace.open(_path + "memory dataspace", _rank, counts, {});

                _filespace.select_slice(_offset,
                                        arma::Row< hsize_t >(_offset) +
                                            arma::Row< hsize_t >(counts),
                                        {});

                // update the current extent
                _current_extent = _new_extent;

                _log->debug(
                    "Properties after change for accomodating new data");
                _log->debug(" rank: {}", Utils::str(_capacity));
                _log->debug(" capacity: {}", Utils::str(_capacity));
                _log->debug(" offset: {}", Utils::str(_offset));
                _log->debug(" current_extent: {}", Utils::str(_current_extent));
                _log->debug("new extent {}", Utils::str(_new_extent));
            }
        }

        // dataset extension is done, now we can check if we have to buffer data
        // FIXME: this has to be put into the bufferfactory class later, ideally
        // using a plain char buffer for it to avoid templating and enabling
        // memory reuse by making the bufferfactory a member.

        if constexpr (std::is_scalar_v< T >)
        {
            if (not is_valid())
            {
                __create_dataset__< std::decay_t< T > >(0);
            }
            else
            {
                HDFType temp_type;
                temp_type.open< std::decay_t< T > >("testtype", 0);
                if (_type != temp_type)
                {
                    throw std::runtime_error("Error, cannot write data of a "
                                             "different type into dataset " +
                                             _path);
                }
            }

            herr_t err = H5Dwrite(get_C_id(),
                                  _type.get_C_id(),
                                  _memspace.get_C_id(),
                                  _filespace.get_C_id(),
                                  H5P_DEFAULT,
                                  data.data());

            if (err < 0)
            {
                throw std::runtime_error(
                    "Dataset " + _path +
                    ": Error in writing nd-array holding scalar values");
            }
        }
        else if constexpr (Utils::is_string_v< T >)
        {
            if (not is_valid())
            {
                __create_dataset__< std::decay_t< T > >(0);
            }
            else
            {
                HDFType temp_type;
                temp_type.open< std::decay_t< T > >("testtype", 0);

                if (_type != temp_type)
                {
                    throw std::runtime_error("Error, cannot write data of a "
                                             "different type into dataset " +
                                             _path);
                }
            }
            // make a buffer that mirrors the shape of the data
            boost::multi_array< const char*, d > buffer(
                reinterpret_cast< boost::array< size_t, d > const& >(
                    *data.shape()));

            // fill the buffer
            std::transform(data.data(),
                           data.data() + data.num_elements(),
                           buffer.data(),
                           [](auto&& str) { return str.c_str(); });

            // write the buffer
            herr_t err = H5Dwrite(get_C_id(),
                                  _type.get_C_id(),
                                  _memspace.get_C_id(),
                                  _filespace.get_C_id(),
                                  H5P_DEFAULT,
                                  buffer.data());

            if (err < 0)
            {
                throw std::runtime_error(
                    "Dataset " + _path +
                    ": Error in writing nd-array holding string values");
            }
        }
        else if constexpr (Utils::is_container_v< T >)
        {
            if constexpr (Utils::is_array_like_v< T >)
            {
                hsize_t typesize = Utils::get_size< T >::value;

                // create dataset with given typesize
                if (not is_valid())
                {
                    __create_dataset__< std::decay_t< T > >(typesize);
                }
                else
                {
                    HDFType temp_type;
                    temp_type.open< std::decay_t< T > >("testtype", typesize);
                    if (_type != temp_type)
                    {
                        throw std::runtime_error(
                            "Error, cannot write data of a "
                            "different type into dataset " +
                            _path);
                    }
                }

                // write the buffer not needed here
                herr_t err = H5Dwrite(get_C_id(),
                                      _type.get_C_id(),
                                      _memspace.get_C_id(),
                                      _filespace.get_C_id(),
                                      H5P_DEFAULT,
                                      data.data());
                if (err < 0)
                {
                    throw std::runtime_error(
                        "Dataset " + _path +
                        ": Error in writing nd-array holding array values");
                }
            }
            else
            {
                // create dataset with given typesize
                if (not is_valid())
                {
                    __create_dataset__< std::decay_t< T > >(0);
                }
                else
                {
                    HDFType temp_type;
                    temp_type.open< std::decay_t< T > >("temp_type", 0);
                    if (_type != temp_type)
                    {
                        throw std::runtime_error(
                            "Error, cannot write data of a "
                            "different type into dataset " +
                            _path);
                    }
                }
                // vector is stored
                if constexpr (std::is_same_v<
                                  std::vector< typename T::value_type >,
                                  std::decay_t< T > >)
                {
                    // make  buffer
                    boost::multi_array< hvl_t, d > buffer(
                        reinterpret_cast< boost::array< size_t, d > const& >(
                            *data.shape()));

                    std::transform(
                        data.data(),
                        data.data() + data.num_elements(),
                        buffer.data(),
                        [](auto&& v) {
                            return hvl_t{
                                v.size(),
                                // cumbersome const cast needed because I want
                                // to keep const Reference argument 'cause it
                                // can bind to lvalues and rvalues alike, i.e.
                                // you can construct a multi_array in the arg
                                // list, but also pass an existing one as
                                // reference.
                                const_cast< Utils::remove_qualifier_t< decltype(
                                    v.data()) >* >(v.data())
                            };
                        });

                    // write the buffer
                    herr_t err = H5Dwrite(get_C_id(),
                                          _type.get_C_id(),
                                          _memspace.get_C_id(),
                                          _filespace.get_C_id(),
                                          H5P_DEFAULT,
                                          buffer.data());

                    if (err < 0)
                    {
                        throw std::runtime_error("Dataset " + _path +
                                                 ": Error in writing nd-array "
                                                 "holding vector values");
                    }
                }
                // no vector is stored
                else
                {
                    // make  buffers, when no vector we need two of them,
                    // one to transform to vector which has contiguous storage
                    // which in turn is needed by hdf5, the other
                    // for turning the new vectors into hvl_t
                    boost::multi_array< std::vector< typename T::value_type >,
                                        d >
                        vector_buffer(reinterpret_cast<
                                      boost::array< size_t, d > const& >(
                            *data.shape()));

                    boost::multi_array< hvl_t, d > buffer(
                        reinterpret_cast< boost::array< size_t, d > const& >(
                            *data.shape()));

                    std::transform(
                        data.data(),
                        data.data() + data.num_elements(),
                        vector_buffer.data(),
                        [](auto&& v) {
                            return std::vector< typename T::value_type >(
                                v.begin(), v.end());
                        });

                    std::transform(vector_buffer.data(),
                                   vector_buffer.data() +
                                       vector_buffer.num_elements(),
                                   buffer.data(),
                                   [](auto&& v) {
                                       return hvl_t{ v.size(), v.data() };
                                   });

                    // write the buffer
                    herr_t err = H5Dwrite(get_C_id(),
                                          _type.get_C_id(),
                                          _memspace.get_C_id(),
                                          _filespace.get_C_id(),
                                          H5P_DEFAULT,
                                          buffer.data());

                    if (err < 0)
                    {
                        throw std::runtime_error(
                            "Dataset " + _path +
                            ": Error in writing nd-array holding non-vector "
                            "container values");
                    }
                }
            }
        }
    }

    /**
     * @brief  Read (a subset of ) a dataset into a buffer of type 'Type'.
     *         Type gives the type of the buffer to read, and currently only
     *         1d reads are supported, so ND dataset of double has to be read
     *         into a 1d buffer containing doubles of size = product of
     *         dimenions of datasets.
     *
     * @tparam Type  Type to read to.
     * @param start  offset to start reading from (inclusive)
     * @param end    where to stop reading (exclusive)
     * @param stride stride to use when reading -> like numpy stride
     * @return auto  Buffer of type 'Type' containing read elements
     */
    template < typename Type >
    auto
    read([[maybe_unused]] std::vector< hsize_t > start  = {},
         [[maybe_unused]] std::vector< hsize_t > end    = {},
         [[maybe_unused]] std::vector< hsize_t > stride = {})
    {
        this->_log->debug(
            "Reading dataset {}, starting at {}, ending at {}, using stride {}",
            _path,
            Utils::str(start),
            Utils::str(end),
            Utils::str(stride));

        if (not is_valid())
        {
            throw std::runtime_error("Dataset " + _path +
                                     ": Dataset id is invalid");
        }

        _filespace.close();
        _memspace.close();

        // variables needed for reading
        std::vector< hsize_t > readshape; // shape vector for read, either
                                          // _current_extent or another shape

        std::size_t size = 1;

        // read entire dataset
        if (start.size() == 0)
        {
            readshape = _current_extent;
            _filespace.close();
            _memspace.close();
            _filespace.open();
            _memspace.open();

            // make flattened size of data to read
            for (auto& s : readshape)
            {
                size *= s;
            }
        }
        // read [start, end) with steps given by stride in each dimension
        else
        {
            // throw error if ranks and shape sizes do not match
            if (start.size() != _rank or end.size() != _rank or
                stride.size() != _rank)
            {
                throw std::invalid_argument(
                    "Dataset " + _path +
                    ": start, end, stride have to be "
                    "same size as dataset rank, which is " +
                    std::to_string(_rank));
            }

            // set offset of current array to start
            _offset = start;

            // make count vector
            // exploit that hsize_t((end-start)/stride) cuts off decimal
            // places and thus results in floor((end-start)/stride) always.
            std::vector< hsize_t > count(start.size());

            // build the count array -> how many elements to read in each
            // dimension
            for (std::size_t i = 0; i < _rank; ++i)
            {
                count[i] = (end[i] - start[i]) / stride[i];
            }

            for (auto& s : count)
            {
                size *= s;
            }

            readshape = count;

            _filespace.close();
            _memspace.close();

            _filespace.open(*this);
            _memspace.open(_path + " memory dataspace", _rank, count, {});

            this->_log->debug("... selecting slice in filespace for dataset {}",
                              _path);
            _filespace.select_slice(start, end, stride);
        }

        // Below the actual reading happens

        // type to read in is a container type, which can hold containers
        // themselvels or just plain types.
        if constexpr (Utils::is_container_v< Type >)
        {
            Type   buffer(size);
            herr_t err = __read_container__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error reading container type ");
            }
            return std::make_tuple(readshape, buffer);
        }
        else if constexpr (Utils::is_string_v< Type >) // we can have string
                                                       // types too, i.e. char*,
                                                       // const char*,
                                                       // std::string
        {
            std::string buffer; // resized in __read_stringtype__ because this
                                // as a scalar
            buffer.resize(size);
            herr_t err = __read_stringtype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error reading string type ");
            }

            return std::make_tuple(readshape, buffer);
        }
        else if constexpr (std::is_pointer_v< Type > &&
                           !Utils::is_string_v< Type >)
        {
            std::shared_ptr< Utils::remove_qualifier_t< Type > > buffer(
                new Utils::remove_qualifier_t< Type >[size]);

            herr_t err = __read_pointertype__(buffer.get());

            if (err < 0)
            {
                std::runtime_error("Dataset " + _path +
                                   ": Error reading pointer type ");
            }
            return std::make_tuple(readshape, buffer);
        }
        else // reading scalar types is simple enough
        {
            Type   buffer(0);
            herr_t err = __read_scalartype__(buffer);
            if (err < 0)
            {
                std::runtime_error("Dataset " + _path +
                                   ": Error reading scalar type ");
            }
            return std::make_tuple(readshape, buffer);
        }
    }

    /**
     * @brief      default consturctor
     */
    HDFDataset() = default;

    /**
     * @brief      Copy constructor
     *
     * @param      other  The other
     */
    HDFDataset(const HDFDataset& other) = default;

    /**
     * @brief      Move constructor
     *
     * @param      other  The other
     */
    HDFDataset(HDFDataset&& other) = default;
    /**
     * @brief      Assignment operator
     *
     * @param      other  The other
     *
     * @return     HDFDataset&
     */
    HDFDataset&
    operator=(const HDFDataset& other) = default;

    /**
     * @brief Move assignment operator
     *
     * @param other
     * @return HDFDataset&
     */
    HDFDataset&
    operator=(HDFDataset&& other) = default;

    /**
     * @brief Construct a new HDFDataset object
     *
     * @param parent_object The HDFGroup/HDFFile into which the dataset shall be
     *                      created
     * @param adaptor The function which makes the data to be written from given
     *                 iterators
     * @param path The path of the dataset in the parent_object
     * @param rank The number of dimensions of the dataset
     * @param capacity The maximum size of the dataset in each dimension. Give
     *                 H5S_UNLIMITED if unlimited size is desired. Then you have
     *                 to give chunksizes.
     * @param chunksize The chunksizes in each dimension to use
     * @param compress_level The compression level to use
     */
    template < HDFCategory cat >
    HDFDataset(HDFObject< cat >&      parent_object,
               std::string            path,
               std::vector< hsize_t > capacity       = {},
               std::vector< hsize_t > chunksizes     = {},
               hsize_t                compress_level = 0)

    {
        open(parent_object, path, capacity, chunksizes, compress_level);
    }

    /**
     * @brief      Destructor
     */
    virtual ~HDFDataset()
    {
        close();
    }
}; // end of HDFDataset class

/**
 * @brief      Exchange state between lhs and rhs
 *
 * @param      lhs        The left hand side
 * @param      rhs        The right hand side
 *
 * @tparam     HDFObject  The type of the parent object
 */
void
swap(HDFDataset& lhs, HDFDataset& rhs)
{
    lhs.swap(rhs);
}

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia
#endif // UTOPIA_DATAIO_HDFDATASET_HH
