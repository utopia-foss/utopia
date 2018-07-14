/**
 * @brief This is the central file of the HDF5 dataIO module of Utopia and
 * provides a class for writing to, reading from and creating almost arbitrary
 * data to a dataset in a HDF5 file.
 * @file hdfdataset.hh
 */
#ifndef HDFDATASET_HH
#define HDFDATASET_HH

#include "hdfattribute.hh"
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"
#include "hdfutilities.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <numeric>
#include <unordered_map>
#include <utility>
namespace Utopia
{
namespace DataIO
{
/**
 * @brief      Class representing a HDFDataset, wich reads and writes
 * data and attributes
 *
 * @tparam     HDFObject  The type of the parent object
 */
template <class HDFObject>
class HDFDataset
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
    template <typename Datatype>
    hid_t __create_dataset__(std::size_t typesize)
    {
        hid_t dset = 0;
        // create group property list and (potentially) intermediate groups
        hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(group_plist, 1);

        // distinguish by chunksize; chunked dataset needed for compression
        if (_chunksizes.size() > 0)
        {
            // create creation property list, set chunksize and compress level
            hid_t plist = H5Pcreate(H5P_DATASET_CREATE);

            H5Pset_chunk(plist, _rank, _chunksizes.data());

            if (_compress_level > 0)
            {
                H5Pset_deflate(plist, _compress_level);
            }

            // make dataspace
            hid_t dspace =
                H5Screate_simple(_rank, _current_extend.data(), _capacity.data());
            // create dataset and return
            dset = H5Dcreate(_parent_object->get_id(), _path.c_str(),
                             HDFTypeFactory::type<Datatype>(typesize), dspace,
                             group_plist, plist, H5P_DEFAULT);
        }
        else
        {
            // make dataspace
            hid_t dspace =
                H5Screate_simple(_rank, _current_extend.data(), _capacity.data());

            // can create the dataset right away
            dset = H5Dcreate(_parent_object->get_id(), _path.c_str(),
                             HDFTypeFactory::type<Datatype>(typesize), dspace,
                             group_plist, H5P_DEFAULT, H5P_DEFAULT);
        }

        H5Oget_info(dset, &_info);
        _address = _info.addr;
        (*_referencecounter)[_address] = 1;
        return dset;
    }

    /**
     * @brief      Helper for selecting subset of dataset
     *
     * @param      offset  The offset
     * @param      stride  The stride
     * @param      block   The block
     * @param      count   The count
     *
     * @return     the dataset subset vector, consisting of dspace and memspace
     */
    std::pair<hid_t, hid_t> __select_dataset_subset__(std::vector<hsize_t> offset,
                                                      std::vector<hsize_t> stride,
                                                      std::vector<hsize_t> block,
                                                      std::vector<hsize_t> count)
    {
        // select the new slab we just added for writing.
        hid_t dspace = H5Dget_space(_dataset);
        hid_t memspace = H5Screate_simple(_rank, count.data(), NULL);

        herr_t select_err =
            H5Sselect_hyperslab(dspace, H5S_SELECT_SET, offset.data(),
                                stride.data(), count.data(), block.data());
        if (select_err < 0)
        {
            throw std::runtime_error("Selecting 1D hyperslab failed!");
        }

        return {dspace, memspace};
    }

    template <typename T>
    herr_t __write_container__(T&& data, hid_t memspace, hid_t filespace)
    {
        using value_type_1 = typename T::value_type;
        using base_type = remove_qualifier_t<value_type_1>;

        // we can write directly if we have a plain vector, no nested or stringtype.
        if constexpr (std::is_same_v<T, std::vector<value_type_1>> &&
                      !is_container_v<value_type_1> && !is_string_v<value_type_1>)
        {
            // check if attribute has been created, else do
            if (_dataset == -1)
            {
                _dataset = __create_dataset__<base_type>(0);
            }

            return H5Dwrite(_dataset, HDFTypeFactory::type<base_type>(0),
                            memspace, filespace, H5P_DEFAULT, data.data());
        }
        // when stringtype or containertype is stored in a container, then
        // we have to buffer. bufferfactory handles how to do this in detail
        else
        {
            std::size_t typesize = 0;
            if constexpr (is_container_v<base_type> and is_array_like_v<base_type>)
            {
                typesize = std::tuple_size<base_type>::value;
            }
            if (_dataset == -1)
            {
                _dataset = __create_dataset__<base_type>(typesize);
            }

            auto buffer = HDFBufferFactory::buffer(
                std::begin(data), std::end(data),
                [](auto& value) -> value_type_1& { return value; });

            return H5Dwrite(_dataset, HDFTypeFactory::type<base_type>(0),
                            memspace, filespace, H5P_DEFAULT, buffer.data());
        }
    }

    template <typename T>
    herr_t __write_container__(const T& data, hid_t memspace, hid_t filespace)
    {
        using value_type_1 = typename T::value_type;
        using base_type = remove_qualifier_t<value_type_1>;

        // we can write directly if we have a plain vector, no nested or stringtype.
        if constexpr (std::is_same_v<T, std::vector<value_type_1>> &&
                      !is_container_v<value_type_1> && !is_string_v<value_type_1>)
        {
            // check if attribute has been created, else do
            if (_dataset == -1)
            {
                _dataset = __create_dataset__<base_type>(0);
            }

            return H5Dwrite(_dataset, HDFTypeFactory::type<base_type>(0),
                            memspace, filespace, H5P_DEFAULT, data.data());
        }
        // when stringtype or containertype is stored in a container, then
        // we have to buffer. bufferfactory handles how to do this in detail
        else
        {
            std::size_t typesize = 0;
            if constexpr (is_container_v<base_type> and is_array_like_v<base_type>)
            {
                typesize = std::tuple_size<base_type>::value;
            }
            if (_dataset == -1)
            {
                _dataset = __create_dataset__<base_type>(typesize);
            }

            auto buffer = HDFBufferFactory::buffer(
                std::begin(data), std::end(data),
                [](auto& value) -> value_type_1& { return value; });

            return H5Dwrite(_dataset, HDFTypeFactory::type<base_type>(0),
                            memspace, filespace, H5P_DEFAULT, buffer.data());
        }
    }

    // Function for writing stringtypes, char*, const char*, std::string
    template <typename T>
    herr_t __write_stringtype__(T data, hid_t memspace, hid_t filespace)
    {
        // Since std::string cannot be written directly,
        // (only const char*/char* can), a buffer pointer has been added
        // to handle writing in a clearer way and with less code
        auto len = 0;
        const char* buffer = nullptr;

        if constexpr (std::is_pointer_v<T>) // const char* or char* -> strlen
                                            // needed
        {
            len = std::strlen(data);
            buffer = data;
        }
        else // simple for strings
        {
            len = data.size();
            buffer = data.c_str();
        }

        // check if attribute has been created, else do
        if (_dataset == -1)
        {
            _dataset = __create_dataset__<const char*>(len);
        }
        // use that strings store data in consecutive memory
        return H5Dwrite(_dataset, HDFTypeFactory::type<const char*>(len),
                        memspace, filespace, H5P_DEFAULT, buffer);
    }

    // Function for writing pointer types, shape of the array has to be given
    // where shape means the same as in python
    template <typename T>
    herr_t __write_pointertype__(T data, hid_t memspace, hid_t filespace)
    {
        // result types removes pointers, references, and qualifiers
        using basetype = remove_qualifier_t<T>;
        // std::cout << _name << ", type for pointer is  " << typeid(basetype).name()
        //   << ", int type is " << typeid(int).name() << std::endl;

        if (_dataset == -1)
        {
            _dataset = __create_dataset__<basetype>(0);
        }

        return H5Dwrite(_dataset, HDFTypeFactory::type<basetype>(), memspace,
                        filespace, H5P_DEFAULT, data);
    }

    // function for writing a scalartype.
    template <typename T>
    herr_t __write_scalartype__(T data, hid_t memspace, hid_t filespace)
    {
        // because we just write a scalar, the shape tells basically that
        // the attribute is pointlike: 1D and 1 entry.
        using basetype = remove_qualifier_t<T>;
        if (_dataset == -1)
        {
            _dataset = __create_dataset__<T>(0);
        }

        return H5Dwrite(_dataset, HDFTypeFactory::type<basetype>(), memspace,
                        filespace, H5P_DEFAULT, &data);
    }

protected:
    /**
     *  @brief Pointer to the parent object of the dataset
     */
    HDFObject* _parent_object;

    /**
     *  @brief path relative to the parent object
     */
    std::string _path;

    /**
     *  @brief dataset id
     */
    hid_t _dataset;

    /**
     *  @brief number of dimensions of the dataset
     */
    hsize_t _rank;

    /**
     *  @brief the currently occupied size of the dataset in number of elements
     */
    std::vector<hsize_t> _current_extend;

    /**
     * @brief  the maximum number of elements which can be stored in the dataset
     */
    std::vector<hsize_t> _capacity;

    /**
     * @brief the chunksizes per dimensions if dataset is extendible or compressed
     */
    std::vector<hsize_t> _chunksizes;

    std::vector<hsize_t> _offset;

    /**
     * @brief the level of compression, 0 to 10
     */
    std::size_t _compress_level;

    /**
     * @brief the info struct used to get the address of the dataset
     */
    H5O_info_t _info;

    /**
     * @brief the address of the dataset in the file, a unique value given by the hdf5 lib
     */
    haddr_t _address;

    /**
     * @brief Pointer to underlying file's referencecounter
     */
    std::shared_ptr<std::unordered_map<haddr_t, int>> _referencecounter;

public:
    /**
     * @brief get a shared_ptr to the parent_object
     *
     * @return std::shared_ptr<HDFObject>
     */
    HDFObject& get_parent()
    {
        return *_parent_object;
    }

    /**
     * @brief get the path of the dataset
     *
     * @return std::string
     */
    std::string get_path()
    {
        return _path;
    }

    /**
     * @brief get the rank of the dataset, i.e. the dimensionality
     *
     * @return std::size_t
     */
    std::size_t get_rank()
    {
        return _rank;
    }

    /**
     * @brief get the current extend of the dataset
     *
     * @return std::vector<hsize_t>
     */
    auto get_current_extend()
    {
        return _current_extend;
    }

    auto get_offset()
    {
        return _offset;
    }
    /**
     * @brief get the maximum extend of the dataset
     *
     * @return std::vector<hsize_t>
     */
    auto get_capacity()
    {
        return _capacity;
    }

    /**
     * @brief Get the chunksizes vector
     *
     * @return auto
     */
    auto get_chunksizes()
    {
        return _chunksizes;
    }

    /**
     * @brief Get the compress level object
     *
     * @return auto
     */
    auto get_compresslevel()
    {
        return _compress_level;
    }

    /**
     * @brief get the current extend of the dataset
     *
     * @return std::vector<hsize_t>
     */
    hid_t get_id()
    {
        return _dataset;
    }

    /**
     * @brief get the reference counter map
     *
     * @return std::map<haddr_t, int>
     */
    auto get_referencecounter()
    {
        return _referencecounter;
    }

    /**
     * @brief get the address of the dataset in the underlying hdffile
     *
     * @return haddr_t
     */
    haddr_t get_address()
    {
        return _address;
    }

    /**
     * @brief      add attribute to the dataset
     *
     * @param      attribute_path  The attribute path
     * @param      data  The attribute data
     *
     * @tparam     Attrdata        The type of the attribute data
     */
    template <typename Attrdata>
    void add_attribute(std::string attribute_path, Attrdata data)
    {
        // make attribute and write
        HDFAttribute attr(*this, attribute_path);
        attr.write(data);
    }

    /**
     * @brief      close the dataset
     */
    void close()
    {
        if (H5Iis_valid(_dataset))
        {
            if ((*_referencecounter)[_address] == 1)
            {
                H5Dclose(_dataset);
                _referencecounter->erase(_referencecounter->find(_address));
                _dataset = -1;
            }
            else
            {
                --(*_referencecounter)[_address];
                _dataset = -1;
            }
        }
    }

    /**
     * @brief Open the dataset in parent_object with relative path 'path'.
     *
     * @param parent_object The HDFGroup/HDFFile into which the dataset shall be created
     * @param adaptor The function which makes the data to be written from given iterators
     * @param path The path of the dataset in the parent_object
     * @param rank The number of dimensions of the dataset
     * @param capacity The maximum size of the dataset in each dimension. Give
     *                 H5S_UNLIMITED if unlimited size is desired. Then you have
     *                 to give chunksizes.
     * @param chunksize The chunksizes in each dimension to use
     * @param compress_level The compression level to use, 0 to 10 (0 = no compression, 10 highest compression)
     */
    void open(HDFObject& parent_object,
              std::string path,
              std::vector<hsize_t> capacity = {},
              std::vector<hsize_t> chunksizes = {},
              hsize_t compress_level = 0)
    {
        _parent_object = &parent_object;
        _path = path;
        _referencecounter = parent_object.get_referencecounter();

        _current_extend = {};

        // Try to find the dataset in the parent_object
        // If it is there, open it.
        // Else: postphone the dataset creation to the first write
        if (H5LTfind_dataset(_parent_object->get_id(), _path.c_str()) == 1)
        { // dataset exists
            // open it
            _dataset = H5Dopen(_parent_object->get_id(), _path.c_str(), H5P_DEFAULT);

            // get dataspace and read out rank, extend, capacity
            hid_t dataspace = H5Dget_space(_dataset);

            _rank = H5Sget_simple_extent_ndims(dataspace);
            _current_extend.resize(_rank);
            _capacity.resize(_rank);
            _offset = _current_extend;

            // if the end of a row has been reached, then set the offset
            // to the beginning of the next:
            // all arrays [lines, columns]
            /*
                current extend describes:
                ********************** filled
                **********************
                **********************
                **********************
                ----------------------
                ----------------------  still available


                then offset shall describe the x below:
                **********************
                **********************
                **********************
                **********************
                x---------------------
                ----------------------

                from which we can go on writing
            */
            if (_rank > 1)
            {
                if (_current_extend[0] == _capacity[0])
                {
                    _offset[0] += 1;
                    _offset[1] = 0;
                }
            }

            H5Sget_simple_extent_dims(dataspace, _current_extend.data(),
                                      _capacity.data());
            H5Sclose(dataspace);

            // Update info and reference counter
            H5Oget_info(_dataset, &_info);
            _address = _info.addr;
            (*_referencecounter)[_address] += 1;
        }
        else
        {
            if (capacity.size() == 0)
            {
                if (chunksizes.size() == 0)
                {
                    throw std::runtime_error(
                        "In trying to create dataset " + path +
                        ": chunksizes have to be given when "
                        "not giving an explicit capacity!");
                }
                else
                {
                    _rank = chunksizes.size();
                    _chunksizes = chunksizes;
                    _capacity = std::vector<hsize_t>(_rank, H5S_UNLIMITED);
                    _offset = std::vector<hsize_t>(_rank, 0);
                }
            }
            else
            {
                _capacity = capacity;
                _chunksizes = chunksizes;
                _rank = _capacity.size();
                _offset = std::vector<hsize_t>(_rank, 0);
            }

            _compress_level = compress_level;

            _dataset = -1;
        }
    }

    /**
     * @brief      swap the state of the objects
     *
     * @param      other  The other
     */
    void swap(HDFDataset& other)
    {
        using std::swap;
        swap(_parent_object, other._parent_object);
        swap(_path, other._path);
        swap(_dataset, other._dataset);
        swap(_rank, other._rank);
        swap(_current_extend, other._current_extend);
        swap(_capacity, other._capacity);
        swap(_chunksizes, other._chunksizes);
        swap(_offset, other._offset);
        swap(_compress_level, other._compress_level);
        swap(_info, other._info);
        swap(_address, other._address);
        swap(_referencecounter, other._referencecounter);
    }

    template <typename T>
    void write(const T& data, [[maybe_unused]] std::vector<hsize_t> shape)
    {
    }

    template <typename T>
    void write(T&& data, [[maybe_unused]] std::vector<hsize_t> shape = {})
    {
        if (_dataset == -1)
        {
            if (_rank > 2)
            {
                throw std::runtime_error("Rank > 2 not supported");
            }

            _current_extend.resize(_rank);

            if constexpr (is_container_v<T>)
            {
                if (_rank == 1)
                {
                    _current_extend[_rank - 1] = data.size();
                }
                else
                {
                    _current_extend[0] = 1;
                    _current_extend[1] = data.size();
                }
            }

            else if constexpr (std::is_pointer_v<T> and !is_string_v<T>)
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
                    _current_extend[i] = shape[i];
                }
            }
            else
            {
                _current_extend[_rank - 1] = 1;
            }

            if constexpr (is_container_v<T>)
            {
                herr_t err = __write_container__(std::forward<T&&>(data), H5S_ALL, H5S_ALL);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in writing container");
                }
            }
            else if constexpr (is_string_v<T>)
            {
                herr_t err = __write_stringtype__(std::forward<T&&>(data), H5S_ALL, H5S_ALL);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in writing string");
                }
            }
            else if constexpr (std::is_pointer_v<T> and !is_string_v<T>)
            {
                herr_t err = __write_pointertype__(std::forward<T&&>(data), H5S_ALL, H5S_ALL);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in writing pointer");
                }
            }
            else
            {
                herr_t err = __write_scalartype__(std::forward<T&&>(data), H5S_ALL, H5S_ALL);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in writing scalar");
                }
            }
        }
        else
        {
            if (_capacity == _current_extend)
            {
                throw std::runtime_error("Dataset " + _path +
                                         ": Error, dataset cannot be extended "
                                         "because it reached its capacity");
            }
            else
            {
                // set offset array
                _offset = _current_extend;
                if (_rank > 1)
                {
                    if (_current_extend[0] == _capacity[0])
                    {
                        _offset[0] += 1;
                        _offset[1] = 0;
                    }
                }

                // if data is a container, then we have to add its size to
                // extend, if it is a pointer, we have to add the pointers
                // shape, else we have to add 1 because we either write
                // a single scalar or string
                if constexpr (is_container_v<T>)
                {
                    if (_rank == 1)
                    {
                        _current_extend[0] += data.size();
                    }
                    else
                    {
                        _current_extend[0] += 1;
                    }
                }
                if constexpr (std::is_pointer_v<T> and !is_string_v<T>)
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
                        _current_extend[i] = shape[i];
                    }
                }
                else
                {
                    if (_rank == 1)
                    {
                        // if rank is one we can only extend into one direction
                        _current_extend[0] += 1;
                    }
                    else
                    {
                        // first fill row, then column wise increase
                        if (_current_extend[0] < _capacity[0])
                        {
                            _current_extend[0] += 1;
                        }
                        // if row is full, start a new one
                        else
                        {
                            _current_extend[1] += 1;
                        }
                    }
                }
            }

            // select counts for dataset
            // this has to be generalized and refactored
            std::vector<hsize_t> counts(_rank, 0);
            // rank is 1
            if (_rank == 1)
            {
                if constexpr (is_container_v<T>)
                {
                    counts[0] = data.size();
                }
                else
                {
                    counts[0] = 1;
                }
            }
            // rank is 2
            else
            {
                if constexpr (is_container_v<T>)
                {
                    counts[0] = 1;
                    counts[1] = data.size();
                }
                else
                {
                    counts = std::vector<hsize_t>(_rank, 1);
                }
            }

            auto [memspace, filespace] =
                __select_dataset_subset__(_offset, std::vector<hsize_t>(_rank, 1),
                                          std::vector<hsize_t>(_rank, 1), counts);

            if constexpr (is_container_v<T>)
            {
                herr_t err = __write_container__(std::forward<T&&>(data), memspace, filespace);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in appending container");
                }
            }
            else if constexpr (is_string_v<T>)
            {
                herr_t err = __write_stringtype__(std::forward<T&&>(data), memspace, filespace);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in appending string");
                }
            }
            else if constexpr (std::is_pointer_v<T> and !is_string_v<T>)
            {
                herr_t err = __write_pointertype__(std::forward<T&&>(data),
                                                   memspace, filespace);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in appending pointer");
                }
            }
            else
            {
                herr_t err = __write_scalartype__(std::forward<T&&>(data), memspace, filespace);
                if (err < 0)
                {
                    throw std::runtime_error("Dataset " + _path +
                                             ": Error in appending scalar");
                }
            }
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
    HDFDataset(const HDFDataset& other)
        : _parent_object(other._parent_object),
          _path(other._path),
          _dataset(other._dataset),
          _rank(other._rank),
          _current_extend(other._current_extend),
          _capacity(other._capacity),
          _chunksizes(other._chunksizes),
          _offset(other._offset),
          _compress_level(other._compress_level),
          _info(other._info),
          _address(other._address),
          _referencecounter(other._referencecounter)
    {
        (*_referencecounter)[_address] += 1;
    }

    /**
     * @brief      Move constructor
     *
     * @param      other  The other
     */
    HDFDataset(HDFDataset&& other) : HDFDataset()
    {
        this->swap(other);
    }

    /**
     * @brief      Assignment operator
     *
     * @param      other  The other
     *
     * @return     HDFDataset&
     */
    HDFDataset& operator=(HDFDataset other)
    {
        this->swap(other);
        return *this;
    }

    /**
     * @brief Construct a new HDFDataset object
     *
     * @param parent_object The HDFGroup/HDFFile into which the dataset shall be created
     * @param adaptor The function which makes the data to be written from given iterators
     * @param path The path of the dataset in the parent_object
     * @param rank The number of dimensions of the dataset
     * @param capacity The maximum size of the dataset in each dimension. Give
     *                 H5S_UNLIMITED if unlimited size is desired. Then you have
     *                 to give chunksizes.
     * @param chunksize The chunksizes in each dimension to use
     * @param compress_level The compression level to use
     */
    HDFDataset(HDFObject& parent_object,
               std::string path,
               std::vector<hsize_t> capacity = {},
               std::vector<hsize_t> chunksizes = {},
               hsize_t compress_level = 0)
        : _referencecounter(parent_object.get_referencecounter())

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
};

/**
 * @brief      Exchange state between lhs and rhs
 *
 * @param      lhs        The left hand side
 * @param      rhs        The right hand side
 *
 * @tparam     HDFObject  The type of the parent object
 */
template <typename HDFObject>
void swap(HDFDataset<HDFObject>& lhs, HDFDataset<HDFObject>& rhs)
{
    lhs.swap(rhs);
}

} // namespace DataIO
} // namespace Utopia
#endif
