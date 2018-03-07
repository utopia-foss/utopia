#ifndef HDFBUFFERFACTORY_HH
#define HDFBUFFERFACTORY_HH
#include "hdfutilities.hh"
namespace Utopia {
namespace DataIO {

/**
 * @brief Class which turns non-vector or plain-array containers into
 *        vectors. If the value_types are containers themselves, these are
 *        turned into vectors as well, because HDF5 cannot write something else.
 *
 */
class HDFBufferFactory {
private:
protected:
public:
    /**
     * @brief static function for turning an iterator range with arbitrarty
     *        datatypes into a vector of data as returned from 'adaptor'.
     *
     *
     * @tparam Iter Iterator
     * @tparam Adaptor function<some_type(typename Iterator::value_type)>
     * @param begin start of raw data range
     * @param end end of raw data range
     * @param adaptor adaptor function-pointer/functor/lamdba...
     * @return auto
     */
    template <typename Iter, typename Adaptor>
    static auto buffer(Iter begin, Iter end, Adaptor &&adaptor) {
        // set up buffer
        std::vector<std::decay_t<decltype(adaptor(*begin))>> buffer(
            std::distance(begin, end));

        // make buffer
        auto buffer_begin = buffer.begin();
        for (; begin != end; ++begin, ++buffer_begin) {
            *buffer_begin = adaptor(*begin);
        }
        return buffer;
    }
};
} // namespace DataIO
} // namespace Utopia
#endif