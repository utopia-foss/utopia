#ifndef DATAIO_GRAPH_UTILS_HH
#define DATAIO_GRAPH_UTILS_HH

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>

#include <dune/utopia/data_io/hdfgroup.hh>

namespace Utopia {
namespace DataIO {

/// Write function for a static boost::Graph
/** This function writes a boost::graph into a HDFGroup.
 * 
 * @tparam save_edges=true Saves the edges if true
 * @tparam GraphType 
 *
 * @param g The graph to save
 * @param parent_grp The parent HDFGroup the graph should be stored in
 * @param name The name the newly created graph group should have
 *
 * @return std::shared_ptr<HDFGroup> The newly created graph group
 */
template<bool save_edges=true, typename GraphType>
std::shared_ptr<HDFGroup> save_graph(GraphType &g,
                                     const std::shared_ptr<HDFGroup>& parent_grp,
                                     const std::string& name)
{
    // Create the group for the graph
    auto grp = parent_grp->open_group(name);

    // Store in attributes that this is supposed to be a Graph
    grp->add_attribute("is_static_graph_group", true);

    // Collect some metadata
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Also store some metadata
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    // TODO _could_ add more attributes here, should be general though

    // Initialize dataset to store vertices list in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});

    // Save vertices list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(boost::vertex_index_t(), g, vd);}
    );

    // Save adjacency list
    if constexpr (save_edges){
        // Initialize dataset to store adjacency list in
        auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

        auto [e, e_end] = boost::edges(g);
        dset_al->write(e, e_end,
            [&](auto ed){
                // Extract indices of source and target vertex as well as of
                // the edge this corresponds to.
                // TODO If possible, use boost::index_type instead of size_t
                return std::array<std::size_t, 2>(
                    {{boost::get(boost::vertex_index_t(), g,
                                 boost::source(ed, g)),
                      boost::get(boost::vertex_index_t(), g,
                                 boost::target(ed, g))}}
                );
            }
        );
    }

    // Return newly created group
    return grp;
}


/// Write function for a static boost::Graph
/** This function writes a boost::graph into a HDFGroup.
 * 
 * @tparam save_edges=true Saves the edges if true 
 * @tparam PropertyMap The property map of the vertex ids
 * @tparam GraphType 
 *
 * @param g The graph to save
 * @param parent_grp The parent HDFGroup the graph should be stored in
 * @param name The name the newly created graph group should have
 * @param ids A custom list of IDs that corresponds to the vertices
 *
 * @return std::shared_ptr<HDFGroup> The newly created graph group
 */
template<bool save_edges=true, typename GraphType, typename PropertyMap>
std::shared_ptr<HDFGroup> save_graph(GraphType &g,
                                     const std::shared_ptr<HDFGroup>& parent_grp,
                                     const std::string& name,
                                     const PropertyMap ids)
{
    // Create the group for the graph
    auto grp = parent_grp->open_group(name);

    // Store in attributes that this is supposed to be a Graph
    grp->add_attribute("is_static_graph_group", true);

    // Collect some metadata
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Also store some metadata
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    // TODO _could_ add more attributes here, should be general though

    // Initialize dataset to store vertices list in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});

    // Save vertices list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(ids, vd);}
    );

    // Save adjacency list
    if constexpr (save_edges){
        // Initialize dataset to store adjacency list in
        auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

        auto [e, e_end] = boost::edges(g);
        dset_al->write(e, e_end,
            [&](auto ed){
                // Extract indices of source and target vertex as well as of
                // the edge this corresponds to.
                // TODO If possible, use boost::index_type instead of size_t
                return std::array<std::size_t, 2>(
                    {{boost::get(ids, boost::source(ed, g)),
                      boost::get(ids, boost::target(ed, g))}}
                );
            }
        );
    }
    
    // Return newly created group
    return grp;
}

} // namespace DataIO
} // namespace Utopia

#endif // DATAIO_GRAPH_UTILS_HH
