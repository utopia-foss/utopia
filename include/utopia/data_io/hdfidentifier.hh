#ifndef UTOPIA_DATAIO_HDFIDENTIFIER_HH
#define UTOPIA_DATAIO_HDFIDENTIFIER_HH

#include <functional>

#include <hdf5.h>

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
 * @brief Wrapper class around an hdf5 identifier, used to manage reference
 * counts of the object this identifier refers to.
 */
class HDFIdentifier final
{
    hid_t                          _id = -1;
    std::function< herr_t(hid_t) > _closing_func;

  public:
    /**
     * @brief Exchange states between caller and 'other'
     *
     * @param other Identifier to exchange states with
     */
    void
    swap(HDFIdentifier& other)
    {
        using std::swap;
        swap(_id, other._id);
        swap(_closing_func, other._closing_func);
    }

    /**
     * @brief Get the HDF5 id held by this object
     *
     * @return hid_t
     */
    hid_t
    get_id() const
    {
        return _id;
    }

    /**
     * @brief Set id to the given argument. Only to be used to invalidate
     *        objects upon move or similar
     *
     * @param id
     */
    void
    set_id(hid_t id)
    {
        _id = id;
    }

    /**
     * @brief Check if thi ID refers to a valid object
     *
     * @return true
     * @return false
     */
    bool
    is_valid() const
    {
        return check_validity(H5Iis_valid(_id), "identifier");
    }

    /**
     * @brief Get the number of references currently referring to the object
     * identified by this ID
     *
     * @return auto
     */
    auto
    get_refcount() const
    {
        if (is_valid())
        {
            return H5Iget_ref(_id);
        }
        else
        {
            return -1;
        }
    }

    /**
     * @brief Increase the reference count of the object referred to by this ID
     * by one.
     *
     */
    void
    increment_refcount()
    {
        if (is_valid())
        {
            H5Iinc_ref(_id);
        }
    }

    /**
     * @brief Decrease the reference count of the object referred to by this ID
     * by one
     *
     */
    void
    decrement_refcount()
    {
        if (is_valid())
        {
            H5Idec_ref(_id);
        }
    }

    /**
     * @brief Close the identifier and render the C-Level id held invalid
     *
     */
    void
    close()
    {
        if (is_valid())
        {
            if (H5Iget_ref(_id) > 1)
            {
                decrement_refcount();
            }
            else
            {
                _closing_func(_id);
            }
        }
        _id = -1;
    }

    /**
     * @brief Open the object and bind it to another C-Level id
     *
     * @param id
     */
    void
    open(hid_t id, std::function< herr_t(hid_t) > closing_func)
    {
        if (is_valid())
        {
            throw std::runtime_error(
                "Error, HDFIdentifier cannot bind to new identifier while "
                "still being valid. Close first.");
        }
        _id           = id;
        _closing_func = closing_func;
    }

    /**
     * @brief Construct HDFIdentifier from the given arguments
     *
     * @param id C-Level id that is to be wrappend into an object of this class.
     *           Management of this id is then taken over by this class
     * instance. Continuing to do so using the C-Interface outside of this class
     *           will result in errors.
     */
    HDFIdentifier(hid_t id, std::function< herr_t(hid_t) > closing_func)
    {
        open(id, closing_func);
    }

    /**
     * @brief Construct HDFIdentifier from the given arguments
     *
     */
    HDFIdentifier() : _id(-1), _closing_func([](hid_t) -> herr_t { return 0; })
    {
    }

    /**
     * @brief Construct HDFIdentifier by copying another instance of
     * HDFIdentifier, incrementing the refcount of the held id in the process
     * @param other Object to copy from
     */
    HDFIdentifier(const HDFIdentifier& other) :
        _id(other._id), _closing_func(other._closing_func)
    {
        increment_refcount();

    }

    /**
     * @brief Construct HDFIdentifier by moving from another instance of
     * HDFIdentifier.
     *
     */
    HDFIdentifier(HDFIdentifier&& other) :
        _id(std::move(other._id)), _closing_func(std::move(other._closing_func))
    {
        other._id = -1;
    }

    /**
     * @brief Assign HDFIdentifier by copying another instance of HDFIdentifier,
     *        incrementing the refcount of the held id in the process
     * @param other Object to copy from
     */
    HDFIdentifier&
    operator=(const HDFIdentifier& other)
    {
        _id           = other._id;
        _closing_func = other._closing_func;

        increment_refcount();
        return *this;
    }

    /**
     * @brief Assign HDFIdentifier by moving from another instance of
     * HDFIdentifier.
     *
     *
     * @return HDFIdentifier&
     */
    HDFIdentifier&
    operator=(HDFIdentifier&& other)
    {
        _id       = std::move(other._id);
        other._id = -1;
        return *this;
    }

    /**
     * @brief Destroy the HDFIdentifier object, decrementing its refcount
     *
     */
    ~HDFIdentifier()
    {
        close();
    }
};

// comparison operators

/**
 * @brief Comparsion operator for equality
 *
 * @param other Object to compare with
 * @return true other == this
 * @return false otherwise
 */
bool
operator==(const HDFIdentifier& lhs, const HDFIdentifier& rhs)
{
    return rhs.get_id() == lhs.get_id();
}

/**
 * @brief Comparsion operator for inequality
 *
 * @param other Object to compare with
 * @return true other != this
 * @return false otherwise
 */
bool
operator!=(const HDFIdentifier& lhs, const HDFIdentifier& rhs)
{
    return not(lhs == rhs);
}

/**
 * @brief Exchange the states of lhs and rhs
 *
 * @param lhs
 * @param rhs
 */
void
swap(HDFIdentifier& lhs, HDFIdentifier& rhs)
{
    lhs.swap(rhs);
}

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

}
}

#endif
