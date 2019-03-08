/**
 * @brief In this file, a class for automatically creating intermediate buffer
 *        data structures between the user provided data and the file are
 *        provided.
 *
 * @file hdfbufferfactory.hh
 */
#ifndef UTOPIA_DATAIO_HDFBUFFERFACTORY_HH
#define UTOPIA_DATAIO_HDFBUFFERFACTORY_HH

#include <vector>

#include <hdf5.h>

#include "hdftypefactory.hh"
#include "hdfutilities.hh"


namespace Utopia {
namespace DataIO {

/**
 * @brief      Class which turns non-vector or plain-array containers into
 *             vectors. If the value_types are containers themselves, these are
 *             turned into vectors as well, because HDF5 cannot write something
 *             else.
 */
class HDFBufferFactory
{
public:
    /**
     * @brief      function for converting source data into variable length
     *             type
     *
     * @param      source  The source
     *
     * @tparam     T       the source type
     *
     * @return     auto
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
            value.p = &(source[0]);
            return value;
        }
    }

    /**
     * @brief      static function for turning an iterator range with
     *             arbitrarty datatypes into a vector of data as returned from
     *             'adaptor'. Version for non-container return types of
     *             'adaptor'
     *
     * @param      begin      start of raw data range
     * @param      end        end of raw data range
     * @param      adaptor    adaptor function-pointer/functor/lamdba...
     *
     * @tparam     Iter       Iterator
     * @tparam     Adaptor    function<some_type(typename
     *                        Iterator::value_type)>
     *
     * @return     auto       The data range buffered from the adaptor
     */
    template <typename Iter, typename Adaptor>
    static auto buffer(Iter begin, Iter end, Adaptor&& adaptor)
    {
        using T = remove_qualifier_t<decltype(adaptor(*begin))>;
        if constexpr (is_container_v<T>)
        {
            // set up buffer
            if constexpr (is_array_like_v<T>)
            {
                std::vector<T> data_buffer(std::distance(begin, end));
                auto buffer_begin = data_buffer.begin();
                for (; begin != end; ++begin, ++buffer_begin)
                {
                    *buffer_begin = adaptor(*begin);
                }
                return data_buffer;
            }
            else
            {
                std::vector<hvl_t> data_buffer(std::distance(begin, end));

                auto buffer_begin = data_buffer.begin();
                for (; begin != end; ++begin, ++buffer_begin)
                {
                    *buffer_begin = convert_source(adaptor(*begin));
                }

                return data_buffer;
            }
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            // set up buffer

            std::vector<const char*> data_buffer(std::distance(begin, end));

            auto buffer_begin = data_buffer.begin();
            for (auto it = begin; it != end; ++it, ++buffer_begin)
            {
                *buffer_begin = convert_source(adaptor(*it));
            }

            return data_buffer;
        }

        else
        { // not a container
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
