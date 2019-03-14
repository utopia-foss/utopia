#ifndef UTOPIA_DATAIO_GRAPH_UTILS_HH
#define UTOPIA_DATAIO_GRAPH_UTILS_HH

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>

#include <hdf5.h>

#include "../core/logging.hh"
#include "hdfgroup.hh"
#include "hdfdataset.hh"

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


namespace _graph_utils_helper {

/** Helper to apply all sub-adaptors of a name-adaptor-tuple to the writer
 * Determines the tuple size and adapts the dataset size
*
* @tparam Function
* @tparam Is
* @tparam Tuple
*
* @param writer_f lambda to save data
* @param std::index_sequence The index_sequence to unpack
*                            the `name_adaptor_tuple`
* @param name_adaptor_tuple A name-lambda-tuple
*
* @return void
*/
template<typename Function, std::size_t... Is, typename Tuple>
constexpr void apply_to_write(Function&& writer_f,
                              std::index_sequence<0, Is...>,
                              Tuple&& name_adaptor_tuple)
{

    // Get size of sub-tuple
    constexpr std::size_t N = std::tuple_size<Tuple>::value;

    static_assert(N == 2, "Irregular name-adaptor-tuple size"
                          "Should be 2");

    (std::apply(
        writer_f,
        std::forward_as_tuple(
            // Get the dataset name
            std::get<0>(name_adaptor_tuple),
            // apply all sub-adaptors
            std::get<Is>(name_adaptor_tuple),
            // size equal number adaptors
            N - 1)
            ),
        ...);

}

/** Helper to call `apply_to_writer` for every lambda of the input tuple
*
* @tparam Function
* @tparam Adaptors
*
* @param writer_f lambda to save data
* @param name_adaptor_tuple Tuple of a datagroup-name and an arbitrary
*                           amount of lambdas
*                           which state how to get data from a given graph
*
* @return void
*/
template<typename Function, typename... Adaptors>
constexpr void apply_one(Function&& writer_f,
                         std::tuple<Adaptors...>& name_adaptor_tuple)
{
    apply_to_write(
        writer_f,
        std::index_sequence_for<Adaptors...>(),
        std::forward<std::tuple<Adaptors...> >(name_adaptor_tuple)
        );

}


/** Helper to call `apply_one` for every element of the input tuple
*
* @tparam Function
* @tparam Is
* @tparam Tuple
*
* @param writer_f lambda to save data
* @param std::index_sequence The index_sequence to unpack the `adaptor_tuple`
* @param adaptor_tuple Tuple of name-lambda-tuples
*
* @return void
*/
template<typename Function, std::size_t... Is, typename Tuple>
constexpr void apply_all(Function&& writer_f,
                         std::index_sequence<Is...>,
                         Tuple&& adaptor_tuple)
{
    (apply_one(
        std::forward<Function>(writer_f),
        std::get<Is>(adaptor_tuple)),
                ...);
}

} // namespace _graph_utils_helper


/** This function writes the results of all functions in a named tuple,
*   applied to all vertices/edges of a boost::graph into a HDFGroup.
*
*   For each adaptor, the data written by this function is available at the
*   path: ``parent_grp/adaptor_name/label``, where label is a dataset of size
*   ``{num_adaptors, num_edges/vertices}``.
*
* @tparam ObjectType   The type of object that information is written from.
*                      Specify the vertex or edge type here. This has to match
*                      the vertex or edge types of the corresponding graph!
*                      Alternatively an vertex or edge descriptor can be given.
*                      However, the descriptor has to be applied to the graph
*                      in the named adaptor tuple. Thus it also has to match
*                      the given graph!
* @tparam GraphType
* @tparam Adaptors
*
* @param g             The graph from which to save vertex or edge properties.
* @param parent_grp    The HDFGroup the data should be written to. For each
*                      tuple entry, a new group will be created, which has the
*                      name specified in that tuple.
* @param label         Under which label the results of the adaptors should be
*                      stored. This will be the name of the dataset to which
*                      the adaptors write data. If time-varying data is to be
*                      written, this can be used to specify the time step.
* @param adaptor_tuple Which vertex- or edge-associated properties to write.
*                      This should be a tuple of tuples, where the latter are
*                      of form (std::string adaptor_name, lambda adaptor).
*
* @return void
*/
template<typename ObjectType,
         typename GraphType,
         typename... Adaptors>
void save_graph_properties(GraphType &g,
                           const std::shared_ptr<HDFGroup>& parent_grp,
                           const std::string& label,
                           std::tuple<Adaptors...>& adaptor_tuple)
{
    static_assert(
        std::is_same_v<ObjectType, typename GraphType::vertex_property_type>
        or
        std::is_same_v<ObjectType, typename GraphType::edge_property_type>
        or
        std::is_same_v<ObjectType, typename GraphType::vertex_descriptor>
        or
        std::is_same_v<ObjectType, typename GraphType::edge_descriptor>,
        "The vertex or edge type does not match the given graph!");

    // Get a logger
    auto log = spdlog::get("data_io");

    // Distinguish between vertex ...
    if constexpr (std::is_same_v<ObjectType,
                                 typename GraphType::vertex_property_type>)
    {
        // Collect some information on the graph
        const auto num_vertices = boost::num_vertices(g);

        // Make vertex iterators
        typename GraphType::vertex_iterator v, v_end;
        boost::tie(v, v_end) = boost::vertices(g);

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // Format: parent_grp/adaptor_name/label
        auto writer_f = [&](auto&& adaptor_name, auto&& adaptor, auto&&){
            parent_grp->open_group(adaptor_name)
                ->open_dataset(label, {num_vertices})
                ->write(v, v_end,
                        [&](auto&& vd){ return adaptor(g[vd]); });
        };

        // Apply all tuple elements to the writer function
        _graph_utils_helper::apply_all(
            writer_f,
            std::index_sequence_for<Adaptors...>(),
            std::forward<std::tuple<Adaptors...> >(adaptor_tuple)
        );

        log->debug("Graph vertex properties saved with label '{}'.", label);
    }

    // ... edge ...
    else if constexpr (std::is_same_v<ObjectType,
                                      typename GraphType::edge_property_type>)
    {
        // Collect some information on the graph
        const auto num_edges = boost::num_edges(g);

        // Make edge iterators
        typename GraphType::edge_iterator v, v_end;
        boost::tie(v, v_end) = boost::edges(g);

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // Format: parent_grp/adaptor_name/label
        auto writer_f = [&](auto&& adaptor_name, auto&& adaptor, auto&&){
            parent_grp->open_group(adaptor_name)
                ->open_dataset(label, {num_edges})
                ->write(v, v_end,
                        [&](auto&& vd){ return adaptor(g[vd]); });
        };

        // Apply all tuple elements to the writer function
        _graph_utils_helper::apply_all(
            writer_f,
            std::index_sequence_for<Adaptors...>(),
            std::forward<std::tuple<Adaptors...> >(adaptor_tuple)
        );

        log->debug("Graph edge properties saved with label '{}'.", label);
    }

    // ... vertex_descriptor ...
    else if constexpr (std::is_same_v<ObjectType,
                                      typename GraphType::vertex_descriptor>)
    {
        // Collect some information on the graph
        const auto num_vertices = boost::num_vertices(g);

        // Make edge iterators
        typename GraphType::vertex_iterator v, v_end;
        boost::tie(v, v_end) = boost::vertices(g);

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // Format: parent_grp/adaptor_name/label
        auto writer_f = [&](auto&& adaptor_name, auto&& adaptor, auto&&){
            parent_grp->open_group(adaptor_name)
                    ->open_dataset(label, {num_vertices})
                    ->write(v, v_end,
                            [&](auto&& vd){ return adaptor(g, vd); });
        };

        // Apply all tuple elements to the writer function
        _graph_utils_helper::apply_all(
                writer_f,
                std::index_sequence_for<Adaptors...>(),
                std::forward<std::tuple<Adaptors...> >(adaptor_tuple)
        );

        log->debug("Graph vertex properties saved with label '{}'.", label);

    }

    // ... and edge_descriptor
    else if constexpr (std::is_same_v<ObjectType,
                                      typename GraphType::edge_descriptor>)
    {

        // Collect some information on the graph
        const auto num_edges = boost::num_edges(g);

        // Make edge iterators
        typename GraphType::edge_iterator v, v_end;
        boost::tie(v, v_end) = boost::edges(g);

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // Format: parent_grp/adaptor_name/label
        auto writer_f = [&](auto&& adaptor_name, auto&& adaptor, auto&&){
            parent_grp->open_group(adaptor_name)
                    ->open_dataset(label, {num_edges})
                    ->write(v, v_end,
                            [&](auto&& vd){ return adaptor(g, vd); });
        };

        // Apply all tuple elements to the writer function
        _graph_utils_helper::apply_all(
                writer_f,
                std::index_sequence_for<Adaptors...>(),
                std::forward<std::tuple<Adaptors...> >(adaptor_tuple)
        );

        log->debug("Graph edge properties saved with label '{}'.", label);
    }
}

// TODO add functions here to open datasets for edge or node attributes.

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_GRAPH_UTILS_HH
