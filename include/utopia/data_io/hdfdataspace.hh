#ifndef UTOPIA_DATAIO_HDFDATASPACE_HH
#define UTOPIA_DATAIO_HDFDATASPACE_HH

#include <armadillo>
#include <stdexcept>

#include <H5Spublic.h>
#include <hdf5.h>

#include "hdfutilities.hh"

#include "../core/logging.hh"
#include "../core/utils.hh"

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
class HDFDataspace final
{
  private:
    /// H5 dataspace identifier
    hid_t _dataspace = -1;

    /// Pointer to logger used by this instance
    std::shared_ptr< spdlog::logger > _log = spdlog::get("data_io");

    // nonthrowing versions of functions

  public:
    /**
     * @brief Get thet dataspace's rank, i.e., number of dimensions
     *
     * @return auto rank of the dataspace
     */
    hsize_t
    rank() const
    {
        if (not is_valid())
        {

            throw std::runtime_error(
                "Error, trying to get rank of invalid dataspace");
        }

        return H5Sget_simple_extent_ndims(_dataspace);
    }

    /**
     * @brief Get the id object
     *
     * @return hid_t
     */
    hid_t
    get_id() const
    {
        return _dataspace;
    }

    /**
     * @brief Get the properties object: size and capacity.
     * @notice The dimensions can be infered from the size of the returned
     * vectors
     * @return auto pair containing (size, capacity) armadillo rowvectors
     */
    std::pair< arma::Row< hsize_t >, arma::Row< hsize_t > >
    get_properties() const
    {
        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to get properties of invalid dataspace," +
                std::to_string(_dataspace));
        }
        
        arma::Row< hsize_t > size;
        size.resize(H5Sget_simple_extent_ndims(_dataspace));

        arma::Row< hsize_t > capacity;
        capacity.resize(size.size());

        H5Sget_simple_extent_dims(_dataspace, size.memptr(), capacity.memptr());

        return std::make_pair(size, capacity);
    }

    /**
     * @brief Get the current size of the dataspace in each dimension
     *
     * @return  arma::Row< hsize_t > Vector containing the dataspace's current
     * size in each dimension
     */
    arma::Row< hsize_t >
    size() const
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
    capacity() const
    {
        return get_properties().second;
    }

    /**
     * @brief Open the dataspace - set it to be equivalent to any data that
     *        later will be used to write or read. If the dataspace has been
     *        opened before, then it has to be closed first.
     *
     */
    void
    open()
    {
        if (is_valid())
        {
            throw std::runtime_error(
                "Error, trying to open a dataspace that is already open");
        }
        else
        {
            _dataspace = H5S_ALL;
        }
    }

    /**
     * @brief Open the dataspace with an HDF5 object, i.e., dataset or attribute
     *        If the dataspace has been opened before, then it has to be closed
     *        first.
     *
     * @tparam Object
     * @param o HDF5 object to retrieve the dataspace for
     */
    template < typename Object >
    void
    open(Object&& object)
    {
        if (is_valid())
        {
            throw std::runtime_error(
                "Error, trying to open a dataspace that is already open");
        }
        else
        {
            // open_dataspace is defined for attribute and dataset
            // in their respective headerfiles to provide uniform
            // interface for both, such that we do not have to
            // differentiate between them
            _dataspace = open_dataspace(std::forward< Object >(object));
        }
    }

    /**
     * @brief Open a new dataset of type 'simple', which is equivalent to a
     *        N-dimensional array of dimension N = 'rank', a given extent,
     *        and a given maximum capacity that in each dimension must be
     *        greater or equal to the extent. If the dataspace has been
     *        opened before, then it has to be closed first.
     *
     * @param rank Dimension of the dataspace
     * @param extent Current extent of the dataspace
     * @param capacity Total capacity of the dataspace
     */
    void
    open(hsize_t              rank,
         arma::Row< hsize_t > extent,
         arma::Row< hsize_t > capacity)
    {

        if (is_valid())
        {
            throw std::runtime_error(
                "Error, trying to open a dataspace that is already open");
        }
        else
        {

            if (capacity.size() == 0)
            {
                _dataspace = H5Screate_simple(rank, extent.memptr(), NULL);
            }
            else
            {
                _dataspace =
                    H5Screate_simple(rank, extent.memptr(), capacity.memptr());
            }
        }
    }

    /**
     * @brief Tells if this object represents a valid dataspace the HDF5 C
     * library can use, and which has not been released
     *
     * @return true
     * @return false
     */
    bool
    is_valid() const
    {
        return check_validity(H5Iis_valid(_dataspace), "dataspace");
    }

    /**
     * @brief Close the dataspace object and release its resources
     */
    void
    close()
    {
        if (is_valid())
        {
            H5Sclose(_dataspace);
            _dataspace = -1;
        }
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

        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to get selection bounds for invalid dataspace");
        }

        const hsize_t r = rank();

        arma::Row< hsize_t > start(r, arma::fill::zeros);
        arma::Row< hsize_t > end(r, arma::fill::zeros);

        const herr_t err =
            H5Sget_select_bounds(_dataspace, start.memptr(), end.memptr());

        if (err < 0)
        {
            throw std::runtime_error(
                "Error when trying to get selection bounds for dataspace");
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
        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to select a slice in an invalid dataspace");
        }

        hsize_t r = rank();

        if ((start.n_elem != r) or (end.n_elem != r))
        {
            throw std::runtime_error("Error, dimensionality of start and "
                                     "end has to be the same as "
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

        const herr_t err = H5Sselect_hyperslab(_dataspace,
                                               H5S_SELECT_SET,
                                               start.memptr(),
                                               strideptr,
                                               count.memptr(),
                                               nullptr);
        if (err < 0)
        {
            throw std::runtime_error(
                "Error when trying to select slice in dataset");
        }
    }

    /**
     * @brief Select the entire dataspace as hyperslap to be read/written to
     *
     */
    void
    select_all()
    {
        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to select everything of an invalid dataspace");
        }

        release_selection();

        const herr_t err = H5Sselect_all(_dataspace);

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
        if (not is_valid())
        {
            throw std::runtime_error(
                "Error, trying to resize an invalid dataspace");
        }

        auto [current_extent, current_capacity] = get_properties();

        // make capacity bigger if needed
        auto new_capacity = arma::max(current_capacity, new_size);

        // resize the dataspace
        const herr_t err = H5Sset_extent_simple(_dataspace,
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
        if (not is_valid())
        {
            throw std::runtime_error(
                "Cannot reset selection, dataspace is invalid");
        }

        H5Sselect_none(_dataspace);
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
    HDFDataspace(hsize_t                rank,
                 std::vector< hsize_t > extent,
                 std::vector< hsize_t > capacity)
    {
        open(rank, extent, capacity);
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
                     int > = 0) :
        _dataspace(open_dataspace(std::forward< Object >(object)))
    {
    }

    /**
     * @brief Destroy the HDFDataspace object
     *
     */
    ~HDFDataspace()
    {
        close();
    }

    /**
     * @brief Swap state with argument
     *
     * @param other Other HDFDataspace instance
     */
    void
    swap(HDFDataspace& other)
    {
        using std::swap;
        swap(_dataspace, other._dataspace);
        swap(_log, other._log);
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