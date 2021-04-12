#ifndef UTOPIA_DATAIO_GRAPH_LOAD_HH
#define UTOPIA_DATAIO_GRAPH_LOAD_HH

#include <string>
#include <fstream>
#include <boost/graph/graphml.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

#include "utopia/core/types.hh"
#include "utopia/data_io/filesystem.hh"
#include "utopia/data_io/cfg_utils.hh"


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
 * /param file_name     The name of the file to load
 * /param pmaps         Any additional property maps; if this contains
 *                      a property map named `weight`, the weights will
 *                      be loaded additionally, if the data file contains
 *                      that information.
 *
 * /return Graph        The loaded graph
 */


template <typename Graph>
Graph load_graph(const Config& cfg,
                 boost::dynamic_properties pmaps)
{
    const auto abs_file_path = get_abs_filepath(cfg);
    const auto format = get_as<std::string>("format", cfg, "dot");

    // Create an empty graph
    Graph g;

    // Load file into file stream
    std::ifstream ifs(abs_file_path.c_str());
    if (not ifs.is_open()) {
        throw std::invalid_argument(
            "Failed opening file for loading graph! Make sure there "
            "exists a file at " + abs_file_path + "!"
        );
    }

    // Load the data from the file stream
    if (format == "graphviz" or format == "gv" or format == "dot") {
        boost::read_graphviz(ifs, g, pmaps);

    } else if (format == "graphml") {
        boost::read_graphml(ifs, g, pmaps);

    } else {
        throw std::invalid_argument(
            "The given file format is not supported. The file format needs "
            "to be one of 'graphviz' / 'gv' / 'dot' or 'graphml' "
            "and needs to be specified in the config's format node, e.g. "
            "load_from_file: { format: graphml }.");
    }

    // Return the graph
    return g;
}

} // namespace GraphLoad

/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_GRAPH_LOAD_HH
