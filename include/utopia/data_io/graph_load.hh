#ifndef UTOPIA_DATAIO_GRAPH_LOAD_HH
#define UTOPIA_DATAIO_GRAPH_LOAD_HH

#include <boost/graph/adjacency_list.hpp>
#include <string>
#include <fstream>
#include <boost/graph/graphml.hpp>
#include <boost/graph/graphviz.hpp>


namespace Utopia {
namespace DataIO {
/*!
 * \addtogroup DataIO
 * \{
 */

namespace GraphLoad {
/// Load a graph
/** \details This function loads a graph
 *
 * /tparam Graph        The graph type
 *
 * /param file_name     The name of the .graphml file
 *
 * /return Graph        The loaded graph
 */
template <typename Graph>
Graph load_graph(const std::string& abs_file_path, const std::string& format)
{

    // Create an empty graph
    Graph g;

    boost::dynamic_properties dyn_prop(boost::ignore_other_properties);

    // Load file into file stream
    std::ifstream ifs(abs_file_path.c_str());
    if (not ifs.is_open()) {
        throw std::invalid_argument(
            "Failed opening file for loading graph! Make sure there "
            "exists a file at " + abs_file_path + "!"
        );
    }

    // Load the data from the file stream
    if (format == "graphviz" or format == "dot") {
        boost::read_graphviz(ifs, g, dyn_prop);

    } else if (format == "graphml") {
        boost::read_graphml(ifs, g, dyn_prop);

    } else {
        throw std::invalid_argument(
            "The given file format is not supported. The file format needs "
            "to be one of 'graphviz', 'gv', 'dot', 'DOT', 'graphml' or 'gml', "
            "and needs to be specified in the config's format node, e.g. "
            "load_from_file: { format: gml }.");
    }

    // Return the graph
    return g;
}

} // namespace GraphLoad

/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_GRAPH_LOAD_HH
