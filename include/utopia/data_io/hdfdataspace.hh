#ifndef UTOPIA_DATAIO_HDFDATASPACE_HH
#define UTOPIA_DATAIO_HDFDATASPACE_HH

#include <armadillo>
#include <stdexcept>

#include <hdf5.h>

#include "../core/logging.hh"
#include "../core/type_traits.hh"
#include "../core/ostream.hh"

#include "hdfobject.hh"
#include "hdfutilities.hh"


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
 * @brief Class that wraps an HDF5 dataspace and takes care of managing its
 *        resources.
 *
 */
class HDFDataspace final : public HDFObject< HDFCategory::dataspace >
{
  public:
    using Base = HDFObject< HDFCategory::dataspace >;
    /**
     * @brief Get thet dataspace's rank, i.e., number of dimensions
     *
     * @return auto rank of the dataspace
     */
    hsize_t
    rank()
    {
        if (is_valid())
        {
            return H5Sget_simple_extent_ndims(get_C_id());
        }
        else
        {
            throw std::runtime_error(
                "Error, trying to get rank of invalid dataspace");
            return 0;
        }
    }

    /**
     * @brief Get the properties object: size and capacity.
     * @note The dimensions can be infered from the size of the returned
     * vectors
     * @return auto pair containing (size, capacity) armadillo rowvectors
     */
    std::pair< arma::Row< hsize_t >, arma::Row< hsize_t > >
    get_properties()
    {
        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to get properties of invalid dataspace," +
                std::to_string(get_C_id()));
        }

        arma::Row< hsize_t > size;
        size.resize(H5Sget_simple_extent_ndims(get_C_id()));

        arma::Row< hsize_t > capacity;
        capacity.resize(size.size());

        H5Sget_simple_extent_dims(get_C_id(), size.memptr(), capacity.memptr());

        return std::make_pair(size, capacity);
    }

    /**
     * @brief Get the current size of the dataspace in each dimension
     *
     * @return  arma::Row< hsize_t > Vector containing the dataspace's current
     * size in each dimension
     */
    arma::Row< hsize_t >
    size()
    {
        return get_properties().first;
    }

    /**
     * @brief Get the capacity of the dataspace in each dimension
     *
     * @return  arma::Row< hsize_t > Vector containing the dataspace's current
     * capacity in each dimension
     */
    arma::Row< hsize_t >
    capacity()
    {
        return get_properties().second;
    }

    /**
     * @brief Open the dataspace - set it to be equivalent to any data that
     *        later will be used to write or read.
     *
     */
    void
    open()
    {

        this->_log->debug("Opening dataspace, setting it to H5S_ALL");
        // no explicit close function needed for H5S_ALL
        _id.open(H5S_ALL, [](hid_t) -> herr_t { return 1; });
        _path = "Dataspace_all";
    }

    /**
     * @brief Open the dataspace with an HDF5 object, i.e., dataset or attribute
     *
     * @tparam Object
     * @param o HDF5 object to retrieve the dataspace for
     */
    template < typename Object >
    void
    open(Object&& object)
    {

        this->_log->debug(
            "Opening dataspace of {}", generate_object_name(object));

        // open_dataspace is defined for attribute and dataset
        // in their respective headerfiles to provide uniform
        // interface for both, such that we do not have to
        // differentiate between them
        bind_to(open_dataspace(std::forward< Object >(object)),
                &H5Sclose,
                object.get_path() + " dataspace");

        _log = spdlog::get("data_io");
    }

    /**
     * @brief Open a new dataset of type 'simple', which is equivalent to a
     *        N-dimensional array of dimension N = 'rank', a given extent,
     *        and a given maximum capacity that in each dimension must be
     *        greater or equal to the extent.
     *
     * @param rank Dimension of the dataspace
     * @param extent Current extent of the dataspace
     * @param capacity Total capacity of the dataspace
     */
    void
    open(std::string          name,
         hsize_t              rank,
         arma::Row< hsize_t > extent,
         arma::Row< hsize_t > capacity)
    {

        this->_log->debug("Opening dataspace from scratch with rank {}, extent "
                          "{} and capacity {}",
                          rank,
                          Utils::str(extent),
                          Utils::str(capacity));
        if (capacity.size() == 0)
        {
            bind_to(
                H5Screate_simple(rank, extent.memptr(), NULL), &H5Sclose, name);
        }
        else
        {
            bind_to(H5Screate_simple(rank, extent.memptr(), capacity.memptr()),
                    &H5Sclose,
                    name);
        }

        _log = spdlog::get("data_io");
    }

    /**
     * @brief Get the selection bounding box, i.e., the start and end vector
     *        of the currently selected subset of the dataspace
     *
     * @return std::pair<arma::Row<hsize_t>, arma::Row<hsize_t>>
     */
    std::pair< arma::Row< hsize_t >, arma::Row< hsize_t > >
    get_selection_bounds()
    {
        arma::Row< hsize_t > start;
        arma::Row< hsize_t > end;

        if (is_valid())
        {
            hsize_t r = rank();
            start.resize(r);
            end.resize(r);

            if (H5Sget_select_bounds(get_C_id(), start.memptr(), end.memptr()) <
                0)
            {
                throw std::runtime_error(
                    "Error, cannot get selection bounds of invalid dataspace");
            }
        }
        return std::make_pair(start, end);
    }

    /**
     * @brief Select a slice in the dataspace defined by [start, end, stride]
     *        in the manner of numpy. Overwrites old selections
     *
     * @param start
     * @param end
     * @param stride
     */
    void
    select_slice(arma::Row< hsize_t > start,
                 arma::Row< hsize_t > end,
                 arma::Row< hsize_t > stride)
    {

        this->_log->debug(
            "Selecting slice in dataspace with start={}, end={}, stride={}",
            Utils::str(start),
            Utils::str(end),
            Utils::str(stride));

        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to select a slice in an invalid dataspace");
        }

        hsize_t r = rank();

        if ((start.n_elem != r) or (end.n_elem != r))
        {
            throw std::runtime_error(
                "Error, dimensionality of start and end has to be the same as "
                "the dataspace's rank");
        }
        // stride may not be given, and hence we have to check for
        // it in order to correctly compute the counts vector: divison
        // can be skipped when stride.size() == 0

        arma::Row< hsize_t > count;
        hsize_t*             strideptr = nullptr;
        if (stride.size() == 0)
        {
            count = ((end - start)).as_row();
        }
        else
        {
            count     = ((end - start) / stride).as_row();
            strideptr = stride.memptr();
        }

        auto err = H5Sselect_hyperslab(get_C_id(),
                                       H5S_SELECT_SET,
                                       start.memptr(),
                                       strideptr,
                                       count.memptr(),
                                       nullptr);
        if (err < 0)
        {
            throw std::runtime_error(
                "Error when trying to select slice in dataspace");
        }
    }

    /**
     * @brief Select the entire dataspace as hyperslap to be read/written to
     *
     */
    void
    select_all()
    {
        this->_log->debug("Selecting everything in dataspace");

        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to select everything of an invalid dataspace");
        }

        herr_t err = H5Sselect_all(get_C_id());
        if (err < 0)
        {
            throw std::runtime_error(
                "Error when trying to select entire dataspace");
        }
    }

    /**
     * @brief Resize the dataspace. The new size needs to fit into the
     * dataspaces capacity
     * @note Dataset needs to be chunked when new_size != capacity of the
     * dataspace
     *
     * @param new_size Desired new size of the dataspace
     */
    void
    resize(arma::Row< hsize_t > new_size)
    {
        this->_log->debug("Resizing dataset from {} to {}",
                          Utils::str(size()),
                          Utils::str(new_size));

        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to resize an invalid dataspace");
        }

        auto [current_extent, current_capacity] = get_properties();

        // make capacity bigger if needed
        auto new_capacity = arma::max(current_capacity, new_size);

        // resize the dataspace
        herr_t err = H5Sset_extent_simple(get_C_id(),
                                          new_size.size(),
                                          new_size.memptr(),
                                          current_capacity.memptr());

        if (err < 0)
        {
            throw std::runtime_error("Error in resizing dataspace");
        }
    }

    /**
     * @brief Release a previously defined selection
     *
     */
    void
    release_selection()
    {
        this->_log->debug("Releasing selection");

        if (not is_valid())
        {
            throw std::runtime_error(
                "Cannot reset selection, dataspace is invalid");
        }

        H5Sselect_none(get_C_id());
    }

    /**
     * @brief Construct HDFDataspace per default. Equivalent to using H5S_ALL
     * when employing the pure C interface.
     *
     */
    HDFDataspace()
    {
        open();
    }

    /**
     * @brief Copy constructor
     *
     */
    HDFDataspace(const HDFDataspace&) = default;

    /**
     * @brief Move constructor
     *
     */
    HDFDataspace(HDFDataspace&&) = default;

    /**
     * @brief Copy assign dataspace
     *
     * @return HDFDataspace&
     */
    HDFDataspace&
    operator=(const HDFDataspace&) = default;

    /**
     * @brief Move assign dataspace
     *
     * @return HDFDataspace&
     */
    HDFDataspace&
    operator=(HDFDataspace&&) = default;

    /**
     * @brief Construct HDFDataspace from the given arguments
     *
     * @param rank
     * @param extent
     * @param capacity
     */
    HDFDataspace(std::string            name,
                 hsize_t                rank,
                 std::vector< hsize_t > extent,
                 std::vector< hsize_t > capacity)
    {
        open(name, rank, extent, capacity);
    }

    /**
     * @brief Construct a new HDFDataspace object from an HDFDataset or
     * HDFAttribute. This loads the file-dataspace that belongs to the dataset
     * or attribute such that it gets managed by this class.
     * @tparam Object automatically dete    rmined
     * @param object object to open the dataset space to
     */
    template < typename Object >
    HDFDataspace(Object&& object,
                 std::enable_if_t<
                     not std::is_same_v< std::decay_t< Object >, HDFDataspace >,
                     int > = 0)
    {
        open(std::forward< Object >(object));
    }

    /**
     * @brief Destroy the HDFDataspace object
     *
     */
    virtual ~HDFDataspace() = default;

    /**
     * @brief Swap state with argument
     *
     * @param other Other HDFDataspace instance
     */
    void
    swap(HDFDataspace& other)
    {
        using std::swap;
        using Utopia::DataIO::swap;
        swap(static_cast< Base& >(*this), static_cast< Base& >(other));
    }
};

/**
 * @brief Swap states of lhs and rhs
 *
 * @param lhs Dataspace to swap states
 * @param rhs Dataspace to swap states
 */
void
swap(HDFDataspace& lhs, HDFDataspace& rhs)
{
    lhs.swap(rhs);
}

}

}
#endif
