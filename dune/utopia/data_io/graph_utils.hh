#ifndef DATAIO_GRAPH_UTILS_HH
#define DATAIO_GRAPH_UTILS_HH

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>

#include <hdf5.h>

#include <dune/utopia/core/logging.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {
namespace DataIO {

/// Write function for a boost::Graph
/** This function writes a boost::graph into a HDFGroup. It assumes that the
 *  vertices and edges of the graph already supply indices.
 * 
 * @tparam GraphType 
 *
 * @param g The graph to save
 * @param parent_grp The parent HDFGroup the graph should be stored in
 * @param name The name the newly created graph group should have
 *
 * @return std::shared_ptr<HDFGroup> The newly created graph group
 */
template<typename GraphType>
std::shared_ptr<HDFGroup> save_graph(GraphType &g,
                                     const std::shared_ptr<HDFGroup>& parent_grp,
                                     const std::string& name)
{
    // Collect some information on the graph
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Get a logger to use here (Note: needs to have been setup beforehand)
    auto log = spdlog::get("data_io");
    log->info("Saving graph '{}' ({} vertices, {} edges) ...",
              name, num_vertices, num_edges);

    // Create the group for the graph and store metadata in its attributes
    auto grp = parent_grp->open_group(name);

    grp->add_attribute("content", "network");
    grp->add_attribute("is_directed", boost::is_directed(g));
    grp->add_attribute("is_parallel", false); // FIXME Make general
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    grp->add_attribute("custom_ids", false);
    

    // Initialize datasets to store vertices and edges in
    auto dset_vl = grp->open_dataset("_vertices", {num_vertices});
    auto dset_al = grp->open_dataset("_edges", {2, num_edges});
    // NOTE Need shape to be {2, num_edges} because write writes line by line.

    // Save vertex list 
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(boost::vertex_index_t(), g, vd);}
    );

    // Save edges
    // NOTE Need to call write twice to achieve the desired data shape.
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            return boost::get(boost::vertex_index_t(), g,
                              boost::source(ed, g));
        }
    );

    dset_al->write(e, e_end,
        [&](auto ed){
            return boost::get(boost::vertex_index_t(), g,
                              boost::target(ed, g));
        }
    );

    log->debug("Graph saved.");

    // Return the newly created group
    return grp;
}


/// Write function for a boost::Graph
/** This function writes a boost::graph into a HDFGroup. By supplying custom
 *  vertex IDs, they need not be part of the graph in order for this function
 *  to operate.
 * 
 * @tparam GraphType 
 * @tparam PropertyMap The property map of the vertex ids
 *
 * @param g The graph to save
 * @param parent_grp The parent HDFGroup the graph should be stored in
 * @param name The name the newly created graph group should have
 * @param vertex_ids A custom property map of vertex IDs to use
 *
 * @return std::shared_ptr<HDFGroup> The newly created graph group
 */
template<typename GraphType, typename PropertyMap>
std::shared_ptr<HDFGroup> save_graph(GraphType &g,
                                     const std::shared_ptr<HDFGroup>& parent_grp,
                                     const std::string& name,
                                     const PropertyMap vertex_ids)
{
    // Collect some information on the graph
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Get a logger to use here (Note: needs to have been setup beforehand)
    auto log = spdlog::get("data_io");
    log->info("Saving graph '{}' ({} vertices, {} edges) ...",
              name, num_vertices, num_edges);

    // Create the group for the graph and store metadata in its attributes
    auto grp = parent_grp->open_group(name);

    grp->add_attribute("content", "network");
    grp->add_attribute("is_directed", boost::is_directed(g));
    grp->add_attribute("is_parallel", false); // FIXME Make general
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    grp->add_attribute("custom_ids", true);
    
    
    // Initialize datasets to store vertices and adjacency lists in
    auto dset_vl = grp->open_dataset("_vertices", {num_vertices});
    auto dset_al = grp->open_dataset("_edges", {2, num_edges});
    // NOTE Need shape to be {2, num_edges} because write writes line by line.

    // Save vertex list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(vertex_ids, vd);}
    );

    // Save edges
    // NOTE Need to call write twice to achieve the desired data shape.
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            return boost::get(vertex_ids, boost::source(ed, g));
        }
    );

    dset_al->write(e, e_end,
        [&](auto ed){
            return boost::get(vertex_ids, boost::target(ed, g));
        }
    );
    
    log->debug("Graph saved.");

    // Return the newly created group
    return grp;
}

// TODO add functions here to open datasets for edge or node attributes.

} // namespace DataIO
} // namespace Utopia

#endif // DATAIO_GRAPH_UTILS_HH
