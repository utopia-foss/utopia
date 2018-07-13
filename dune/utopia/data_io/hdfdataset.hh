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
    hid_t __create_dataset_helper__(std::size_t typesize)
    {
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
            return H5Dcreate(_parent_object->get_id(), _path.c_str(),
                             HDFTypeFactory::type<Datatype>(typesize), dspace,
                             group_plist, plist, H5P_DEFAULT);
        }
        else
        {
            // make dataspace
            hid_t dspace =
                H5Screate_simple(_rank, _current_extend.data(), _capacity.data());

            // can create the dataset right away
            return H5Dcreate(_parent_object->get_id(), _path.c_str(),
                             HDFTypeFactory::type<Datatype>(typesize), dspace,
                             group_plist, H5P_DEFAULT, H5P_DEFAULT);
        }
    }

    /**
     * @brief      wrapper for creating a dataset given all parameters needed
     *
     * @tparam     result_type     The type of the data stored in this dataset
     */
    template <typename result_type>
    void __create_dataset__()
    {
        std::size_t typesize = 0;
        if constexpr (is_container_v<result_type> and is_array_like_v<result_type>)
        {
            typesize = std::tuple_size<result_type>::value;
        }

        // make a new non-compressed dataset
        _dataset = __create_dataset_helper__<result_type>(typesize);

        // update info and add the new dataset to the reference counter
        H5Oget_info(_dataset, &_info);
        _address = _info.addr;
        (*_referencecounter)[_address] = 1;
    }

    /**
     * @brief      For writing a dataset
     * @details    This assumes that the dataset has already been created
     *
     * @param      begin      The begin of the iterator range
     * @param      end        The end of the iterator range
     * @param      adaptor    The adaptor to extract the data
     * @param      dspace     The data space
     * @param      memspace   The the memory space
     *
     * @tparam     Iter       The type of the iterator
     * @tparam     Adaptor    The type of the adaptor
     */
    template <typename Iter, typename Adaptor>
    void __write_dataset__(Iter begin, Iter end, Adaptor&& adaptor, hid_t dspace, hid_t memspace)
    {
        using result_type = remove_qualifier_t<decltype(adaptor(*begin))>;
        // now that the dataset has been made let us write to it
        // buffering at first
        if constexpr (is_container_v<result_type>)
        {
            // make an intermediate buffer which then can be turned into hvl_t.
            std::vector<result_type> temp(std::distance(begin, end));
            std::generate(temp.begin(), temp.end(),
                          [&begin, &adaptor]() { return adaptor(*(begin++)); });

            // then turn this temporary buffer into a hvl_t thing, then write.
            auto buffer = HDFBufferFactory::buffer(
                temp.begin(), temp.end(),
                [](auto& val) -> result_type& { return val; });

            if constexpr (is_array_like_v<result_type>)
            {
                constexpr std::size_t s = std::tuple_size<result_type>::value;
                // write to buffer
                herr_t write_err =
                    H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(s),
                             memspace, dspace, H5P_DEFAULT, buffer.data());
                if (write_err < 0)
                {
                    throw std::runtime_error(
                        "Writing to 1D fixed length array dataset "
                        "failed in "
                        "dataset" +
                        _path);
                }
            }
            else
            {
                // write to buffer
                herr_t write_err =
                    H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(),
                             memspace, dspace, H5P_DEFAULT, buffer.data());
                if (write_err < 0)
                {
                    throw std::runtime_error(
                        "Writing to 1D variable len dataset failed in "
                        "dataset" +
                        _path);
                }
            }
        }
        else
        {
            auto buffer = HDFBufferFactory::buffer(begin, end, adaptor);

            // write to buffer
            herr_t write_err =
                H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(),
                         memspace, dspace, H5P_DEFAULT, buffer.data());

            if (write_err < 0)
            {
                throw std::runtime_error("Writing to 1D dataset failed!");
                // FIXME Is this really only 1D?
                // FIXME Could we add some info here, e.g. the path of the dataset?
            }
        }
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
    std::vector<hid_t> __select_dataset_subset__(std::vector<hsize_t>& offset,
                                                 std::vector<hsize_t>& stride,
                                                 std::vector<hsize_t>& block,
                                                 std::vector<hsize_t>& count)
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

    /**
     * @brief      overload for reading std::string from dataset
     *
     * @param      buffer_size   The buffer size
     * @param      dspace        The dspace
     * @param      memspace      The memspace
     *
     * @tparam     desired_type  The desired return type
     *
     * @return     The data read from the dataset
     */
    template <typename desired_type>
    auto __read_from_dataset__(hsize_t buffer_size, hid_t dspace, hid_t memspace)
    {
        if constexpr (is_container_v<desired_type>)
        {
            std::vector<desired_type> data(buffer_size);

            if constexpr (is_array_like_v<desired_type>)
            {
                hid_t type = H5Dget_type(_dataset);
                herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                          H5P_DEFAULT, data.data());
                H5Tclose(type);
                if (read_err < 0)
                {
                    throw std::runtime_error("Error reading 1d dataset");
                }
                return data;
            }
            else
            {
                std::vector<hvl_t> buffer(buffer_size);

                hid_t type = H5Dget_type(_dataset);
                herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                          H5P_DEFAULT, buffer.data());
                H5Tclose(type);
                if (read_err < 0)
                {
                    throw std::runtime_error("Error reading 1d dataset");
                }
                for (std::size_t i = 0; i < buffer_size; ++i)
                {
                    data[i].resize(buffer[i].len);
                    auto ptr = static_cast<typename desired_type::value_type*>(
                        buffer[i].p);

                    for (std::size_t j = 0; j < buffer[i].len; ++j)
                    {
                        data[i][j] = ptr[j];
                    }
                }

                return data;
            }
        }
        else
        {
            if constexpr (std::is_same_v<std::string, desired_type>)
            {
                // create the buffer
                std::vector<const char*> buffer(buffer_size);
                hid_t type = H5Dget_type(_dataset);

                // read the data
                herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                          H5P_DEFAULT, buffer.data());
                H5Tclose(type);

                if (read_err < 0)
                {
                    throw std::runtime_error("Error reading 1d dataset");
                }

                // fill the buffer
                std::vector<std::string> data(buffer_size);
                for (std::size_t i = 0; i < buffer_size; ++i)
                {
                    data[i] = std::string(buffer[i]);
                }

                return data;
            }
            else
            {
                std::vector<desired_type> buffer(buffer_size);
                hid_t type = H5Dget_type(_dataset);
                herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                          H5P_DEFAULT, buffer.data());
                H5Tclose(type);

                if (read_err < 0)
                {
                    throw std::runtime_error("Error reading 1d dataset");
                }
                return buffer;
            }
        }
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
     * @param      attribute_data  The attribute data
     *
     * @tparam     Attrdata        The type of the attribute data
     */
    template <typename Attrdata>
    void add_attribute(std::string attribute_path, Attrdata attribute_data)
    {
        // make attribute and write
        HDFAttribute attr(*this, attribute_path);
        attr.write(attribute_data);
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
                }
            }
            else
            {
                _capacity = capacity;
                _chunksizes = chunksizes;
                _rank = _capacity.size();
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
        swap(_compress_level, other._compress_level);
        swap(_info, other._info);
        swap(_address, other._address);
        swap(_referencecounter, other._referencecounter);
    }

    /**
     * @brief      Write data, extracting fields from complex data like
     * structs via an adaptor function. Adds an attribute which contains
     *             rank and size of the data. If you wish to write
     * vector containers, make sure that you do return a reference from
     *             adaptor!
     *
     * @param      begin           The begin
     * @param      end             The end
     * @param      adaptor         The adaptor
     * @param      rank            The rank. Defaults to 1
     * @param      extend          The extend of the data to write
     * @param      capacity        The maximum size of the dataset
     * @param      chunksize       The chunksize; extension is only
     * possible if a nonzero value is given here
     * @param      compress_level  The compression level; only possible
     * with nonzero chunksize
     *
     * @tparam     Iter            The type of the iterator
     * @tparam     Adaptor         The type of the adaptor
     */
    template <typename Iter, typename Adaptor>
    void write(Iter begin, Iter end, Adaptor&& adaptor)
    {
        using result_type = remove_qualifier_t<decltype(adaptor(*begin))>;

        // get size of stuff to write
        hsize_t size = std::distance(begin, end);

        // currently not supported rank -> throw error
        if (_rank > 2)
        {
            throw std::runtime_error(
                "Cannot write dataset: Ranks higher than "
                "2 are not supported currently!");
        }

        if (_dataset == -1)
        { // dataset does not exist yet

            // set current extend
            _current_extend = std::vector<hsize_t>(_rank, 1);
            _current_extend[_rank - 1] = size;

            // set capacity
            if (_capacity.size() == 0)
            {
                _capacity = _current_extend;
            }
            else
            {
                if (!(_capacity == _current_extend) and _chunksizes.size() == 0)
                {
                    throw std::runtime_error(
                        "The provision of chunksizes is mandatory if dataset " +
                        _path + " shall be extendible.");
                }
            }

            // build a new dataset and assign to _dataset member
            __create_dataset__<result_type>();

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false)
            {
                throw std::runtime_error(
                    "Trying to create dataset at path "
                    "'" +
                    _path +
                    "' resulted in invalid "
                    "ID! Check your arguments.");
            }

            // gather data for dimensionality info: rank, extend, capacity
            // TODO see #119 for changes needed here
            // https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues/119
            std::vector<hsize_t> dims(1 + _current_extend.size() + _capacity.size());
            std::size_t i = 0;
            dims[i] = _rank;
            ++i;

            for (std::size_t j = 0; j < _current_extend.size(); ++j, ++i)
            {
                dims[i] = _current_extend[j];
            }

            for (std::size_t j = 0; j < _capacity.size(); ++j, ++i)
            {
                dims[i] = _capacity[i];
            }

            // store the dimensionality info attribute
            add_attribute<std::vector<hsize_t>>("dimensionality info", dims);

            // write dataset
            __write_dataset__(begin, end, std::forward<Adaptor&&>(adaptor), H5S_ALL, H5S_ALL);
        }
        else
        { // dataset does exist

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false)
            {
                throw std::runtime_error("Existing dataset '" + _path +
                                         "' "
                                         "has an invalid ID. Has the dataset "
                                         "already been closed?");
            }

            // check if dataset can be extended, i.e. if extend < capacity.
            for (std::size_t i = 0; i < _rank; ++i)
            {
                if ((_current_extend[i] == _capacity[i]) && (_current_extend[i] != 0))
                {
                    throw std::runtime_error(
                        "Dataset " + _path +
                        " cannot be extended! Its "
                        "extend reached capacity. Make larger, or set the size "
                        "to unlimited and give a chunksize to avoid this");
                }
            }

            // distinguish between ranks: treat 1d differently from nd
            if (_rank == 1)
            {
                /* If the dataset can be extended, then do so now.
                Extend the first dimension in the dataspace, such that:

                dataset before:

                dim0
                 |      +
                 |      +
                 |      +
                 ν

                dataset after extend by size 2

                dim0
                 |      +
                 |      +
                 |      +
                 |      +
                 |      +
                 ν
                */

                // get the offset, stride count and block vectors for
                // selection of the hyperslab
                std::vector<hsize_t> offset = _current_extend;
                std::vector<hsize_t> stride{1};
                std::vector<hsize_t> count = offset;
                std::vector<hsize_t> block{1};

                // make dataset larger
                _current_extend[0] += size;

                herr_t ext_err = H5Dset_extent(_dataset, _current_extend.data());
                if (ext_err < 0)
                {
                    throw std::runtime_error("1D dataset " + _path +
                                             "could not be "
                                             "extended!");
                }

                // gather information for dimensionality info attribute
                std::vector<hsize_t> dims(1 + _current_extend.size() + _capacity.size());
                std::size_t i = 0;
                dims[i] = _rank;
                ++i;

                for (std::size_t j = 0; j < _current_extend.size(); ++j, ++i)
                {
                    dims[i] = _current_extend[j];
                }

                for (std::size_t j = 0; j < _capacity.size(); ++j, ++i)
                {
                    dims[i] = _capacity[i];
                }

                // add the attribute
                add_attribute<std::vector<hsize_t>>("dimensionality info", dims);

                // make subset selection and extract data and memory space
                auto spaces = __select_dataset_subset__(offset, stride, block, count);
                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                // and now: write the data
                __write_dataset__(begin, end, std::forward<Adaptor&&>(adaptor),
                                  dspace, memspace);

                // // buffering
                // auto buffer = HDFBufferFactory::buffer<result_type>(
                //     begin, end, std::forward<Adaptor &&>(adaptor));

                // // write to buffer
                // herr_t write_err =
                //     H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(),
                //              memspace, dspace, H5P_DEFAULT, buffer.data());

                H5Sclose(dspace);
                H5Sclose(memspace);
            }
            else
            { // N- dimensional dataset

                /* If the dataset can be extended, then do so now.
                Extend the first dimension in the dataspace, such that:

                dataset before:

                     dim1 ->
                dim0 +++++++++++++++++++++++++++++++++++++++++++++++
                 |   +++++++++++++++++++++++++++++++++++++++++++++++
                 ν

                dataset after extend

                       dim1 ->
                 dim0  +++++++++++++++++++++++++++++++++++++++++++++++
                 |     +++++++++++++++++++++++++++++++++++++++++++++++
                 |     +++++++++++++++++++++++++++++++++++++++++++++++
                 ν
                */

                // get some stuff we need for the hyperslab selection later
                std::vector<hsize_t> offset = _current_extend;
                std::vector<hsize_t> stride(_current_extend.size(), 1);
                std::vector<hsize_t> count = offset;
                std::vector<hsize_t> block(_current_extend.size(), 1);
                count[0] = 1; // such that we add one more 'slice'

                // extend
                _current_extend[0] += 1; // add one more slice
                for (std::size_t i = 1; i < offset.size(); ++i)
                {
                    offset[i] = 0;
                }

                herr_t ext_err = H5Dset_extent(_dataset, _current_extend.data());
                if (ext_err < 0)
                {
                    throw std::runtime_error("ND dataset " + _path +
                                             " could not be "
                                             "extended!");
                }

                // gather information for dimensionality info attribute
                std::vector<hsize_t> dims(1 + _current_extend.size() + _capacity.size());
                std::size_t i = 0;
                dims[i] = _rank;
                ++i;

                for (std::size_t j = 0; j < _current_extend.size(); ++j, ++i)
                {
                    dims[i] = _current_extend[j];
                }

                for (std::size_t j = 0; j < _capacity.size(); ++j, ++i)
                {
                    dims[i] = _capacity[i];
                }

                // store the attribute
                add_attribute<std::vector<hsize_t>>("dimensionality info", dims);

                // make subset selection and extract data and memory space
                auto spaces = __select_dataset_subset__(offset, stride, block, count);
                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                // and write the dataset
                __write_dataset__(begin, end, std::forward<Adaptor&&>(adaptor),
                                  dspace, memspace);

                H5Sclose(memspace);
                H5Sclose(dspace);
            }
        }
    }

    /**
     * @brief      Read (a subset_of) a dataset. Flattens nd datasets
     *
     * @param      start         The start. If not given, the entire
     * dataset will be read.
     * @param      end           The end
     * @param      stride        The stride
     *
     * @tparam     desired_type  The desired return type
     *
     * @return     the read data
     */
    template <typename desired_type>
    auto read(std::vector<hsize_t> start = {},
              std::vector<hsize_t> end = {},
              std::vector<hsize_t> stride = {})
    {
        // check if dataset id is ok
        if (H5Iis_valid(_dataset) == false)
        {
            throw std::runtime_error("Existing dataset '" + _path +
                                     "' has an "
                                     "invalid ID. Has the dataset already "
                                     "been closed?");
        }
        // check if rank is zero ->bad
        if (_rank == 0)
        {
            throw std::runtime_error("Rank of dataset " + _path + " is zero!");
        }
        // 1d dataset
        if (_rank == 1)
        {
            // assume that if start is empty, the entire dataset should be read
            if (start.size() == 0)
            {
                return __read_from_dataset__<desired_type>(_current_extend[0],
                                                           H5S_ALL, H5S_ALL);
            }
            else
            { // read a subset determined by start end stride
                // check that the arrays have the correct size:
                if (start.size() != _rank || end.size() != _rank || stride.size() != _rank)
                {
                    throw std::invalid_argument("Cannot read dataset " + _path +
                                                ": start, "
                                                "end and/or stride size did "
                                                "not match the rank!");
                }
                // determine the count to be read
                std::vector<hsize_t> count(start.size());
                for (std::size_t i = 0; i < start.size(); ++i)
                {
                    count[i] = (end[i] - start[i]) / stride[i];
                }
                std::vector<hsize_t> block(count.size(), 1);

                // select dataset subset to read
                auto spaces = __select_dataset_subset__(start, stride, block, count);
                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                // read
                auto buffer = __read_from_dataset__<desired_type>(count[0], dspace, memspace);

                // close memoryspace resources and return
                H5Sclose(dspace);
                H5Sclose(memspace);

                return buffer;
            }
        }
        else
        { // N-dimensional dataset
            // assume that if start is empty, the entire dataset should be read
            if (start.size() == 0)
            {
                // hid_t type = H5Dget_type(_dataset);

                // make a buffer and read
                // this is a 1d data which contains the multi -
                // dimensional stuff in a flattened version
                std::size_t buffersize = 1;
                for (std::size_t i = 0; i < _rank; ++i)
                {
                    buffersize *= _current_extend[i];
                }

                return __read_from_dataset__<desired_type>(buffersize, H5S_ALL, H5S_ALL);
            }
            else
            { // read a subset determined by start end stride
                // check that the arrays have the correct size:
                if (start.size() != _rank || end.size() != _rank || stride.size() != _rank)
                {
                    throw std::invalid_argument("Cannot read dataset " + _path +
                                                ": start, "
                                                "end and/or stride size did "
                                                "not match the rank!");
                }

                // determine the count to be read
                std::vector<hsize_t> count(start.size());
                for (std::size_t i = 0; i < start.size(); ++i)
                {
                    count[i] = (end[i] - start[i]) / stride[i];
                }
                std::vector<hsize_t> block(count.size(), 1);

                // select dataset subset to read
                auto spaces = __select_dataset_subset__(start, stride, block, count);
                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                // determine buffersize
                std::size_t buffersize = 1;
                for (std::size_t i = 0; i < _rank; ++i)
                {
                    buffersize *= count[i];
                }

                // read
                auto buffer =
                    __read_from_dataset__<desired_type>(buffersize, dspace, memspace);

                // close memoryspace resources
                H5Sclose(dspace);
                H5Sclose(memspace);

                return buffer;
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
        if (H5Iis_valid(_dataset))
        {
            if ((*_referencecounter)[_address] == 1)
            {
                H5Dclose(_dataset);
                _referencecounter->erase(_referencecounter->find(_address));
            }
            else
            {
                --(*_referencecounter)[_address];
            }
        }
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
