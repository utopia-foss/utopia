#ifndef HDFBUFFERFACTORY_HH
#define HDFBUFFERFACTORY_HH
#include "hdftypefactory.hh"
#include "hdfutilities.hh"
#include <hdf5.h>
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
    template <
        typename T,
        std::enable_if_t<std::is_same<T, std::string>::value == false, int> = 0>
    static auto convert_source(T &source) {
        hvl_t value;
        value.len = source.size();
        value.p = source.data();
        return value;
    }

    template <
        typename T,
        std::enable_if_t<std::is_same<T, std::string>::value == true, int> = 0>
    static auto convert_source(T &source) {
        return source.c_str();
    }

    /**
     * @brief static function for turning an iterator range with arbitrarty
     *        datatypes into a vector of data as returned from 'adaptor'.
     *        Version for non-container return types of 'adaptor'
     *
     *
     * @tparam Iter Iterator
     * @tparam Adaptor function<some_type(typename Iterator::value_type)>
     * @param begin start of raw data range
     * @param end end of raw data range
     * @param adaptor adaptor function-pointer/functor/lamdba...
     * @return auto
     */
    template <typename T, typename Iter, typename Adaptor,
              std::enable_if_t<is_container_type<T>::value == false, int> = 0>
    static auto buffer(Iter begin, Iter end, Adaptor &&adaptor) {
        // set up buffer
        std::vector<std::decay_t<decltype(adaptor(*begin))>> data_buffer(
            std::distance(begin, end));

        // make buffer
        auto buffer_begin = data_buffer.begin();
        for (; begin != end; ++begin, ++buffer_begin) {
            *buffer_begin = adaptor(*begin);
        }
        return data_buffer;
    }

    /**
     * @brief static function for turning an iterator range with arbitrarty
     *        datatypes into a vector of data as returned from 'adaptor'.
     *        Version for container return types of 'adaptor'. adaptor
     * should return a reference to the containers, this is faster and
     *        secure.
     *
     * @tparam Iter Iterator
     * @tparam Adaptor function<some_type(typename Iterator::value_type)>
     * @param begin start of raw data range
     * @param end end of raw data range
     * @param adaptor adaptor function-pointer/functor/lamdba...
     * @return auto
     */

    template <typename T, typename Iter, typename Adaptor,
              std::enable_if_t<is_container_type<T>::value == true &&
                                   std::is_same<T, std::string>::value == false,
                               int> = 0>
    static auto buffer(Iter begin, Iter end, Adaptor &&adaptor) {
        // set up buffer

        std::vector<hvl_t> data_buffer(std::distance(begin, end));

        // using result_value_type = typename result_container_type::value_type;
        // if (std::is_same<result_container_type,
        //                  std::vector<result_value_type>>::value) {
        auto buffer_begin = data_buffer.begin();
        for (; begin != end; ++begin, ++buffer_begin) {
            *buffer_begin = convert_source(adaptor(*begin));
        }
        // }

        return data_buffer;
    }

    // overload for fucking strings
    template <typename T, typename Iter, typename Adaptor,
              std::enable_if_t<is_container_type<T>::value == true &&
                                   std::is_same<T, std::string>::value == true,
                               int> = 0>
    static auto buffer(Iter begin, Iter end, Adaptor &&adaptor) {
        // set up buffer

        std::vector<const char *> data_buffer(std::distance(begin, end));

        // using result_value_type = typename result_container_type::value_type;
        // if (std::is_same<result_container_type,
        //                  std::vector<result_value_type>>::value) {
        auto buffer_begin = data_buffer.begin();
        for (auto it = begin; it != end; ++it, ++buffer_begin) {
            *buffer_begin = convert_source(adaptor(*it));
        }
        // }

        return data_buffer;
    }

    // // get a buffer for reading
    // template <typename T> auto read_buffer(hsize_t size) {
    //     return std::vector<T>(size);
    // }
};

} // namespace DataIO

} // namespace Utopia
#endif