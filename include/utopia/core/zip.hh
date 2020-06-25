#ifndef UTOPIA_CORE_ZIP_HH
#define UTOPIA_CORE_ZIP_HH

#include <iterator>
#include <tuple>
#include <type_traits>
#include <iterator>
#include <iostream>

#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/for_each.hpp>

#include "type_traits.hh"

namespace Utopia
{

/// Tools for iterating over collections
/** \ingroup Iter
 */
namespace Itertools
{

/// Iterator over an arbitrary number of collections
/** This object stores an iterator for each collection it iterates over and
 *  obeys the lowest-level iterator category of all inserted iterators.
 *
 *  Most operations on this iterator are forwarded to the underlying
 *  iterators.
 *  Dereferencing packs the resulting references to values of the collections
 *  into an `std::tuple` and returns it.
 *
 *  \note This iterator is invalidated as soon as any of the underlying
 *        iterators is invalidated.
 *
 *  \tparam Iters The types of iterators used within this object.
 *  \ingroup Iter
 */
template <typename... Iters>
class ZipIterator
{
private:
    using Tuple = std::tuple<Iters...>;

    using RefTuple = std::tuple<decltype(*std::declval<Iters>())...>;

    using PtrTuple = std::tuple<typename std::iterator_traits<Iters>::pointer...>;

    Tuple _iterators;

    using This = ZipIterator<Iters...>;

public:
    using value_type = std::tuple<typename std::iterator_traits<Iters>::value_type...>;
    using difference_type = int;
    using pointer = PtrTuple;
    using reference = RefTuple;
    using iterator_category =
        std::common_type_t<typename std::iterator_traits<Iters>::iterator_category...>;

    /// Increment by prefix
    /** \return Reference to this object after increment
     */
    ZipIterator& operator++()
    {
        boost::hana::for_each(_iterators, [](auto&& iter) { ++iter; });
        return *this;
    }

    /// Decrement by prefix
    /** \return Reference to this object after decrement
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::bidirectional_iterator_tag,
                                  category>, int> = 0>
    ZipIterator& operator--()
    {
        boost::hana::for_each(_iterators, [](auto&& iter) { --iter; });
        return *this;
    }

    /// Increment by postfix
    /** \return Copy of this object *before* increment
     */
    ZipIterator operator++(int)
    {
        This copy(*this);
        boost::hana::for_each(_iterators, [](auto&& iter) { iter++; });
        return copy;
    }

    /// Decrement by postfix
    /** \return Copy of this object *before* decrement
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::bidirectional_iterator_tag,
                                  category>, int> = 0>
    ZipIterator operator--(int)
    {
        This copy(*this);
        boost::hana::for_each(_iterators, [](auto&& iter) { iter--; });
        return copy;
    }

    /// Increment by a number of steps
    /** \param n The number of incrementation steps
     *  \return A copy of this object which has been incremented by `n`.
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    ZipIterator operator+(const difference_type n) const
    {
        This copy(*this);
        return copy += n;
    }

    /// Decrement by a number of steps
    /** \param n The number of decrementation steps
     *  \return A copy of this object which has been decremented by `n`.
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    ZipIterator operator-(const difference_type n) const
    {
        This copy(*this);
        return copy -= n;
    }

    /// Compute the difference between two iterators
    /** \param other The iterator to compare this object to
     *  \return The difference in steps
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    difference_type operator-(const ZipIterator& other) const
    {
        return std::get<0>(_iterators) - std::get<0>(other._iterators);
    }

    /// Increment in-place by a number of steps
    /** \param n The number of incrementation steps
     *  \return Reference to this object after incrementing it by `n`.
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    ZipIterator& operator+=(const difference_type n)
    {
        boost::hana::for_each(_iterators, [&n](auto&& iter) { iter += n; });
        return *this;
    }

    /// Decrement in-place by a number of steps
    /** \param n The number of decrementation steps
     *  \return Reference to this object after decrementing it by `n`.
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    ZipIterator& operator-=(const difference_type n)
    {
        boost::hana::for_each(_iterators, [&n](auto&& iter) { iter -= n; });
        return *this;
    }

    /// Dereference this iterator at its current position
    /** \return Const tuple of references to underlying object values
     */
    const reference operator*() const
    {
        return boost::hana::transform(
            _iterators,
            [](auto&& iter) { return std::ref(*iter); });
    }

    /// Dereference this iterator at its current position
    /** \return Tuple of references to underlying object values
     */
    reference operator*()
    {
        return boost::hana::transform(
            _iterators,
            [](auto&& iter) { return std::ref(*iter); });
    }

    /// Indirect this pointer at its current position
    /** \return Const tuple of pointers to underlying object values
     */
    const pointer operator->() const
    {
        return boost::hana::transform(
            _iterators,
            [](auto&& iter) { return &(*iter); });
    }

    /// Indirect this pointer at its current position
    /** \return Tuple of pointers to underlying object values
     */
    pointer operator->()
    {
        return boost::hana::transform(
            _iterators,
            [](auto&& iter) { return &(*iter); });
    }

    /// Default-construct a zip iterator. Dereferencing it is undefined.
    ZipIterator() = default;

    /// Copy-construct a zip iterator.
    /** \param other The iterator to copy from
     */
    ZipIterator(const ZipIterator& other) = default;

    /// Move-construct a zip iterator.
    /** \param other The iterator to move from (will be left in undefined
     *               state)
     */
    ZipIterator(ZipIterator&& other) = default;

    /// Move-assign a zip iterator.
    /** \param other The iterator to move from (will be left in undefined
     *               state)
     *  \return Reference to this object after moving
     */
    ZipIterator& operator=(ZipIterator&& other) = default;

    /// Copy-assign a zip iterator.
    /** \param other The iterator to copy from
     *  \return Reference to this object after copying
     */
    ZipIterator& operator=(const ZipIterator& other)= default;

    /// Construct from a pack of iterators
    /** \param iters The iterators to bundle in this ZipIterator
     */
    ZipIterator(Iters... iters)
        : _iterators(std::make_tuple(iters...))
    {
    }

    /// Construct from a tuple of iterators
    /** \param iters The iterators to bundle in this ZipIterator
     */
    template <typename... Iterators>
    ZipIterator(std::tuple<Iterators...> iters) : _iterators(iters)
    {
    }

    /// Compare this object with another zip iterator
    /** This compares the internal iterators of the ZipIterator objects
     *  \param other The iterator to compare to
     *  \return True if all internal iterators are equal
     */
    template<typename... It>
    bool operator==(const ZipIterator<It...>& other) const
    {
        return _iterators == other._iterators;
    }

    /// Compare this object with another zip iterator
    /** This compares the internal iterators of the ZipIterator objects
     *  \param other The iterator to compare to
     *  \return True if all internal iterators are un-equal
     */
    template<typename... It>
    bool operator!=(const ZipIterator<It...>& other) const
    {
        return _iterators != other._iterators;
    }

    /// Less-than compare this object with another zip iterator
    /** This compares the internal iterators of the ZipIterator objects
     *  \param other The iterator to compare to
     *  \return True if all internal iterators are less-than compared
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    bool operator<(const ZipIterator& other) const
    {
        return _iterators < other._iterators;
    }

    /// Less-than or equal compare this object with another zip iterator
    /** This compares the internal iterators of the ZipIterator objects
     *  \param other The iterator to compare to
     *  \return True if this object is not greater than the other
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    bool operator<=(const ZipIterator& other) const
    {
        return _iterators <= other._iterators;
    }

    /// Greater-than compare this object with another zip iterator
    /** This compares the internal iterators of the ZipIterator objects
     *  \param other The iterator to compare to
     *  \return True if all internal iterators are greater-than compared
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    bool operator>(const ZipIterator& other) const
    {
        return _iterators > other._iterators;
    }

    /// Greater-than or equal compare this object with another zip iterator
    /** This compares the internal iterators of the ZipIterator objects
     *  \param other The iterator to compare to
     *  \return True if this object is not less than the other
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    bool operator>=(const ZipIterator& other) const
    {
        return _iterators >= other._iterators;
    }

    /**
     * @brief  offset dereference operator for purely random accesss iterators held by ZipIterator
     *
     * @param n increment
     * @return tuple containing references to values pointed to by zipped iterators
     */
    /// Random access this object
    /** Create a copy of this object, increment it by the specified steps
     *  and return a reference to it. This does not modify the original
     *  iterator!
     *
     *  \param n The steps to increment
     *  \return Dereferenced temporary which has been incremented by `n` steps
     */
    template<typename category = iterator_category,
             typename std::enable_if_t<
                std::is_base_of_v<std::random_access_iterator_tag,
                                  category>, int> = 0>
    reference operator[](const difference_type n) const
    {
        return *(*this + n);
    }

    /// Destroy this object
    ~ZipIterator() = default;

    /// Write the contents of this iterator into an outstream
    /** Use reporter for tuples, see std::operator<<().
     */
    friend std::ostream& operator<< (std::ostream& ostr,
                                     const ZipIterator& right
    )
    {
        // using  Utopia::Utils::operator<<;
        ostr << "->" << *right;
        return ostr;
    }
};

/// Deduce the iterator types from the types inside the tuple
template <typename... Iterators>
ZipIterator(std::tuple<Iterators...> iters)->ZipIterator<Iterators...>;

// iterator adaptors

/// Return a zip iterator built from an adapter applied to containers
/** Use this function to apply STL iterator adaptors to a pack
 *  of containers and build a ZipIterator from the resulting pack
 *  of iterators.
 *
 *  \param adaptor The callable iterator adaptor to apply to all containers
 *  \param container The containers to build the ZipIterator from
 *  \return ZipIterator created from iterators built by the adaptor
 *  \ingroup Iter
 *
 *  \todo Make this work with template template instead of callable
 */
template <typename Adaptor, typename... Containers>
auto adapt_zip(Adaptor&& adaptor, Containers&... containers)
{
    return ZipIterator(adaptor(containers)...);
}

/// A range defined by instances of ZipIterator
/** The range itself only references the containers it is constructed from.
 *  The containers must exist throughout the life time of this range.
 *
 *  \tparam Containers Types of containers iterated over by ZipIterator
 *  \ingroup Iter
 */
template <typename... Containers>
class zip
{
protected:
    using Tuple = std::tuple<std::reference_wrapper<Containers>...>;
    Tuple _containers;

public:
    /**
     * @brief make Zipiterator containing the begin iterators of the
     *        containers 'zip' refers to
     *
     * @return auto
     */
    auto begin()
    {
        return ZipIterator(boost::hana::transform(
            _containers, [](auto& cont) { return cont.get().begin(); }));
    }

    /**
     * @brief make Zipiterator containing the end iterators of the
     *        containers 'zip' refers to
     *
     * @return auto
     */
    auto end()
    {
        return ZipIterator(boost::hana::transform(
            _containers, [](auto& cont) { return cont.get().end(); }));
    }

    /**
     * @brief make Zipiterator containing the const begin iterators of the
     *        containers 'zip' refers to
     *
     * @return auto
     */
    auto cbegin()
    {
        return ZipIterator(boost::hana::transform(
            _containers, [](auto& cont) { return cont.get().cbegin(); }));
    }

    /**
     * @brief make Zipiterator containing the const end iterators of the
     *        containers 'zip' refers to
     *
     * @return auto
     */
    auto cend()
    {
        return ZipIterator(boost::hana::transform(
            _containers, [](auto& cont) { return cont.get().cend(); }));
    }

    /**
     * @brief make Zipiterator containing the reverse begin iterators of the
     *        containers 'zip' refers to
     *
     * @return auto
     */
    auto rbegin()
    {
        return ZipIterator(boost::hana::transform(
            _containers, [](auto& cont) { return cont.get().rbegin(); }));
    }

    /**
     * @brief make Zipiterator containing the reverse end iterators of the
     *        containers 'zip' refers to
     *
     * @return auto
     */
    auto rend()
    {
        return ZipIterator(boost::hana::transform(
            _containers, [](auto& cont) { return cont.get().rend(); }));
    }

    /**
     * @brief Construct a new zip object
     *
     * @param other
     */
    zip(const zip& other) = delete;

    /**
     * @brief deleted copy assignment
     *
     * @param other
     * @return zip&
     */
    zip& operator=(const zip& other) = delete;

    /**
     * @brief assign zip object
     *
     * @param other
     * @return zip&
     */
    zip& operator=(zip&& other) = default;

    /**
     * @brief Construct a new zip object
     *
     * @param other
     */
    zip(zip&& other) = default;

    /// Destroy the range
    ~zip() = default;

    /// Instantiate a new zip range
    /** \param conts The pack of containers to create the range from
     */
    zip(Containers&... conts) : _containers(conts...)
    {
    }
};

/**
 * @brief Begin function like std::begin
 *
 * @tparam Containers
 * @param zipper
 * @return auto
 */
template <typename... Containers>
auto begin(zip<Containers...>& zipper)
{
    return zipper.begin();
}

/**
 * @brief end function like std::end
 *
 * @tparam Containers
 * @param zipper
 * @return auto
 */
template <typename... Containers>
auto end(zip<Containers...>& zipper)
{
    return zipper.end();
}

/**
 * @brief Begin function like std::begin
 *
 * @tparam Containers
 * @param zipper
 * @return auto
 */
template <typename... Containers>
auto cbegin(zip<Containers...>& zipper)
{
    return zipper.cbegin();
}

/**
 * @brief end function like std::end
 *
 * @tparam Containers
 * @param zipper
 * @return auto
 */
template <typename... Containers>
auto cend(zip<Containers...>& zipper)
{
    return zipper.cend();
}

/**
 * @brief Begin function like std::begin
 *
 * @tparam Containers
 * @param zipper
 * @return auto
 */
template <typename... Containers>
auto rbegin(zip<Containers...>& zipper)
{
    return zipper.rbegin();
}

/**
 * @brief end function like std::end
 *
 * @tparam Containers
 * @param zipper
 * @return auto
 */
template <typename... Containers>
auto rend(zip<Containers...>& zipper)
{
    return zipper.rend();
}

/**
 *  \} // group Iter
 */

} // namespace Itertools
} // namespace Utopia

#endif // UTOPIA_CORE_ZIP_HH
