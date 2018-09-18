#ifndef DATAIO_GRAPH_UTILS_HH
#define DATAIO_GRAPH_UTILS_HH

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>

#include <dune/utopia/data_io/hdfgroup.hh>

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
    // TODO get the logger and add some messages?

    // Collect some information on the graph that is to be saved
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Create the group for the graph and store metadata in its attributes
    auto grp = parent_grp->open_group(name);

    grp->add_attribute("is_graph_group", true);
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    grp->add_attribute("custom_ids", false);
    
    // Initialize datasets to store vertices and adjacency lists in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});
    auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

    // Save vertex list 
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(boost::vertex_index_t(), g, vd);}
    );

    // Save adjacency list
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            // TODO If possible, use boost::index_type instead of size_t
            return std::array<std::size_t, 2>(
                {{boost::get(boost::vertex_index_t(), g,
                             boost::source(ed, g)),
                  boost::get(boost::vertex_index_t(), g,
                             boost::target(ed, g))}}
            );
        }
    );
    // FIXME edge index is missing!?!

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
    // TODO get the logger and add some messages?

    // Collect some information on the graph that is to be saved
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Create the group for the graph and store metadata in its attributes
    auto grp = parent_grp->open_group(name);

    grp->add_attribute("is_graph_group", true);
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    grp->add_attribute("custom_ids", true);
    
    // Initialize datasets to store vertices and adjacency lists in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});
    auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

    // Save vertex list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(vertex_ids, vd);}
    );

    // Save adjacency list
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            // TODO If possible, use boost::index_type instead of size_t
            return std::array<std::size_t, 2>(
                {{boost::get(vertex_ids, boost::source(ed, g)),
                  boost::get(vertex_ids, boost::target(ed, g))}}
            );
        }
    );
    // FIXME edge index is missing!?!
    
    // Return the newly created group
    return grp;
}

} // namespace DataIO
} // namespace Utopia

#endif // DATAIO_GRAPH_UTILS_HH
