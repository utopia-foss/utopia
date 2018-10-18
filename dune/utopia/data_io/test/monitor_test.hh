#ifndef UTOPIA_TEST_MONITOR_TEST_HH
#define UTOPIA_TEST_MONITOR_TEST_HH

#include <string>

namespace Utopia {
namespace DataIO {

/** @brief Saves the buffer of an std::cout stream
 * 
 * From https://stackoverflow.com/a/32079599
 */
class Savebuf: public std::streambuf {
    
    /// The streambuffer
    std::streambuf* sbuf;

    /// The saved stream buffer
    std::string save;

    int overflow(int c) {
         if (!traits_type::eq_int_type(c, traits_type::eof())) {
             save.push_back(traits_type::to_char_type(c));
             return sbuf->sputc(c);
         }
         else {
             return traits_type::not_eof(c);
         }
    }

    int sync() { return sbuf->pubsync(); }
public:

    /**
     * @brief Construct a new savebuf object
     * 
     * @param sbuf The stream buffer
     */
    Savebuf(std::streambuf* sbuf): sbuf(sbuf) {}

    /**
     * @brief Get the saved stream buffer as a std::string
     * 
     * @return std::string 
     */
    std::string str() const { return save; }
};

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_TEST_MONITOR_TEST_HH
