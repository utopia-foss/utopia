#ifndef UTOPIA_DATAIO_GRAPH_LOAD_HH
#define UTOPIA_DATAIO_GRAPH_LOAD_HH

#include <boost/graph/adjacency_list.hpp>
#include <string>
#include <fstream>
#include <boost/graph/graphml.hpp>
// #include <boost/graph/graphviz.hpp>


namespace Utopia {
namespace DataIO {
/*!
 * \addtogroup DataIO
 * \{
 */

namespace GraphLoad{
/// Load a graph
/** \details This function loads a graph
 *  \warning Not tested yet
 *
 * /tparam Graph        The graph type
 *
 * /param file_name     The name of the .graphml file
 *
 * /return Graph        The loaded graph
 */
template <typename Graph>
Graph load_graphml(std::string file_name)
{

    // Create an empty graph
    Graph g;

    boost::dynamic_properties dyn_prop(boost::ignore_other_properties);

    std::ifstream ifs(file_name.c_str());
    if (!ifs.is_open())
    {
        std::cout << "loading file failed." << std::endl;
        throw "Could not load file.";
    }

    // Load the data from the file stream
    boost::read_graphml(ifs, g, dyn_prop);

    // Return the graph
    return g;
}


} // namespace GraphLoad

/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_GRAPH_LOAD_HH
