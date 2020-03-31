#ifndef UTOPIA_DATAIO_HDFOBJECT_HH
#define UTOPIA_DATAIO_HDFOBJECT_HH

#include <hdf5.h>
#include <memory>

#include "hdfidentifier.hh"
#include "hdfutilities.hh"

#include <utopia/core/logging.hh>

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
 * @brief Common base class for all HDF5 classes in the DATAIO Module
 *        i.e., for all classes that wrap HDF5-C-Library functionality like
 *        HDFDataset, HDFGroup, HDFFile. This class is not intendet  as
 * something to be used independently, but only as a base class to be inherited
 * from.
 * @tparam HDFCategory type of object an instance of this class refers to
 */
template < HDFCategory objectcategory >
class HDFObject
{
  protected:
    /**
     * @brief Identifier object that binds an instance of this class to an HDF5
     * object
     *
     */
    HDFIdentifier _id;

    /**
     * @brief Name of the object
     *
     */
    std::string _path;

    /**
     * @brief pointer to the logger for dataio
     *
     */
    std::shared_ptr< spdlog::logger > _log;

  public:
    /**
     * @brief Named variable for template arg
     *
     */
    static constexpr HDFCategory category = objectcategory;

    /**
     * @brief swap the state of the caller with the state of the argument
     *
     * @param other
     */
    void
    swap(HDFObject& other)
    {
        using std::swap;
        swap(_id, other._id);
        swap(_path, other._path);
        swap(_log, other._log);
    }

    // implement open and close in a sensible way here, preferably without crtp,
    // and by employing the identifier's management abilities

    /**
     * @brief Get the name or path object
     *
     * @return std::string name of the object
     */
    std::string
    get_path() const
    {
        return _path;
    }

    /**
     * @brief Get the id object
     *
     * @return hid_t C-Library identifier held by this object
     */
    auto
    get_id_object() const
    {
        return _id;
    }

    /**
     * @brief Get the logger object
     *
     * @return auto
     */
    auto
    get_logger() const
    {
        return _log;
    }
    /**
     * @brief Get the C id object
     *
     * @return hid_t
     */
    hid_t
    get_C_id() const
    {
        return _id.get_id();
    }

    /**
     * @brief Get the reference count of object
     *
     * @return auto
     */
    auto
    get_refcount()
    {
        return _id.get_refcount();
    }

    /**
     * @brief Check if the object is still valid
     *
     * @return true
     * @return false
     */
    virtual bool
    is_valid() const
    {
        if (get_C_id() == -1)
        {
            return false;
        }
        else
        {
            // use check_validity to provide correct name
            return check_validity(H5Iis_valid(get_C_id()), _path);
        }
    }

    /**
     * @brief Close function which takes care of correctly closing the object
     *        and managing the reference counter.
     */
    void
    close()
    {

        if (is_valid())
        {
            _id.close();

            _path = "";
        }
    }

    // README: the 'bind_to' function is not named 'open' to avoid confusion wrt
    // to 'open' functions in derived classes. The latter have vastly differing
    // arguments and hence cannot be prototyped here. Furthermore their
    // functionality is more high level than this function.

    /**
     * @brief Open the object and bind it to a HDF5 object identified by 'id'
     * with name 'path'. Object should be created beforehand
     * @param id id of the object to bind to, by rvalue reference. This is made
     * such that the given id is 'stolen' by this object and reference count
     *           remains unchanged
     * @param path Name or path of the object to bind to
     */
    void
    bind_to(hid_t                          id,
            std::function< herr_t(hid_t) > closing_func,
            std::string                    path = {})
    {
        if (is_valid())
        {
            throw std::runtime_error("Error: Cannot bind object to new "
                                     "identifier while the old is still valid");
        }

        if (not check_validity(H5Iis_valid(id), path))
        {
            throw std::invalid_argument(
                "Error: invalid argument! The id given "
                "for an object of " +
                generate_object_name(*this) +
                " cannot be managed by an HDFObject instance!");
        }

        _log->debug("Opening object of ", generate_object_name(*this));

        _id.open(id, closing_func);

        if (path.size() == 0)
        {
            // putting some stupid special treatments here because HDF5
            // cannot handle things generically

            // attribute has to be treated separately
            if constexpr (category == HDFCategory::attribute)
            {
                _path.resize(H5Aget_name(get_C_id(), 0, _path.data()) + 1);

                H5Aget_name(get_C_id(), _path.size(), _path.data());
            }
            else
            {

                // a preliminary call to H5Iget_name will give the needed size.
                // cumbersome, but works. The +1 is for the null terminator

                _path.resize(H5Iget_name(get_C_id(), _path.data(), 0) + 1);

                // then, a second call will give the full name
                H5Iget_name(get_C_id(), _path.data(), _path.size());
            }

            // set proper null char. Perhaps needs to be done because of string,
            // which takes care of ending char itself, while H5Xget_name also
            // does?
            _path.pop_back();
        }
        else
        {
            _path = path;
        }
    }

    /**
     * @brief Construct HDFObject from the given arguments
     *
     */
    HDFObject() : _id(), _path(""), _log(spdlog::get("data_io"))
    {
    }

    /**
     * @brief Construct HDFObject by moving
     *
     */
    HDFObject(HDFObject&& other) :
        _id(std::move(other._id)), _path(std::move(other._path)),
        _log(std::move(other._log))
    {
        other._id.set_id(-1);
    }

    /**
     * @brief Construct HDFObject by copying another object
     *
     */
    HDFObject(const HDFObject& other) = default;

    /**
     * @brief Construct HDFObject from the given argument
     *
     * @param id C-Library identifier, by rvalue reference count. This is done
     * such that the id is 'stolen' by this object and hence reference counts
     * stay the same
     * @param path Name of the object
     */
    HDFObject(hid_t                          id,
              std::function< herr_t(hid_t) > closing_func,
              std::string                    path = {}) :
        _log(spdlog::get("data_io"))
    {
        bind_to(id, closing_func, path);
    }

    /**
     * @brief Copy assignment operator
     *
     * @param other
     * @return HDFObject&
     */
    HDFObject&
    operator=(const HDFObject& other)
    {
        _id   = other._id;
        _path = other._path;
        _log  = other._log;

        return *this;
    }

    /**
     * @brief move assignment operator
     *
     * @param other
     * @return HDFObject&
     */
    HDFObject&
    operator=(HDFObject&& other)
    {
        _id   = std::move(other._id);
        _path = std::move(other._path);
        _log  = std::move(other._log);

        other._id.set_id(-1);

        return *this;
    }

    /**
     * @brief Destroy the HDFObject object. Has to be implemented in subclass!
     *
     */
    virtual ~HDFObject()
    {
        close();
    }
};

/**
 * @brief Exchange state of lhs and rhs
 *
 * @tparam cat automatically determined
 * @param lhs
 * @param rhs
 */
template < HDFCategory cat >
void
swap(HDFObject< cat >& lhs, HDFObject< cat >& rhs)
{
    lhs.swap(rhs);
}

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO
}
}
#endif