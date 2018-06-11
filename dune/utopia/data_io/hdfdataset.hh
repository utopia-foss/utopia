#ifndef HDFDATASET_HH
#define HDFDATASET_HH

#include "hdfattribute.hh"
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"
#include "hdfutilities.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <numeric>
#include <cmath>
#include <unordered_map>

namespace Utopia
{
namespace DataIO
{

/**
 * @brief      Class representing a HDFDataset, wich reads and writes data and
 *             attributes
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
     * @param      chunksize       The chunksize
     * @param      compress_level  The compress level; only possible with a
     *                             non-zero chunksize
     *
     * @tparam     Datatype        The data type stored in this dataset
     *
     * @return     The created dataset
     */
    template <typename Datatype>
    hid_t __create_dataset_helper__(hsize_t chunksize, hsize_t compress_level)
    {
        // create group property list and (potentially) intermediate groups
        hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(group_plist, 1);

        // distinguish by chunksize; chunked dataset needed for compression
        if (chunksize > 0)
        {
            // create creation property list, set chunksize and compress level
            hid_t plist = H5Pcreate(H5P_DATASET_CREATE);

            std::vector<hsize_t> chunksizes(_rank, chunksize);
            H5Pset_chunk(plist, _rank, chunksizes.data());

            if (compress_level > 0)
            {
                H5Pset_deflate(plist, compress_level);
            }

            // make dataspace
            hid_t dspace = H5Screate_simple(_rank, _extend.data(),
                                            _max_extend.data());

            // create dataset and return
            return H5Dcreate(_parent_object->get_id(), _name.c_str(),
                             HDFTypeFactory::type<Datatype>(), dspace,
                             group_plist, plist, H5P_DEFAULT);
        }
        else
        {
            // can create the dataset right away
            return H5Dcreate(_parent_object->get_id(), _name.c_str(),
                             HDFTypeFactory::type<Datatype>(),
                             H5Screate_simple(_rank, _extend.data(),
                                              _max_extend.data()),
                             group_plist, H5P_DEFAULT, H5P_DEFAULT);
        }
    }

    /**
     * @brief      wrapper for creating a dataset given all parameters needed
     *
     * @param      size            The (flattened) size of the dataset
     * @param      rank            The rank (number of dimensions)
     * @param      extend          The extend of the data to write; if zero,
     *                             this will lead to the dataset size being
     *                             extended. Note that extension is only
     *                             possible with chunked data.
     * @param      max_size        The maximum size of the dataset
     * @param      chunksize       The chunksize
     * @param      compress_level  The compress level; only available with
     *                             non-zero chunksize
     *
     * @tparam     result_type     The type of the data stored in this dataset
     */
    template <typename result_type>
    void __create_dataset__(hsize_t size,
                            hsize_t rank,
                            std::vector<hsize_t> extend,
                            std::vector<hsize_t> max_size,
                            hsize_t chunksize,
                            hsize_t compress_level)
    {
        _rank = rank;

        // Distinguish by chunksize
        if (chunksize > 0)
        {
            if (extend.size() == 0)
            {
                // not given; create vector from size and rank
                _extend = std::vector<hsize_t>(_rank, size);
            }
            else
            {
                _extend = extend;
            }

            if (max_size.size() == 0)
            {
                _max_extend = std::vector<decltype(H5S_UNLIMITED)>(_rank, H5S_UNLIMITED);
            }
            else
            {
                _max_extend = max_size;
            }

            if (compress_level > 0)
            {
                // make a new compressed dataset
                _dataset = __create_dataset_helper__<result_type>(chunksize, compress_level);
            }
            else
            {
                // make a new non-compressed dataset
                _dataset = __create_dataset_helper__<result_type>(chunksize, compress_level);
            }
        }
        else
        { // chunk size 0 implies non-extendable and non-compressed dataset

            if (extend.size() == 0)
            {
                // not given; create vector from size and rank
                _extend = std::vector<hsize_t>(_rank, size);
            }
            else
            {
                _extend = extend;
            }

            if (max_size.size() == 0)
            {
                _max_extend = _extend;
            }
            else
            {
                _max_extend = max_size;
            }

            // make dataset if necessary
            _dataset = __create_dataset_helper__<result_type>(chunksize, compress_level);
        }

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
    void __write_dataset__(Iter begin,
                           Iter end,
                           Adaptor&& adaptor,
                           hid_t dspace,
                           hid_t memspace)
    {
        using result_type =
            typename HDFTypeFactory::result_type<decltype(adaptor(*begin))>::type;

        // First, create the buffer
        auto buffer = HDFBufferFactory::buffer(begin, end,
                                               std::forward<Adaptor&&>(adaptor)
                                               );

        // Now, write to the buffer
        herr_t write_err = H5Dwrite(_dataset,
                                    HDFTypeFactory::type<result_type>(),
                                    memspace, dspace,
                                    H5P_DEFAULT, buffer.data());

        if (write_err < 0)
        {
            throw std::runtime_error("Writing to 1D dataset failed!");
            // FIXME Is this really only 1D?
            // FIXME Could we add some info here, e.g. the name of the dataset?
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

        herr_t select_err = H5Sselect_hyperslab(dspace,
                                                H5S_SELECT_SET,
                                                offset.data(),
                                                stride.data(),
                                                count.data(),
                                                block.data());
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
    auto __read_from_dataset__(hsize_t buffer_size,
                               hid_t dspace,
                               hid_t memspace)
    {
        if constexpr (is_container<desired_type>::value)
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
            { // container was not a std::string
                // create the buffer
                std::vector<hvl_t> buffer(buffer_size);
                std::vector<desired_type> data(buffer_size);

                // Read the data
                hid_t type = H5Dget_type(_dataset);
                herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                          H5P_DEFAULT, buffer.data());
                H5Tclose(type);

                if (read_err < 0)
                {
                    throw std::runtime_error("Error reading 1d dataset");
                }

                // fill the buffer, static-casting to the desired type
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
        { // desired type is not a container
            // create a buffer
            std::vector<desired_type> buffer(buffer_size);
            hid_t type = H5Dget_type(_dataset);

            // read the data directly into the buffer
            herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                      H5P_DEFAULT, buffer.data());
            H5Tclose(type);

            if (read_err < 0)
            {
                throw std::runtime_error("Error reading 1d dataset");
            }

            // can directly return the buffer here
            return buffer;
        }
    }

protected:
    std::shared_ptr<HDFObject> _parent_object;
    std::string _name;
    hid_t _dataset;
    hsize_t _rank;
    std::vector<hsize_t> _extend;
    std::vector<hsize_t> _max_extend;
    H5O_info_t _info;
    haddr_t _address;
    std::shared_ptr<std::unordered_map<haddr_t, int>> _referencecounter;

public:
    /**
     * @brief      get a weak ptr to the parent_object
     *
     * @return     std::weak_ptr<HDFObject>
     */
    std::shared_ptr<HDFObject> get_parent()
    {
        return _parent_object;
    }

    /**
     * @brief      get the dataset name
     *
     * @return     the dataset name
     */
    std::string get_name()
    {
        return _name;
    }


    /**
     * @brief      get the dataset rank
     *
     * @return     the dataset rank
     */
    std::size_t get_rank()
    {
        return _rank;
    }


    /**
     * @brief      get the current dataset extend
     *
     * @return     the dataset extend
     */
    auto get_extend()
    {
        return _extend;
    }


    /**
     * @brief      get the dataset capacity, i.e. the maximum extend
     *
     * @return     the dataset capacity
     */
    auto get_capacity()
    {
        return _max_extend;
    }


    /**
     * @brief      get the dataset ID
     *
     * @return     the dataset ID
     */
    hid_t get_id()
    {
        return _dataset;
    }


    /**
     * @brief      get the reference counter
     *
     * @return     the reference counter
     */
    auto get_referencecounter()
    {
        return _referencecounter;
    }


    /**
     * @brief      get the address of the dataset
     *
     * @return     the address of the dataset
     */
    haddr_t get_address()
    {
        return _address;
    }

    /**
     * @brief      add attribute to the dataset
     *
     * @param      attribute_name  The attribute name
     * @param      attribute_data  The attribute data
     *
     * @tparam     Attrdata        The type of the attribute data
     */
    template <typename Attrdata>
    void add_attribute(std::string attribute_name, Attrdata attribute_data)
    {
        // make attribute and write
        HDFAttribute attr(*this, attribute_name);
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
            }
            else
            {
                --(*_referencecounter)[_address];
            }
        }
    }

    /**
     * @brief      swap the state
     *
     * @param      other  The other
     */
    void swap(HDFDataset& other)
    {
        using std::swap;
        swap(_parent_object, other._parent_object);
        swap(_name, other._name);
        swap(_dataset, other._dataset);
        swap(_rank, other._rank);
        swap(_extend, other._extend);
        swap(_max_extend, other._max_extend);
        swap(_info, other._info);
        swap(_address, other._address);
        swap(_referencecounter, other._referencecounter);
    }

    /**
     * @brief      Write data, extracting fields from complex data like structs
     *             via an adaptor function. Adds an attribute which contains
     *             rank and size of the data. If you wish to write vector
     *             containers, make sure that you do return a reference from
     *             adaptor!
     *
     * @param      begin           The begin
     * @param      end             The end
     * @param      adaptor         The adaptor
     * @param      rank            The rank. Defaults to 1
     * @param      extend          The extend of the data to write
     * @param      max_size        The maximum size of the dataset
     * @param      chunksize       The chunksize; extension is only possible if
     *                             a nonzero value is given here
     * @param      compress_level  The compression level; only possible with
     *                             nonzero chunksize
     *
     * @tparam     Iter            The type of the iterator
     * @tparam     Adaptor         The type of the adaptor
     */
    template <typename Iter, typename Adaptor>
    void write(Iter begin,
               Iter end,
               Adaptor&& adaptor,
               hsize_t rank = 1,
               std::vector<hsize_t> extend = {},
               std::vector<hsize_t> max_size = {},
               hsize_t chunksize = 0,
               hsize_t compress_level = 0)
    {
        using result_type =
            typename HDFTypeFactory::result_type<decltype(adaptor(*begin))>::type;

        // get size of stuff to write
        hsize_t size = std::distance(begin, end);
        _rank = rank;

        // currently not supported rank -> throw error
        if (_rank > 2)
        {
            throw std::runtime_error("Cannot write dataset: Ranks higher than "
                                     "2 are not supported currently!");
        }

        if (_dataset == -1)
        { // dataset does not exist yet

            // build a new dataset and assign to _dataset member
            __create_dataset__<result_type>(size, rank, extend, max_size,
                                            chunksize, compress_level);

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false)
            {
                throw std::runtime_error("Trying to create dataset named "
                                         "'" + _name + "' resulted in invalid "
                                         "ID! Check your arguments.");
            }

            // gather data for dimensionality info: rank, extend, max_extend
            // TODO see #119 for changes needed here
            // https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues/119
            std::vector<hsize_t> dims(1 + _extend.size() + _max_extend.size());
            std::size_t i = 0;
            dims[i] = _rank;
            ++i;

            for (std::size_t j = 0; j < _extend.size(); ++j, ++i)
            {
                dims[i] = _extend[j];
            }

            for (std::size_t j = 0; j < _max_extend.size(); ++j, ++i)
            {
                dims[i] = _max_extend[i];
            }

            // store the dimensionality info attribute
            add_attribute<std::vector<hsize_t>>("dimensionality info", dims);
            
            // write dataset
            __write_dataset__(begin, end, std::forward<Adaptor&&>(adaptor),
                              H5S_ALL, H5S_ALL);
        }
        else
        { // dataset does exist

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false)
            {
                throw std::runtime_error("Existing dataset '" + _name + "' "
                                         "has an invalid ID. Has the dataset "
                                         "already been closed?");
            }

            // check if dataset can be extended, i.e. if extend < max_extend.
            for (std::size_t i = 0; i < _rank; ++i)
            {
                if ((_extend[i] == _max_extend[i]) && (_extend[i] != 0))
                {
                    throw std::runtime_error("Dataset cannot be extended! Its "
                                             "extend reached max_extend. Did "
                                             "you set a nonzero chunksize?");
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
                
                // get the offset, stride count and block vectors for selection
                // of the hyperslab
                std::vector<hsize_t> offset = _extend;
                std::vector<hsize_t> stride{1};
                std::vector<hsize_t> count = offset;
                std::vector<hsize_t> block{1};

                // make dataset larger
                _extend[0] += size;

                herr_t ext_err = H5Dset_extent(_dataset, _extend.data());
                if (ext_err < 0)
                {
                    throw std::runtime_error("1D dataset could not be "
                                             "extended!");
                }

                // gather information for dimensionality info attribute
                std::vector<hsize_t> dims(1 + _extend.size() + _max_extend.size());
                std::size_t i = 0;
                dims[i] = _rank;
                ++i;

                for (std::size_t j = 0; j < _extend.size(); ++j, ++i)
                {
                    dims[i] = _extend[j];
                }

                for (std::size_t j = 0; j < _max_extend.size(); ++j, ++i)
                {
                    dims[i] = _max_extend[i];
                }

                // add the attribute
                add_attribute<std::vector<hsize_t>>("dimensionality info", dims);

                // make subset selection and extract data and memory space
                auto spaces = __select_dataset_subset__(offset, stride,
                                                        block, count);
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
                std::vector<hsize_t> offset = _extend;
                std::vector<hsize_t> stride(_extend.size(), 1);
                std::vector<hsize_t> count = offset;
                std::vector<hsize_t> block(extend.size(), 1);
                count[0] = 1; // such that we add one more 'slice'

                // extend
                _extend[0] += 1; // add one more slice
                for (std::size_t i = 1; i < offset.size(); ++i)
                {
                    offset[i] = 0;
                }

                herr_t ext_err = H5Dset_extent(_dataset, _extend.data());
                if (ext_err < 0)
                {
                    throw std::runtime_error("ND dataset could not be "
                                             "extended!");
                }

                // gather information for dimensionality info attribute
                std::vector<hsize_t> dims(1 + _extend.size() + _max_extend.size());
                std::size_t i = 0;
                dims[i] = _rank;
                ++i;

                for (std::size_t j = 0; j < _extend.size(); ++j, ++i)
                {
                    dims[i] = _extend[j];
                }

                for (std::size_t j = 0; j < _max_extend.size(); ++j, ++i)
                {
                    dims[i] = _max_extend[i];
                }

                // store the attribute
                add_attribute<std::vector<hsize_t>>("dimensionality info",
                                                    dims);

                // make subset selection and extract data and memory space
                auto spaces = __select_dataset_subset__(offset, stride,
                                                        block, count);
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
     * @param      start         The start. If not given, the entire dataset
     *                           will be read.
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
            throw std::runtime_error("Existing dataset '" + _name + "' has an "
                                     "invalid ID. Has the dataset already "
                                     "been closed?");
        }

        // 1d dataset
        if (_rank == 1)
        {
            // assume that if start is empty, the entire dataset should be read
            if (start.size() == 0)
            {
                return __read_from_dataset__<desired_type>(_extend[0],
                                                           H5S_ALL, H5S_ALL);
            }
            else
            { // read a subset determined by start end stride
              // check that the arrays have the correct size:
                if (   start.size()  != _rank
                    || end.size()    != _rank
                    || stride.size() != _rank)
                {
                    throw std::invalid_argument("Cannot read dataset: start, "
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
                auto spaces = __select_dataset_subset__(start, stride,
                                                        block, count);
                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                // read
                auto buffer = __read_from_dataset__<desired_type>(count[0],
                                                                  dspace,
                                                                  memspace);

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
                // this is a 1d data which contains the multi - dimensional
                // stuff in a flattened version
                std::size_t buffersize = 1;
                for (std::size_t i = 0; i < _rank; ++i)
                {
                    buffersize *= _extend[i];
                }

                return __read_from_dataset__<desired_type>(buffersize,
                                                           H5S_ALL, H5S_ALL);
            }
            else
            { // read a subset determined by start end stride
                // check that the arrays have the correct size:
                if (   start.size()  != _rank
                    || end.size()    != _rank
                    || stride.size() != _rank)
                {
                    throw std::invalid_argument("Cannot read dataset: start, "
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
                auto spaces = __select_dataset_subset__(start, stride,
                                                        block, count);
                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                // determine buffersize
                std::size_t buffersize = 1;
                for (std::size_t i = 0; i < _rank; ++i)
                {
                    buffersize *= count[i];
                }

                // read
                auto buffer = __read_from_dataset__<desired_type>(buffersize,
                                                                  dspace,
                                                                  memspace);

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
          _name(other._name),
          _dataset(other._dataset),
          _rank(other._rank),
          _extend(other._extend),
          _max_extend(other._max_extend),
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
     * @brief      Constructor
     *
     * @param      parent_object  The parent object
     * @param      name           The name
     */
    HDFDataset(HDFObject& parent_object, std::string name)
        : _parent_object(std::make_shared<HDFObject>(parent_object)),
          _name(name),
          _dataset(-1),
          _rank(0),
          _extend({}),
          _max_extend({}),
          _referencecounter(parent_object.get_referencecounter())

    {
        // Try to find the dataset in the parent_object
        // If it is there, open it.
        // Else: postphone the dataset creation to the first write
        if (H5LTfind_dataset(_parent_object->get_id(), _name.c_str()) == 1)
        { // dataset exists
            // open it
            _dataset = H5Dopen(_parent_object->get_id(), _name.c_str(),
                               H5P_DEFAULT);

            // get dataspace and read out rank, extend, max_extend
            hid_t dataspace = H5Dget_space(_dataset);

            _rank = H5Sget_simple_extent_ndims(dataspace);
            _extend.resize(_rank);
            _max_extend.resize(_rank);

            H5Sget_simple_extent_dims(dataspace, _extend.data(),
                                      _max_extend.data());
            H5Sclose(dataspace);

            // Update info and reference counter
            H5Oget_info(_dataset, &_info);
            _address = _info.addr;
            (*_referencecounter)[_address] += 1;
        }
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
}; // end of HDFDataset class

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



/**
 * @brief      Try to guess a good chunksize for a dataset
 * @detail     This is based on the h5py method `guess_chunk` with the same
 *             objective. It tries to guess an appropriate chunk layout for
 *             for a dataset, using the current extend, the maximum extend,
 *             and the size of each element (in bytes).
 *             As in h5py, this method will allocate chunks only as large as
 *             the set CHUNKSIZE_MAX. Chunks are generally close to some power-of-2
 *             fraction of each axis, slightly favoring bigger values for the
 *             last index.
 *
 * @param      extend       The current extend of the dataset
 * @param      typesize     The size of each element in bytes
 * @param      max_extend   The maximum extend the dataset can have (currently
 *                          not taken into consideration for the calculation)
 */
// TODO not sure about the types here ... hsize_t really needed? const?
const std::vector<hsize_t> guess_chunksize(std::vector<hsize_t> extend,
                                           const hsize_t typesize,
                                           [[maybe_unused]] const std::vector<hsize_t> max_extend = {})
{    
    // Define some constants
    // TODO Better to define somewhere else?
    // Factor by which chunks are adjusted
    const unsigned int CHUNKSIZE_BASE = 16 * 1024;

    // Soft lower and hard upper limit at 8k and 1M, respectively
    const unsigned int CHUNKSIZE_MIN = 8 * 1024;
    const unsigned int CHUNKSIZE_MAX = 1024 * 1024;


    // Check that an extend is given; if not, the dataset is a scalar dataset
    // which does not allow chunking anyway
    auto rank = extend.size();
    if (rank == 0) {
        throw std::invalid_argument("Cannot guess chunksize for scalar "
                                    "dataset!");
    }

    std::cout << "guessing chunksize for:" << std::endl;
    std::cout << "  typesize:     " << typesize << std::endl;

    // For large typesizes, chunking makes no sense: a chunk needs to at least
    // extend to two elements which cannot be the case if the typesize (of a
    // single element) is larger than half the maximum chunksize
    if (typesize > CHUNKSIZE_MAX / 2) {
        std::cout << "  -> type size >= 1/2 max. chunksize" << std::endl;
        return std::vector<hsize_t>(rank, 1);
    }

    // Extend values can also be 0, indicating unlimited extend of that
    // dimension. To not run into further problems, guess a value for those.
    // h5py uses 1024 here, let's do the same
    // TODO figure out why this specific value is used
    std::replace(extend.begin(), extend.end(), 0, 1024);

    // Determine the byte size of a dataset with this extend
    auto bytes_dset = typesize * std::accumulate(extend.begin(), extend.end(),
                                                 1, std::multiplies<>());
    // NOTE The std::accumulate is used for calculating the product of entries

    // ... and the target size of each chunk
    double bytes_target = (CHUNKSIZE_BASE
                           * std::pow(2, std::log10(bytes_dset / 1024.*1024)));
    // NOTE this is a double as it is more convenient to calculate with ...

    // Ensure the target chunk size is between CHUNKSIZE_MIN and CHUNKSIZE_MAX
    // in order to not choose too large or too small chunks
    if (bytes_target > CHUNKSIZE_MAX) {
        bytes_target = CHUNKSIZE_MAX;
    }
    else if (bytes_target < CHUNKSIZE_MIN) {
        bytes_target = CHUNKSIZE_MIN;
    }

    std::cout << "  bytes_dset:   " << bytes_dset << std::endl;
    std::cout << "  bytes_target: " << bytes_target << std::endl;

    // Create the temporary target vector that will store the chunksize values.
    // It starts with a copy of the extend values. After optimization, a const
    // version of this is created and returned.
    std::vector<hsize_t> _chunks(extend);

    // ... and a variable that will store the size (in bytes) of this specific
    // chunk configuration
    unsigned long int bytes_chunks;

    /* Now optimize the chunks for each dimension by repeatedly looping over
     * the vector and dividing the values by two (rounding up).
     *
     * The loop is left when the following condition is fulfilled:
     *   smaller than target chunk size OR within 50% of target chunk size
     *   AND
     *   smaller than maximum chunk size
     * Also, if the typesize is larger 
     * 
     * NOTE:
     * Limit the optimization to 23 iterations per dimension; usually, we will
     * leave the loop much earlier; the _mean_ extend of the dataset would have
     * to be ~8M entries _per dimension_ to exhaust this optimization loop.
     */
    std::cout << "optimization:" << std::endl;
    for (unsigned int i=0; i < 23 * rank; i++)
    {
        // With the current values of the chunks, calculate the chunk size
        bytes_chunks = typesize * std::accumulate(_chunks.begin(),
                                                  _chunks.end(),
                                                  1, std::multiplies<>());
        std::cout << "  bytes_chunks: " << bytes_chunks;

        // If close enough to target size, optimization is finished
        if ((   bytes_chunks <= bytes_target
             || (std::abs(bytes_chunks - bytes_target) / bytes_target < 0.5))
            && bytes_chunks <= CHUNKSIZE_MAX)
        {
            std::cout << "  -> close enough to target size now" << std::endl;
            break;
        }

        // Divide the chunk size of the current axis by two, rounding upwards
        std::cout << "  -> reducing size of dim " << i%rank << std::endl;
        _chunks[i % rank] = 1 + ((_chunks[i % rank] - 1) / 2);
        // NOTE integer division fun; can do this because all are unsigned
        // and the chunks entry is always nonzero
    }

    // Create a const version of the temporary chunks vector
    const std::vector<hsize_t> chunks(_chunks);
    // TODO is this the best way to do this?

    return chunks;
}


} // namespace DataIO
} // namespace Utopia
#endif // HDFDATASET_HH
