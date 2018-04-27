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
 * @brief Class representing a HDFDataset, wich reads and writes data
 *        and attributes
 *
 * @tparam HDFObject
 */
template <class HDFObject>
class HDFDataset
{
private:
    // helper function for making a non compressed dataset
    template <typename Datatype>
    hid_t __create_dataset_helper__(hsize_t chunksize, hsize_t compress_level)
    {
        hid_t group_plist = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(group_plist, 1);

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
            hid_t dspace = H5Screate_simple(_rank, _extend.data(), _max_extend.data());

            // create dataset and return
            return H5Dcreate(_parent_object->get_id(), _name.c_str(),
                             HDFTypeFactory::type<Datatype>(), dspace,
                             group_plist, plist, H5P_DEFAULT);
        }
        else
        {
            // create dataset right away
            return H5Dcreate(_parent_object->get_id(), _name.c_str(),
                             HDFTypeFactory::type<Datatype>(),
                             H5Screate_simple(_rank, _extend.data(), _max_extend.data()),
                             group_plist, H5P_DEFAULT, H5P_DEFAULT);
        }
    }

    // wrapper for creating a dataset given all parameters needed
    template <typename result_type>
    void __create_dataset__(hsize_t size,
                            hsize_t rank,
                            std::vector<hsize_t> extend,
                            std::vector<hsize_t> max_size,
                            hsize_t chunksize,
                            hsize_t compress_level)
    {
        _rank = rank;
        if (chunksize > 0)
        {
            if (extend.size() == 0)
            {
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
            if (compress_level > 0)
            {
                // compressed dataset: make a new dataset
                _dataset = __create_dataset_helper__<result_type>(chunksize, compress_level);
            }
            else
            {
                _dataset = __create_dataset_helper__<result_type>(chunksize, compress_level);
            }
        }
        else
        { // chunk size 0 implies non-extendable and non - compressed
          // dataset
          //

            if (extend.size() == 0)
            {
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
            if (_dataset == -1)
            {
                _dataset = __create_dataset_helper__<result_type>(chunksize, compress_level);
            }
        }
    }

    // for writing a  dataset
    template <typename Iter, typename Adaptor>
    void __write_dataset__(Iter begin, Iter end, Adaptor&& adaptor, hid_t dspace, hid_t memspace)
    {
        using result_type =
            typename HDFTypeFactory::result_type<decltype(adaptor(*begin))>::type;
        // now that the dataset has been made let us write to it
        // buffering at first
        auto buffer =
            HDFBufferFactory::buffer(begin, end, std::forward<Adaptor&&>(adaptor));

        // std::cout << "buffer test: " << buffer.back().len << std::endl;

        // write to buffer
        herr_t write_err = H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(),
                                    memspace, dspace, H5P_DEFAULT, buffer.data());
        if (write_err < 0)
        {
            throw std::runtime_error("writing to 1d dataset failed!");
        }
    }

    // helper for selecting subset of dataset
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
            throw std::runtime_error("1d hyperslab failed!");
        }
        return {dspace, memspace};
    }

    // overload for std::string
    template <typename desired_type>
    auto __read_from_dataset__(hsize_t buffer_size, hid_t dspace, hid_t memspace)
    {
        if constexpr (is_container<desired_type>::value == true)
        {
            if constexpr (std::is_same_v<std::string, desired_type> == true)
            {
                std::vector<const char*> buffer(buffer_size);
                hid_t type = H5Dget_type(_dataset);
                herr_t read_err = H5Dread(_dataset, type, memspace, dspace,
                                          H5P_DEFAULT, buffer.data());
                H5Tclose(type);
                if (read_err < 0)
                {
                    throw std::runtime_error("Error reading 1d dataset");
                }
                std::vector<std::string> data(buffer_size);
                for (std::size_t i = 0; i < buffer_size; ++i)
                {
                    data[i] = std::string(buffer[i]);
                }

                return data;
            }
            else
            {
                std::vector<hvl_t> buffer(buffer_size);
                std::vector<desired_type> data(buffer_size);

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

protected:
    std::shared_ptr<HDFObject> _parent_object;
    std::string _name;
    hid_t _dataset;
    hsize_t _rank;
    std::vector<hsize_t> _extend;
    std::vector<hsize_t> _max_extend;
    H5O_info_t _info;
    haddr_t _address;
    std::shared_ptr<std::unordered_map<haddr_t, int>> _refcounts;

public:
    /**
     * @brief get a weak ptr to the parent_object
     *
     * @return std::weak_ptr<HDFObject>
     */
    std::shared_ptr<HDFObject> get_parent()
    {
        return _parent_object;
    }

    std::string get_name()
    {
        return _name;
    }

    std::size_t get_rank()
    {
        return _rank;
    }

    auto get_extend()
    {
        return _extend;
    }

    auto get_capacity()
    {
        return _max_extend;
    }

    hid_t get_id()
    {
        return _dataset;
    }

    auto get_refcounts()
    {
        return _refcounts;
    }

    haddr_t get_address()
    {
        return _address;
    }

    /**
     * @brief add attribute to the dataset
     *
     * @tparam Attrdata
     * @param attribute_name
     * @param attribute_data
     */
    template <typename Attrdata>
    void add_attribute(std::string attribute_name, Attrdata attribute_data)
    {
        // make attribute and write

        HDFAttribute<HDFDataset>(*this, attribute_name).write(attribute_data);
    }

    /**
     * @brief close the dataset
     *
     */
    void close()
    {
        if (H5Iis_valid(_dataset) == true)
        {
            if ((*_refcounts)[_address] == 1)
            {
                H5Dclose(_dataset);
                _refcounts->erase(_refcounts->find(_address));
            }
            else
            {
                --(*_refcounts)[_address];
            }
        }
    }

    /**
     * @brief swap the state
     *
     * @param other
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
        swap(_refcounts, other._refcounts);
    }

    /**
     * @brief Write data, extracting fields from complex data like structs via a
     * adaptor function. Adds an attribute which contains rank and size of the
     * data. If you wish to write vector containers, make sure that you do
     * return a reference from adaptor!
     *
     * @tparam Iter
     * @tparam Adaptor
     * @param begin
     * @param end
     * @param adaptor
     * @param chunksize
     * @param compress_level
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
            throw std::runtime_error(
                "ranks higher than 2 not supported currently");
        }

        if (_dataset == -1)
        { // dataset does not exist

            // build a new dataset anda assign to _dataset member
            __create_dataset__<result_type>(size, rank, extend, max_size,
                                            chunksize, compress_level);

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false)
            {
                throw std::runtime_error(
                    "dataset id invalid, has the dataset already been closed?");
            }

            // add attribute with dimensionality
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

            add_attribute<std::vector<hsize_t>>("dimensionality info", dims);
            // write dataset
            __write_dataset__(begin, end, std::forward<Adaptor&&>(adaptor), H5S_ALL, H5S_ALL);
        }
        else
        { // dataset does exist

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false)
            {
                throw std::runtime_error(
                    "dataset id invalid, has the dataset already been closed?");
            }

            // check if the dataset can be extended, i.e. if extend <
            // max_extend.
            for (std::size_t i = 0; i < _rank; ++i)
            {
                if (_extend[i] == _max_extend[i])
                {
                    if (_extend[i] != 0)
                    {
                        throw std::runtime_error("dataset cannot be extended");
                    }
                }
            }

            //     // distinguish between ranks: treat 1d differently from nd
            if (_rank == 1)
            {
                /* if the dataset can be extended, then do so now:
                 extend the first dimension in the dataspace, such
                 that
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
                // hyperslab
                std::vector<hsize_t> offset = _extend;
                std::vector<hsize_t> stride{1};
                std::vector<hsize_t> count = offset;
                std::vector<hsize_t> block{1};

                // make dataset larger
                _extend[0] += size;

                herr_t ext_err = H5Dset_extent(_dataset, _extend.data());
                if (ext_err < 0)
                {
                    throw std::runtime_error(
                        "1d dataset could not be extended ");
                }

                // add dimensionality attribute
                // add attribute with dimensionality
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
                add_attribute<std::vector<hsize_t>>("dimensionality info", dims);

                auto spaces = __select_dataset_subset__(offset, stride, block, count);

                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

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

                /* if the dataset can be extended, then do so now:
                extend the first dimension in the dataspace, such
                that
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
                    throw std::runtime_error("nd enlargement failed");
                }

                // add dimensionality attribute
                // add attribute with dimensionality
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
                add_attribute<std::vector<hsize_t>>("dimensionality info", dims);

                // make subset selectiion
                auto spaces = __select_dataset_subset__(offset, stride, block, count);

                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];

                __write_dataset__(begin, end, std::forward<Adaptor&&>(adaptor),
                                  dspace, memspace);

                H5Sclose(memspace);
                H5Sclose(dspace);
            }
        }
    }

    /**
     * @brief Read (a subset_of) a dataset. Flattens nd datasets
     *
     * @tparam result_type
     * @param start
     * @param end
     * @param stride
     * @return auto
     */
    template <typename desired_type>
    auto read(std::vector<hsize_t> start = {},
              std::vector<hsize_t> end = {},
              std::vector<hsize_t> stride = {})
    {
        // check if dataset id is ok
        if (H5Iis_valid(_dataset) == false)
        {
            throw std::runtime_error(
                "dataset id invalid, has the dataset already been closed?");
        }

        // 1d dataset
        if (_rank == 1)
        {
            // assume that if start is empty, then the entire dataset should be
            // read;
            if (start.size() == 0)
            {
                return __read_from_dataset__<desired_type>(_extend[0], H5S_ALL, H5S_ALL);
            }
            else
            { // read a subset determined by start end stride
              // check that the arrays have the correct size:
                if (start.size() != _rank || end.size() != _rank || stride.size() != _rank)
                {
                    throw std::invalid_argument("array sizes != rank");
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

                // close memoryspace resources
                H5Sclose(dspace);
                H5Sclose(memspace);

                return buffer;
            }
        }
        else
        { // N D dataset
            // assume that if start is empty, then the entire  dataset should be
            // read;
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

                return __read_from_dataset__<desired_type>(buffersize, H5S_ALL, H5S_ALL);
            }
            else
            { // read a subset determined by start end stride
                // check that the arrays have the correct size:
                if (start.size() != _rank || end.size() != _rank || stride.size() != _rank)
                {
                    throw std::invalid_argument(
                        "array arguments have to be the same size as rank");
                }
                // determine the count to be read
                std::vector<hsize_t> count(start.size());
                for (std::size_t i = 0; i < start.size(); ++i)
                {
                    count[i] = (end[i] - start[i]) / stride[i];
                }
                std::vector<hsize_t> block(count.size(), 1);

                auto spaces = __select_dataset_subset__(start, stride, block, count);

                hid_t dspace = spaces[0];
                hid_t memspace = spaces[1];
                std::size_t buffersize = 1;
                for (std::size_t i = 0; i < _rank; ++i)
                {
                    buffersize *= count[i];
                }

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
     * @brief default consturctor
     *
     */
    HDFDataset() = default;

    /**
     * @brief Copy constructor
     *
     * @param other
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
          _refcounts(other._refcounts)
    {
    }

    /**
     * @brief Move constructor
     *
     * @param other
     */
    HDFDataset(HDFDataset&& other) : HDFDataset()
    {
        this->swap(other);
    }

    /**
     * @brief Assignment operator
     *
     * @param other
     * @return HDFDataset&
     */
    HDFDataset& operator=(HDFDataset other)
    {
        this->swap(other);
        return *this;
    }
    /**
     * @brief Constructor
     *
     * @param parent_object
     * @param name
     */
    HDFDataset(HDFObject& parent_object, std::string name)
        : _parent_object(std::make_shared<HDFObject>(parent_object)),
          _name(name),
          _dataset(-1),
          _rank(0),
          _extend({}),
          _max_extend({}),
          _refcounts(parent_object.get_refcounts())

    {
        // try to find the dataset in the parent_object, open if it is
        // there, else postphone the dataset creation to the first write
        if (H5LTfind_dataset(_parent_object->get_id(), _name.c_str()) == 1)
        {
            _dataset = H5Dopen(_parent_object->get_id(), _name.c_str(), H5P_DEFAULT);
            // get dataspace and read out extend and max extend
            hid_t dataspace = H5Dget_space(_dataset);

            _rank = H5Sget_simple_extent_ndims(dataspace);
            _extend.resize(_rank);
            _max_extend.resize(_rank);
            H5Sget_simple_extent_dims(dataspace, _extend.data(), _max_extend.data());
            H5Sclose(dataspace);

            H5Oget_info(_dataset, &_info);
            _address = _info.addr;
            (*_refcounts)[_address] += 1;
        }
    }

    /**
     * @brief Destructor
     *
     */
    virtual ~HDFDataset()
    {
        if (H5Iis_valid(_dataset) == true)
        {
            if ((*_refcounts)[_address] == 1)
            {
                H5Dclose(_dataset);
                _refcounts->erase(_refcounts->find(_address));
            }
            else
            {
                --(*_refcounts)[_address];
            }
        }
    }
};

/**
 * @brief EXchange state between lhs and rhs
 *
 * @tparam HDFObject
 * @param lhs
 * @param rhs
 */
template <typename HDFObject>
void swap(HDFDataset<HDFObject>& lhs, HDFDataset<HDFObject>& rhs)
{
    lhs.swap(rhs);
}

} // namespace DataIO
} // namespace Utopia
#endif