#ifndef UTOPIA_CORE_STRING_HH
#define UTOPIA_CORE_STRING_HH

#include <string>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


namespace Utopia {

/// Joins together the strings in a container
/** Wraps boost::algorithm::join
  */
template<class Cont = std::vector<std::string>>
std::string join (const Cont& cont, const std::string& delim = ", ") {
    if (cont.empty()) {
        return "";
    }
    return boost::algorithm::join(cont, delim);
}

/// Splits a string and returns a container of string segments
/** Wraps boost::algorithm::split. This function aims to cover *typical* use
  * cases for string splitting, not the task of string splitting in general.
  *
  * \tparam SeqCont The container type to use; note that the element type of
  *                 that container always be std::string
  *
  * \param  s       The string to split according to `delims`
  * \param  delims  The delimiters (plural!) to split by. Will be passed to
  *                 boost::is_any_of.
  *
  * \note   The boost::is_any_of classifier being used here means that *any*
  *         character in `delims` is used as a delimiter, even if they are on
  *         their own.
  *         Additionally, the boost::algorithm::token_compress_on qualifier is
  *         used, meaning that adjacent tokens are compressed and do not lead
  *         to an empty segment entry. However, note that a delimiter like `->`
  *         will match `->` but also `>-` `->-` `->>-` and other combinations!
  */
template<class SeqCont = std::vector<std::string>>
SeqCont split (const std::string& s, const std::string& delims = " ") {
    auto segments = SeqCont{};

    if (s.empty()) {
        return segments;
    }

    boost::split(segments, s, boost::is_any_of(delims),
                 boost::algorithm::token_compress_on);
    return segments;
}


} // namespace Utopia

#endif // UTOPIA_CORE_STRING_HH
