#ifndef HDFBUFFERFACTORY_HH
#define HDFBUFFERFACTORY_HH
#include "hdftypefactory.hh"
#include "hdfutilities.hh"
#include <hdf5.h>
namespace Utopia
{
namespace DataIO
{
/**
 * @brief Class which turns non-vector or plain-array containers into
 *        vectors. If the value_types are containers themselves, these are
 *        turned into vectors as well, because HDF5 cannot write something else.
 *
 */
class HDFBufferFactory
{
private:
protected:
public:
    /**
     * @brief function for converting source data into varilble length type
     *
     * @tparam T
     * @tparam 0
     * @param source
     * @return auto
     */
    template <typename T>
    static auto convert_source(T& source)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return source.c_str();
        }
        else
        {
            hvl_t value;
            value.len = source.size();
            value.p = source.data();
            return value;
        }
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
    template <typename Iter, typename Adaptor>
    static auto buffer(Iter begin, Iter end, Adaptor&& adaptor)
    {
        using T = typename HDFTypeFactory::result_type<decltype(adaptor(*begin))>::type;

        if constexpr (is_container_type<T>::value)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                // set up buffer

                std::vector<const char*> data_buffer(std::distance(begin, end));

                // using result_value_type = typename result_container_type::value_type;
                // if (std::is_same<result_container_type,
                //                  std::vector<result_value_type>>::value) {
                auto buffer_begin = data_buffer.begin();
                for (auto it = begin; it != end; ++it, ++buffer_begin)
                {
                    *buffer_begin = convert_source(adaptor(*it));
                }
                // }

                return data_buffer;
            }
            else
            {
                // set up buffer

                std::vector<hvl_t> data_buffer(std::distance(begin, end));

                // using result_value_type = typename result_container_type::value_type;
                // if (std::is_same<result_container_type,
                //                  std::vector<result_value_type>>::value) {
                auto buffer_begin = data_buffer.begin();
                for (; begin != end; ++begin, ++buffer_begin)
                {
                    *buffer_begin = convert_source(adaptor(*begin));
                }
                // }

                return data_buffer;
            }
        }
        else
        {
            // set up buffer
            std::vector<T> data_buffer(std::distance(begin, end));

            // make buffer
            auto buffer_begin = data_buffer.begin();
            for (; begin != end; ++begin, ++buffer_begin)
            {
                *buffer_begin = adaptor(*begin);
            }
            return data_buffer;
        }
    }
};

} // namespace DataIO

} // namespace Utopia
#endif