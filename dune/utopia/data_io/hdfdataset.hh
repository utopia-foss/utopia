#ifndef HDFDATASET_HH
#define HDFDATASET_HH
#include "hdfattribute.hh"
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"
#include "hdfutilities.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <numeric>

namespace Utopia {
namespace DataIO {
template <class HDFObject> class HDFDataset {
private:
    // helper function for making a compressed dataset
    template <typename Datatype>
    hid_t __make_dataset_compressed__(std::size_t chunksize,
                                      std::size_t compress_level) {
        // create creation property list, set chunksize and compress level
        hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_deflate(plist, compress_level);
        H5Pset_chunk(plist, _rank, &chunksize);

        // make dataspace
        hid_t dspace =
            H5Screate_simple(_rank, _extend.data(), _max_extend.data());

        // make dataset and return
        return H5Dcreate(_parent_object->get_id(), _name.c_str(),
                         HDFTypeFactory::type<Datatype>(), dspace, H5P_DEFAULT,
                         plist, H5P_DEFAULT);
    }

    // helper function for making a non compressed dataset
    template <typename Datatype> hid_t __make_dataset__(std::size_t chunksize) {
        if (chunksize > 0) {

            // create creation property list, set chunksize and compress level
            hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
            H5Pset_chunk(plist, _rank, &chunksize);

            // make dataspace
            hid_t dspace =
                H5Screate_simple(_rank, _extend.data(), _max_extend.data());

            // create dataset and return
            return H5Dcreate(_parent_object->get_id(), _name.c_str(),
                             HDFTypeFactory::type<Datatype>(), dspace,
                             H5P_DEFAULT, plist, H5P_DEFAULT);
        } else {
            // create dataset right away

            return H5Dcreate(
                _parent_object->get_id(), _name.c_str(),
                HDFTypeFactory::type<Datatype>(),
                H5Screate_simple(_rank, _extend.data(), _max_extend.data()),
                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        }
    }

protected:
    std::shared_ptr<HDFObject> _parent_object;
    std::string _name;
    hid_t _dataset;
    hsize_t _rank;
    std::vector<hsize_t> _extend;
    std::vector<hsize_t> _max_extend;

public:
    /**
     * @brief get a weak ptr to the parent_object
     *
     * @return std::weak_ptr<HDFObject>
     */
    std::weak_ptr<HDFObject> get_parent() { return _parent_object; }

    std::string get_name() { return _name; }

    std::size_t get_rank() { return _rank; }

    auto get_extend() { return _extend; }

    auto get_capacity() { return _max_extend; }

    hid_t get_id() { return _dataset; }

    /**
     * @brief add attribute to the dataset
     *
     * @tparam Attrdata
     * @param attribute_name
     * @param attribute_data
     */
    template <typename Attrdata>
    void add_attribute(std::string attribute_name, Attrdata attribute_data) {

        HDFAttribute<HDFDataset, Attrdata> attribute(
            *this, std::forward<std::string &&>(attribute_name));

        attribute.write(attribute_data);
    }
    /**
     * @brief close the dataset
     *
     */
    void close() {
        if (H5Iis_valid(_dataset) > 0) {
            H5Dclose(_dataset);
        }
        _dataset = -1;
    }

    /**
     * @brief swap the state
     *
     * @param other
     */
    void swap(HDFDataset &other) {
        using std::swap;
        swap(_parent_object, other._parent_object);
        swap(_name, other._name);
        swap(_dataset, other._dataset);
        swap(_rank, other._rank);
        swap(_extend, other._extend);
        swap(_max_extend, other._max_extend);
    }

    /**
     * @brief Write data
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
    void write(Iter begin, Iter end,
               Adaptor &&adaptor = [](auto &value) { return value; },
               hsize_t rank = 1, std::vector<hsize_t> extend = {},
               std::vector<hsize_t> max_size = {}, std::size_t chunksize = 0,
               std::size_t compress_level = 0) {

        using result_type = std::decay_t<decltype(adaptor(*begin))>;
        // get size of stuff to write
        std::size_t size = std::distance(begin, end);
        _rank = rank;

        if (_dataset == -1) { // dataset does not exist
            if (chunksize > 0) {
                if (extend.size() == 0) {
                    _extend = std::vector<hsize_t>(_rank, size);

                } else {
                    _extend = extend;
                }
                if (max_size.size() == 0) {
                    _max_extend = std::vector<decltype(H5S_UNLIMITED)>(
                        rank, H5S_UNLIMITED);
                }
                if (compress_level >
                    0) { // compressed dataset: make a new 1d dataset
                    _dataset = __make_dataset_compressed__<result_type>(
                        chunksize, compress_level);
                } else {
                    _dataset = __make_dataset__<result_type>(_rank, chunksize);
                }
            } else { // chunk size 0 implies non-extendable and non - compressed
                     // dataset
                     //
                if (extend.size() == 0) {
                    _extend = std::vector<hsize_t>(_rank, size);
                } else {
                    _extend = extend;
                }
                if (max_size.size() == 0) {
                    _max_extend = _extend;
                }
                // make dataset if necessary
                if (_dataset == -1) {
                    _dataset = __make_dataset__<result_type>(chunksize);
                }
            }

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false) {
                throw std::runtime_error(
                    "dataset id invalid, has the dataset already been closed?");
            }

            // now that the dataset has been made let us write to it
            // buffering at first
            auto buffer = HDFBufferFactory::buffer(
                begin, end, std::forward<Adaptor &&>(adaptor));

            // write to buffer
            herr_t write_err =
                H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(), H5S_ALL,
                         H5S_ALL, H5P_DEFAULT, buffer.data());

            if (write_err < 0) {
                throw std::runtime_error("writing to 1d dataset failed!");
            }

        } else { // dataset does exist

            // check validity of own dataset id
            if (H5Iis_valid(_dataset) == false) {
                throw std::runtime_error(
                    "dataset id invalid, has the dataset already been closed?");
            }

            // check if the dataset can be extended, i.e. if extend <
            // max_extend.
            for (std::size_t i = 0; i < _rank; ++i) {
                if (_extend[i] == _max_extend[i]) {
                    throw std::runtime_error("dataset cannot be extended");
                }
            }

            //     // distinguish between ranks: treat 1d differently from nd
            if (_rank == 1) {
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
                // get the offset
                auto offset = _extend[0];
                hsize_t stride = 1;
                // make dataset larger
                _extend[0] += size;
                herr_t ext_err = H5Dset_extent(_dataset, _extend.data());
                if (ext_err < 0) {
                    throw std::runtime_error(
                        "1d dataset could not be extended ");
                }

                // select the new slab we just added for writing.
                hid_t dspace = H5Dget_space(_dataset);
                herr_t select_err = H5Sselect_hyperslab(
                    dspace, H5S_SELECT_SET, &offset, &stride, &size, nullptr);
                if (select_err < 0) {
                    throw std::runtime_error("1d hyperslab failed!");
                }

                // buffering
                auto buffer = HDFBufferFactory::buffer(
                    begin, end, std::forward<Adaptor &&>(adaptor));

                // write to buffer
                herr_t write_err =
                    H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(),
                             H5S_ALL, dspace, H5P_DEFAULT, buffer.data());

                if (write_err < 0) {
                    throw std::runtime_error("writing to 1d dataset failed!");
                }
            } else { // N- dimensional dataset

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
                count[0] = 1; // such that we add one more 'slice'

                // extend
                _extend[0] += 1; // add one more slice
                herr_t ext_err = H5Dset_extent(_dataset, _extend.data());
                if (ext_err < 0) {
                    throw std::runtime_error("nd enlargement failed");
                }

                // select the new slab we just added for writing.
                hid_t dspace = H5Dget_space(_dataset);
                herr_t select_err =
                    H5Sselect_hyperslab(dspace, H5S_SELECT_SET, offset.data(),
                                        stride.data(), count.data(), nullptr);
                if (select_err < 0) {
                    throw std::runtime_error("nd hyperslab failed");
                }

                // buffering
                auto buffer = HDFBufferFactory::buffer(
                    begin, end, std::forward<Adaptor &&>(adaptor));

                // write to buffer
                herr_t write_err =
                    H5Dwrite(_dataset, HDFTypeFactory::type<result_type>(),
                             H5S_ALL, dspace, H5P_DEFAULT, buffer.data());

                if (write_err < 0) {
                    throw std::runtime_error("writing to n-d dataset failed!");
                }
            }
        }
    }

    /**
     * @brief Overload to be able to give container temporary for
     writing
     *
     * @tparam Container
     * @tparam Adaptor
     * @param data
     * @param adaptor
     * @param rank
     * @param extend
     * @param max_size
     * @param chunksize
     * @param compress_level
     */
    template <typename Container, typename Adaptor>
    void write(const Container &data,
               Adaptor &&adaptor = [](auto &value) { return value; },
               hsize_t rank = 1, std::vector<hsize_t> extend = {},
               std::vector<hsize_t> max_size = {}, std::size_t chunksize = 0,
               std::size_t compress_level = 0) {
        write(data.begin(), data.end(), std::forward<Adaptor &&>(adaptor), rank,
              extend, max_size, chunksize, compress_level);
    }

    /**
     * @brief Overload to be able to give container temporary for
     writing
     *
     * @tparam Container
     * @tparam Adaptor
     * @param data
     * @param adaptor
     * @param rank
     * @param extend
     * @param max_size
     * @param chunksize
     * @param compress_level
     */
    template <typename Container, typename Adaptor>
    void write(Container &&data,
               Adaptor &&adaptor = [](auto &value) { return value; },
               hsize_t rank = 1, std::vector<hsize_t> extend = {},
               std::vector<hsize_t> max_size = {}, std::size_t chunksize = 0,
               std::size_t compress_level = 0) {
        write(data.begin(), data.end(), std::forward<Adaptor &&>(adaptor), rank,
              extend, max_size, chunksize, compress_level);
    }

    /**
     * @brief Read (a subset_of) a dataset
     *
     * @tparam result_type
     * @param start
     * @param end
     * @param stride
     * @return auto
     */
    template <typename result_type>
    std::vector<result_type> read(std::vector<hsize_t> start = {},
                                  std::vector<hsize_t> end = {},
                                  std::vector<hsize_t> stride = {}) {
        // check if dataset id is ok
        if (H5Iis_valid(_dataset) == false) {
            throw std::runtime_error(
                "dataset id invalid, has the dataset already been closed?");
        }

        // 1d dataset
        if (_rank == 1) {
            // assume that if start is empty, then the entire dataset should be
            // read;
            if (start.size() == 0) {
                hid_t type = H5Dget_type(_dataset);
                std::vector<result_type> buffer(_extend[0]);
                herr_t read_err = H5Dread(_dataset, type, H5S_ALL, H5S_ALL,
                                          H5P_DEFAULT, buffer.data());
                if (read_err < 0) {
                    throw std::runtime_error("Error reading 1d dataset");
                }
                H5Tclose(type);

                return buffer;
            } else { // read a subset determined by start end stride
                     // check that the arrays have the correct size:
                if (start.size() != _rank || end.size() != _rank ||
                    stride.size() != _rank) {
                    throw std::invalid_argument("array sizes != rank");
                }
                // determine the count to be read
                std::vector<hsize_t> count(start.size());
                for (std::size_t i = 0; i < start.size(); ++i) {
                    count[i] = (end[i] - start[i]) / stride[i];
                }

                // select the desired slab for reading in the
                hid_t dspace = H5Dget_space(_dataset);
                herr_t select_err =
                    H5Sselect_hyperslab(dspace, H5S_SELECT_SET, start.data(),
                                        stride.data(), count.data(), nullptr);
                if (select_err < 0) {
                    throw std::runtime_error("selection of hyperslab  to write "
                                             "in 1d dataset failed!");
                }

                // now make a buffer and read
                std::vector<result_type> buffer(count[0]);
                hid_t type = H5Dget_type(_dataset);
                herr_t read_err = H5Dread(_dataset, type, H5S_ALL, dspace,
                                          H5P_DEFAULT, buffer.data());
                H5Tclose(type);
                if (read_err < 0) {
                    throw std::runtime_error(
                        "Error  reading subset of 1d dataset");
                }
                return buffer;
            }
        } else { // N D dataset
            // assume that if start is empty, then the entire  dataset should be
            // read;
            if (start.size() == 0) {
                hid_t type = H5Dget_type(_dataset);

                // make a buffer and read
                // this is a 1d data which contains the multi - dimensional
                // stuff in a flattened version
                std::vector<result_type> buffer(
                    std::accumulate(_extend.begin(), _extend.end(), 0));

                herr_t read_err = H5Dread(_dataset, type, H5S_ALL, H5S_ALL,
                                          H5P_DEFAULT, buffer.data());
                if (read_err < 0) {
                    throw std::runtime_error("Error  reading 1d dataset");
                }
                H5Tclose(type);
                return buffer;
            } else { // read a subset determined by start end stride
                // check that the arrays have the correct size:
                if (start.size() != _rank || end.size() != _rank ||
                    stride.size() != _rank) {
                    throw std::invalid_argument(
                        "array arguments have to be the same size as rank");
                }
                //             // determine the count to be read
                //             std::vector<hsize_t> count(start.size());
                //             for (std::size_t i = 0; i < start.size(); ++i) {
                //                 count[i] = (end[i] - start[i]) / stride[i];
                //             }

                //             // select the desired slab for reading in the
                //             dataspace hid_t dspace = H5Dget_space(_dataset);
                //             herr_t select_err = H5Sselect_hyperslab(
                //                 dspace, H5S_SELECT_SET, &start, &stride,
                //                 &count, nullptr);
                //             if (select_err < 0) {
                //                 throw std::runtime_error("selection of
                //                 hyperslab to write
                //                 "
                //                                          "in 1d dataset
                //                                          failed!");
                //             }

                //             // now make buffer
                //             // this is a 1d data which contains the
                //             multi-dimensional stuff
                //             // in a flattened version
                //             std::vector<result_type> buffer(
                //                 std::accumulate(count.begin(), count.end(),
                //                 0));

                //             // get type and read
                //             hid_t type = H5Dget_type(_dataset);
                //             herr_t read_err = H5Dread(_dataset, type,
                //             H5S_ALL, dspace,
                //                                       H5P_DEFAULT,
                //                                       buffer.data());
                //             H5Tclose(type);
                //             if (read_err < 0) {
                //                 throw std::runtime_error(
                //                     "Error  reading subset of 1d dataset");
                //             }
                //             H5Tclose(type);
                //             return buffer;
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
    HDFDataset(const HDFDataset &other)
        : _parent_object(other._parent_objet), _name(other._name),
          _dataset(other._dataset), _rank(other._rank), _extend(other._extend),
          _max_extend(other._max_extend) {}

    /**
     * @brief Move constructor
     *
     * @param other
     */
    HDFDataset(HDFDataset &&other) : HDFDataset() { this->swap(other); }

    /**
     * @brief Assignment operator
     *
     * @param other
     * @return HDFDataset&
     */
    HDFDataset &operator=(HDFDataset other) {
        this->swap(other);
        return *this;
    }
    /**
     * @brief Constructor
     *
     * @param parent_object
     * @param name
     */
    HDFDataset(HDFObject &parent_object, std::string name)
        : _parent_object(std::make_shared<HDFObject>(parent_object)),
          _name(name), _dataset(-1), _rank(0), _extend({}), _max_extend({}) {

        // try to find the dataset in the parent_object, open if it is
        // there, else postphone the dataset creation to the first write
        if (H5LTfind_dataset(_parent_object->get_id(), _name.c_str()) > 0) {
            _dataset =
                H5Dopen(_parent_object->get_id(), _name.c_str(), H5P_DEFAULT);
            // get dataspace and read out extend and max extend
            hid_t dataspace = H5Dget_space(_dataset);

            _rank = H5Sget_simple_extent_ndims(dataspace);
            H5Sget_simple_extent_dims(dataspace, _extend.data(),
                                      _max_extend.data());
            H5Sclose(dataspace);
        }
    }

    /**
     * @brief Destructor
     *
     */
    virtual ~HDFDataset() {
        if (H5Iis_valid(_dataset) == true) {
            H5Dclose(_dataset);
        }
    }
}; // namespace DataIO
} // namespace DataIO
} // namespace Utopia
#endif