/** @file generators.hpp
 *  @brief In this file xorshift-family -Random number generators are
 * implemented (Wikipedia provides a good intro).
 *  @author Harald Mack, harald.mack@iup.uni-heidelberg.de
 *  @bug No known bugs.
 * @todo learn more about bitshifts and xor and stuff
 */

#ifndef UTOPIA_MODELS_AMEEMULTI_GENERATORS_HH
#define UTOPIA_MODELS_AMEEMULTI_GENERATORS_HH
#include <array>
#include <random>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/**
 * @brief Base class for xorshift style pseudo random number generators.
 *
 *
 * @tparam Statetype Type fo the internal state of the generator
 * @tparam Resulttype Type fo the output of the generator
 * @tparam a  Parameters for use in the generator, only mess with when you know
 * what your doing
 * @tparam b  Parameters for use in the generator, only mess with when you know
 * what your doing
 * @tparam c  Parameters for use in the generator, only mess with when you know
 * what your doing
 */
template <typename Statetype, typename Resulttype, unsigned int a, unsigned int b, unsigned int c>
class XorShiftBase
{
protected:
    Statetype _state;

public:
    /**
     * @brief type alias to enable usage in std random distributions
     *
     */
    using result_type = Resulttype;
    /**
     * @brief type alias to enable usage in std random distributions
     *
     */
    using state_type = Statetype;

    /**
     * @brief get the maximum value able to be stored in the result type
     *
     * @return constexpr result_type
     */
    constexpr static result_type max()
    {
        return std::numeric_limits<result_type>::max();
    }

    /**
     * @brief abstract prototype for actual random number generation operator
     *
     * @return result_type
     */
    virtual inline result_type operator()() = 0;

    /**
     * @brief Get the state object
     *
     * @return state_type
     */
    state_type get_state()
    {
        return _state;
    }

    /**
     * @brief Set the state object
     *
     */
    void set_state(state_type state)
    {
        _state = state;
    }

    /**
     * @brief get the minimum number able to be stored in the result type
     *
     * @return constexpr result_type
     */
    constexpr static result_type min()
    {
        return std::numeric_limits<result_type>::min();
    }

    /**
     * @brief Construct a new Xor Shift Base object
     *
     */
    XorShiftBase() = default;

    /**
     * @brief Destroy the Xor Shift Base object
     *
     */
    virtual ~XorShiftBase() = default;

    /**
     * @brief Construct a new Xor Shift Base object
     *
     * @param other
     */
    XorShiftBase(const XorShiftBase& other) = default;

    /**
     * @brief Construct a new Xor Shift Base object
     *
     * @param other
     */
    XorShiftBase(XorShiftBase&& other) = default;

    /**
     * @brief Move assign object
     *
     * @return XorShiftBase&
     */
    XorShiftBase& operator=(XorShiftBase&&) = default;

    /**
     * @brief copy assign object
     *
     * @return XorShiftBase&
     */
    XorShiftBase& operator=(const XorShiftBase&) = default;

    /**
     * @brief Construct a new Xor Shift Base object
     *
     * @param state
     */
    XorShiftBase(state_type state) : _state(std::forward<state_type>(state))
    {
    }

    /**
     * @brief Construct a new Xor Shift Base object, using a single number
     *        which is transformed into an appropriate seed of uniformly
     *        distributed numbers for the rng in question. This is done
     *        using std::seed_seq
     *
     * @param singlestate - a single unsigned int number
     */
    XorShiftBase(typename Statetype::value_type single_state)
        : _state([&]() {
              std::seed_seq seq{single_state};
              Statetype state;
              seq.generate(state.begin(), state.end());
              return state;
          }())
    {
    }
};

/**
 * @brief      XorShift-random-number-generator
 * (https://de.wikipedia.org/wiki/Xorshift)
 * Be careful here, this generator is not recommended for production level code.
 * Rather use Xoroshiro, XorshiftPlus or, if speed is not your greatest concern,
 * XorShiftStar
 */
template <typename Statetype = std::array<uint64_t, 1>, typename Resulttype = uint64_t, unsigned int a = 13, unsigned int b = 7, unsigned int c = 17>
class XorShift : public XorShiftBase<Statetype, Resulttype, a, b, c>
{
private:
    using Base = XorShiftBase<Statetype, Resulttype, a, b, c>;

public:
    using result_type = Resulttype;
    using state_type = Statetype;

    /**
     * @brief Actual random number generating function
     *
     * @return result_type
     */
    inline result_type operator()() override
    {
        this->_state[0] ^= this->_state[0] << a;
        this->_state[0] ^= this->_state[0] >> b;
        this->_state[0] ^= this->_state[0] << c;
        return this->_state[0];
    }

    /**
     * @brief Exchanges states of with another object of equal type
     *
     * @param other
     */
    void swap(XorShift& other)
    {
        if (this == &other)
        {
            return;
        }
        using std::swap;
        swap(this->_state, other._state);
    }
    /**
     * @brief Construct a new Xor Shift object
     *
     */
    XorShift() = default;
    /**
     * @brief Destroy the Xor Shift object
     *
     */
    virtual ~XorShift() = default;
    /**
     * @brief Construct a new Xor Shift object
     *
     * @param other
     */
    XorShift(const XorShift& other) = default;
    /**
     * @brief Construct a new Xor Shift object
     *
     * @param other
     */
    XorShift(XorShift&& other) : XorShift()
    {
        this->swap(other);
    }
    /**
     * @brief
     *
     * @param other
     * @return XorShift&
     */
    XorShift& operator=(XorShift other)
    {
        this->swap(other);
        return *this;
    }
    /**
     * @brief Construct a new Xor Shift object
     *
     * @param state
     */
    XorShift(state_type state) : Base(std::forward<Statetype>(state)){};

    /**
     * @brief Construct a new Xor Shift object
     *
     * @param singlestate - a single unsigned int number
     */
    XorShift(typename Statetype::value_type single_state) : Base(single_state)
    {
    }
};

/**
 * @brief Exchanges states of two objects
 *
 * @tparam Statetype
 * @tparam Resulttype
 * @tparam a
 * @tparam b
 * @tparam c
 * @param lhs
 * @param rhs
 */
template <typename Statetype, typename Resulttype, unsigned int a, unsigned int b, unsigned int c>
void swap(XorShift<Statetype, Resulttype, a, b, c>& lhs,
          XorShift<Statetype, Resulttype, a, b, c>& rhs)
{
    lhs.swap(rhs);
}

/**
 * @brief xorshift* - random-number-generator
 (https://en.wikipedia.org/wiki/Xorshift): very fast, long period, good
 statistics (Claimed by wikipedia and blindly followed here), not tested by
 myself other than making a bitmap of 1000000 numbers and searching it for
 patterns (non found). Requires good seed, however, therefore random device used
 for seeding below.
 */
template <typename Statetype = std::array<uint64_t, 16>, typename Resulttype = uint64_t, unsigned int a = 31, unsigned int b = 11, unsigned int c = 30>
class XorShiftStar : public XorShiftBase<Statetype, Resulttype, a, b, c>
{
protected:
    /**
     * number selecting one number from the state vector for use in
     * random-number-generation, must be in [0,15]
     */
    int _p;
    using Base = XorShiftBase<Statetype, Resulttype, a, b, c>;

public:
    using result_type = Resulttype;
    using state_type = Statetype;

    /**
     * @brief Exchanges state with another object
     *
     * @param other
     */
    void swap(XorShiftStar& other)
    {
        if (this == &other)
        {
            return;
        }
        using std::swap;
        swap(this->_state, other._state);
        swap(_p, other._p);
    }

    /**
     * @brief Get the p object
     *
     * @return int
     */
    int get_p()
    {
        return _p;
    }

    /**
     * @brief Set the p object
     *
     * @param p
     */
    void set_p(int p)
    {
        _p = p;
    }

    /**
     * @brief Actually generates the random numbers
     *
     * @return result_type
     */
    inline result_type operator()() override
    {
        const typename Statetype::value_type s0 = this->_state[_p];
        typename Statetype::value_type s1 = this->_state[_p = (_p + 1) & 15];
        s1 ^= s1 << a;                                      // a
        this->_state[_p] = s1 ^ s0 ^ (s1 >> b) ^ (s0 >> c); // b, c
        return this->_state[_p] * UINT64_C(1181783497276652981); // where does this number come from?
    }

    /**
     * @brief Construct a new Xor Shift Star object
     *
     */
    XorShiftStar() = default;

    /**
     * @brief Destroy the Xor Shift Star object
     *
     */
    virtual ~XorShiftStar() = default;

    /**
     * @brief Construct a new Xor Shift Star object
     *
     * @param other
     */
    XorShiftStar(const XorShiftStar& other) = default;

    /**
     * @brief Construct a new Xor Shift Star object
     *
     * @param other
     */
    XorShiftStar(XorShiftStar&& other) : XorShiftStar()
    {
        this->swap(other);
    }

    /**
     * @brief Assignment operator
     *
     * @param other
     * @return XorShiftStar&
     */
    XorShiftStar& operator=(XorShiftStar other)
    {
        this->swap(other);
        return *this;
    }

    /**
     * @brief Construct a new Xor Shift Star object
     *
     * @param state
     * @param p
     */
    XorShiftStar(Statetype state, int p)
        : Base(std::forward<state_type>(state)), _p(p)
    {
    }

    /**
     * @brief Construct a new Xor Shift Star object
     *
     * @param state
     */
    XorShiftStar(Statetype state)
        : Base(std::forward<state_type>(state)),
          _p(7) // set p to seven, seems as good as anything
    {
    }

    /**
     * @brief Construct a new Xor Shift Star object
     *
     * @param singlestate - a single unsigned int number
     */
    XorShiftStar(typename Statetype::value_type single_state)
        : Base(single_state), _p([&]() {
              std::seed_seq seq{single_state};
              std::array<decltype(_p), 1> p;
              seq.generate(p.begin(), p.end());
              return p[0];
          }())
    {
    }
};

/**
 * @brief Exchanges states of two objects
 *
 * @tparam Statetype
 * @tparam Resulttype
 * @tparam a
 * @tparam b
 * @tparam c
 * @param lhs
 * @param rhs
 */
template <typename Statetype, typename Resulttype, unsigned int a, unsigned int b, unsigned int c>
void swap(XorShiftStar<Statetype, Resulttype, a, b, c>& lhs,
          XorShiftStar<Statetype, Resulttype, a, b, c>& rhs)
{
    lhs.swap(rhs);
}

/**
 * @brief      XorshiftPlus128 random number generator, faster than
 * XorShiftStar, but with shorter period length (2^{128}-1 vs 2^{1024}).
 */
template <typename Statetype = std::array<uint64_t, 3>, typename Resulttype = uint64_t, unsigned int a = 23, unsigned int b = 17, unsigned int c = 26>
class XorShiftPlus : public XorShiftBase<Statetype, Resulttype, a, b, c>
{
protected:
    using Base = XorShiftBase<Statetype, Resulttype, a, b, c>;

public:
    using result_type = Resulttype;
    using state_type = Statetype;

    /**
     * @brief Exchanges state with another object
     *
     * @param other
     */
    void swap(XorShiftPlus& other)
    {
        using std::swap;
        if (this == &other)
        {
            return;
        }
        swap(this->_state, other._state);
    }

    /**
     * @brief actually generates the random numbers
     *
     * @return result_type
     */
    inline result_type operator()() override
    {
        uint64_t x = this->_state[0];
        uint64_t const y = this->_state[1];
        this->_state[0] = y;
        x ^= x << a;                                   // a
        this->_state[1] = x ^ y ^ (x >> b) ^ (y >> c); // b, c
        return this->_state[1] + y;
    }

    /**
     * @brief Construct a new Xor Shift Plus object
     *
     */
    XorShiftPlus() = default;

    /**
     * @brief Destroy the Xor Shift Plus object
     *
     */
    virtual ~XorShiftPlus() = default;

    /**
     * @brief Construct a new Xor Shift Plus object
     *
     * @param other
     */
    XorShiftPlus(const XorShiftPlus& other) = default;

    /**
     * @brief Construct a new Xor Shift Plus object
     *
     * @param other
     */
    XorShiftPlus(XorShiftPlus&& other) : XorShiftPlus()
    {
        this->swap(other);
    }

    /**
     * @brief
     *
     * @param other
     * @return XorShiftPlus&
     */
    XorShiftPlus& operator=(XorShiftPlus other)
    {
        this->swap(other);
        return *this;
    }
    /**
     * @brief Construct a new Xor Shift Plus object
     *
     * @param state
     */
    XorShiftPlus(Statetype state) : Base(std::forward<Statetype>(state))
    {
    }

    /**
     * @brief Construct a new Xor Shift Plus object
     *
     * @param singlestate - a single unsigned int number
     */
    XorShiftPlus(typename Statetype::value_type single_state)
        : Base(single_state)
    {
    }
};

/**
 * @brief Exchanges states of two objects
 *
 * @tparam Statetype
 * @tparam Resulttype
 * @tparam a
 * @tparam b
 * @tparam c
 * @param lhs
 * @param rhs
 */
template <typename Statetype, typename Resulttype, unsigned int a, unsigned int b, unsigned int c>
void swap(XorShiftPlus<Statetype, Resulttype, a, b, c>& lhs,
          XorShiftPlus<Statetype, Resulttype, a, b, c>& rhs)
{
    lhs.swap(rhs);
}

/**
 * @brief This is an implementation of the Xoroshiro generator for
 *
 * @tparam uint64_t
 * @tparam uint32_t
 * @tparam 55
 * @tparam 14
 * @tparam 36
 */
template <typename Statetype = std::array<uint64_t, 2>, typename Resulttype = uint32_t, unsigned int a = 55, unsigned int b = 14, unsigned int c = 36>
class Xoroshiro : public XorShiftBase<Statetype, Resulttype, a, b, c>
{
protected:
    using Base = XorShiftBase<Statetype, Resulttype, a, b, c>;

    // helper variable
    static constexpr unsigned int ITYPE_BITS = 8 * sizeof(typename Statetype::value_type);

    // helper variable
    static constexpr unsigned int RTYPE_BITS = 8 * sizeof(Resulttype);

    // helper for 'rotating the state
    static inline typename Statetype::value_type rotl(const typename Statetype::value_type x, int k)
    {
        return (x << k) | (x >> (ITYPE_BITS - k));
    }

public:
    using state_type = Statetype;
    using result_type = Resulttype;

    /**
     * @brief Exchanges state with anothr object of equal type
     *
     * @param other
     */
    void swap(Xoroshiro& other)
    {
        using std::swap;
        swap(this->_state, other._state);
    }

    /**
     * @brief generates the actual random numbers
     *
     * @return result_type
     */
    inline result_type operator()() override
    {
        auto res = this->_state[0] + this->_state[1];
        this->_state[1] ^= this->_state[0];
        this->_state[0] =
            rotl(this->_state[0], a) ^ this->_state[1] ^ (this->_state[1] << b);
        this->_state[1] = rotl(this->_state[1], c);
        return (res >> (ITYPE_BITS - RTYPE_BITS));
    }

    /**
     * @brief Construct a new Xoroshiro object
     *
     * @param other
     */
    Xoroshiro(const Xoroshiro& other) = default;

    /**
     * @brief Construct a new Xoroshiro object
     *
     * @param other
     */
    Xoroshiro(Xoroshiro&& other) : Xoroshiro()
    {
        this->swap(other);
    }

    /**
     * @brief
     *
     * @param other
     * @return Xoroshiro&
     */
    Xoroshiro& operator=(Xoroshiro other)
    {
        this->swap(other);
        return *this;
    }

    /**
     * @brief Construct a new Xoroshiro object
     *
     * @param s
     */
    Xoroshiro(Statetype s)
        : Base(std::forward<Statetype>({{s[0], (s[0] || s[1] ? s[1] : 1)}}))
    {
    }

    /**
     * @brief Construct a new Xoroshiro object
     *
     * @param singlestate - a single unsigned int number
     */
    Xoroshiro(typename Statetype::value_type singlestate) : Base(singlestate)
    {
    }

    /**
     * @brief Construct a new Xoroshiro object
     *
     */
    Xoroshiro() = default;

    /**
     * @brief Destroy the Xoroshiro object
     *
     */
    virtual ~Xoroshiro() = default;
};

/**
 * @brief Exchanges states of two objects
 *
 * @tparam Statetype
 * @tparam Resulttype
 * @tparam a
 * @tparam b
 * @tparam c
 * @param lhs
 * @param rhs
 */
template <typename Statetype, typename Resulttype, unsigned int a, unsigned int b, unsigned int c>
void swap(Xoroshiro<Statetype, Resulttype, a, b, c>& lhs,
          Xoroshiro<Statetype, Resulttype, a, b, c>& rhs)
{
    lhs.swap(rhs);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif