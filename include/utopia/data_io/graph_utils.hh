#ifndef UTOPIA_DATAIO_GRAPH_UTILS_HH
#define UTOPIA_DATAIO_GRAPH_UTILS_HH

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/all_of.hpp>
#include <boost/hana/ext/std/tuple.hpp>

#include <hdf5.h>

#include "../core/logging.hh"
#include "../core/utils.hh"
#include "hdfgroup.hh"
#include "hdfdataset.hh"

namespace Utopia {
namespace DataIO {


/** This function opens a HDFGroup for graph data and adds attributes.
 * @details The attributes that are set mark the newly created group as
 *          containing network data and add some graph metadata like whether
 *          the graph is directed and/or allows parallel edges.
 * 
 * @tparam Graph
 *
 * @param g          The graph to save
 * @param parent_grp The parent HDFGroup the graph data should be stored in
 * @param name       The name the newly created graph group should have
 *
 * @return std::shared_ptr<HDFGroup> The newly created graph group
 */
template<typename Graph>
std::shared_ptr<HDFGroup>
    create_graph_group(const Graph& g,
                       const std::shared_ptr<HDFGroup>& parent_grp,
                       const std::string& name)
{
    // Create the group for the graph and store metadata in its attributes
    auto grp = parent_grp->open_group(name);

    grp->add_attribute("content", "network");
    grp->add_attribute("is_directed", boost::is_directed(g));
    grp->add_attribute("allows_parallel", boost::allows_parallel_edges(g));

    spdlog::get("data_io")->info("Opened graph group '{}'.", name);

    return grp;
}


/// Write function for a boost::Graph
/** This function writes a boost::graph into a HDFGroup. It assumes that the
 *  vertices and edges of the graph already supply indices.
 * 
 * @tparam Graph 
 *
 * @param g The graph to save
 * @param grp The HDFGroup the graph should be stored in
 *
 * @return void
 */
template<typename Graph>
void save_graph(const Graph& g,
                const std::shared_ptr<HDFGroup>& grp)
{
    // Collect some information on the graph
    const auto num_vertices = boost::num_vertices(g);
    const auto num_edges = boost::num_edges(g);

    // Get a logger to use here (Note: needs to have been setup beforehand)
    auto log = spdlog::get("data_io");
    log->info("Saving graph with {} vertices and {} edges ...",
              num_vertices, num_edges);

    // Store additional metadata in the group attributes
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    
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
}


/// Write function for a boost::Graph
/** This function writes a boost::graph into a HDFGroup. By supplying custom
 *  vertex IDs, they need not be part of the graph in order for this function
 *  to operate.
 * 
 * @tparam Graph 
 * @tparam PropertyMap The property map of the vertex ids
 *
 * @param g The graph to save
 * @param grp The HDFGroup the graph should be stored in
 * @param vertex_ids A custom property map of vertex IDs to use
 *
 * @return void
 */
template<typename Graph, typename PropertyMap>
void save_graph(const Graph& g,
                const std::shared_ptr<HDFGroup>& grp,
                const PropertyMap vertex_ids)
{
    // Collect some information on the graph
    const auto num_vertices = boost::num_vertices(g);
    const auto num_edges = boost::num_edges(g);

    // Get a logger to use here (Note: needs to have been setup beforehand)
    auto log = spdlog::get("data_io");
    log->info("Saving graph with {} vertices and {} edges ...",
              num_vertices, num_edges);


    // Store additional metadata in the group attributes
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    
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
}


// helper functions for checking properties of adaptor types for save_graph_properties

// checks if typename T is tuplelike of size 2, base case
template <typename T, typename U = std::void_t<>>
struct is_size_two : std::false_type
{
};

// checks if typenam T is tuplelike of size 2, positive case
template <typename T>
struct is_size_two<
    T, std::void_t<std::enable_if_t<std::tuple_size_v<std::decay_t<T>> == 2>>>
    : std::true_type
{
};

/** This function writes the results of all functions in a named tuple,
*   applied to all vertices/edges of a boost::graph into a HDFGroup.
*
*   For each adaptor, the data written by this function is available at the
*   path: ``nw_grp/adaptor_name/label``, where label is a dataset of size
*   ``{num_adaptors, num_edges/vertices}``.
*
* @tparam ObjectType   The type of object that information is written from.
*                      Specify the vertex or edge type here. This has to match
*                      the vertex or edge types of the corresponding graph!
*                      Alternatively an vertex or edge descriptor can be given.
*                      However, the descriptor has to be applied to the graph
*                      in the named adaptor tuple. Thus it also has to match
*                      the given graph!
* @tparam Graph
* @tparam Adaptors
*
* @param g             The graph from which to save vertex or edge properties.
* @param nw_grp        The HDFGroup the data should be written to. This should
*                      be the previously created network group.
*                      For each tuple entry, a new group will be created,
*                      which has the name specified as first element of that
*                      tuple.
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
template <typename ObjectType, typename Graph, typename... Adaptors>
void
save_graph_properties(Graph& g, const std::shared_ptr<HDFGroup>& nw_grp,
                      const std::string& label,
                      std::tuple<Adaptors...>& adaptor_tuple)
{

    static_assert(
        // assert size of adaptors
        std::conjunction_v<is_size_two<std::decay_t<Adaptors>>...>,
        "Error: The content of 'adaptor_tuple' has to be tuplelike of size 2");

    static_assert(
        // assert stringlike-ness of first thing
        std::conjunction_v<Utils::is_string<
            std::tuple_element_t<0, std::decay_t<Adaptors>>>...>,
        "Error, the first entry of each entry of 'adaptor_tuple' has to be "
        "string like, to name the adaptor");

    static_assert(
        // assert that the second one is a callable
        std::conjunction_v<Utils::is_callable_object<
            std::tuple_element_t<1, std::decay_t<Adaptors>>>...>,
        "Error, the second enry of each entry of 'adaptor_tuple has to be a "
        "callable "
        "object");

    static_assert(
        std::is_same_v<ObjectType, typename Graph::vertex_property_type>
        or
        std::is_same_v<ObjectType, typename Graph::edge_property_type>
        or
        std::is_same_v<ObjectType, typename Graph::vertex_descriptor>
        or
        std::is_same_v<ObjectType, typename Graph::edge_descriptor>,
        "Error: the vertex or edge type does not match the given graph!"
    );

    // enum to ACCess members of adaptortuple elements by name
    enum Acc{name, adaptor};

    // Get a logger
    auto log = spdlog::get("data_io");

    // Distinguish between vertex property, ...
    if constexpr (std::is_same_v<ObjectType,
                                 typename Graph::vertex_property_type>)
    {
        // Collect some information on the graph
        const auto num_vertices = boost::num_vertices(g);

        // Make vertex iterators
        typename Graph::vertex_iterator v, v_end;
        boost::tie(v, v_end) = boost::vertices(g);


        // assert scalar property of adaptor return type
        static_assert(
            std::conjunction_v<
                std::is_scalar<std::invoke_result_t<
                    std::tuple_element_t<1, Adaptors>,
                    decltype(g[*v])
                    >
                >...
            >,    
            "Error: Adaptors need to return scalar types.");

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // adaptor_pair = [name, adaptor_function]
        auto writer_f = [&](auto&& adaptor_pair) {
            const auto grp = nw_grp->open_group(std::get<Acc::name>(adaptor_pair));
            
            
            grp->open_dataset(label, {num_vertices})
               ->write(v, v_end, [&](auto&& vd){ 
                    return std::get<Acc::adaptor>(adaptor_pair)(g[vd]);
                });

            // NOTE At the moment this attribute is added to the data group
            //      every time data is written, overwriting the existing one.
            //      Once the DataManager is available this could and should
            //      easily be handled such that it is written only once.
            grp->add_attribute("is_vertex_property", true);
        };

        boost::hana::for_each(
          std::forward<std::tuple<Adaptors...>>(adaptor_tuple), writer_f);

        log->debug("Graph vertex properties saved with label '{}'.", label);
    }

    // ... edge property,...
    else if constexpr (std::is_same_v<ObjectType,
                                      typename Graph::edge_property_type>)
    {
        // Collect some information on the graph
        const auto num_edges = boost::num_edges(g);

        // Make edge iterators
        typename Graph::edge_iterator ei, ei_end;
        boost::tie(ei, ei_end) = boost::edges(g);

        // assert scalar property of adaptor return type
        static_assert(
            std::conjunction_v<std::is_scalar<std::invoke_result_t<
                std::tuple_element_t<1, Adaptors>, decltype(g[*ei])>>...>,
            "Error: Adaptors need to return scalar types.");

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // adaptor_pair = [name, adaptor_function]
        auto writer_f = [&](auto&& adaptor_pair) {
            const auto grp = nw_grp->open_group(std::get<Acc::name>(adaptor_pair));

            grp->open_dataset(label, { num_edges })
               ->write(ei, ei_end, [&](auto&& ed) {
                    return std::get<Acc::adaptor>(adaptor_pair)(g[ed]);
              });

            // NOTE At the moment this attribute is added to the data group
            //      every time data is written, overwriting the existing one.
            //      Once the DataManager is available this could and should
            //      easily be handled such that it is written only once.
            grp->add_attribute("is_edge_property", true);
        };


        boost::hana::for_each(
          std::forward<std::tuple<Adaptors...>>(adaptor_tuple), writer_f);

        log->debug("Graph edge properties saved with label '{}'.", label);
    }

    // ... vertex_descriptor ...
    else if constexpr (std::is_same_v<ObjectType,
                                      typename Graph::vertex_descriptor>)
    {
        // Collect some information on the graph
        const auto num_vertices = boost::num_vertices(g);

        // Make edge iterators
        typename Graph::vertex_iterator v, v_end;
        boost::tie(v, v_end) = boost::vertices(g);

        // assert scalar property of adaptor return type
        static_assert(
            std::conjunction_v<std::is_scalar<
                std::invoke_result_t<std::tuple_element_t<1, Adaptors>,
                                     decltype(g), decltype(*v)&>>...>,
            "Error: Adaptors need to return scalar types.");

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // adaptor_pair = [name, adaptor_function]
        auto writer_f = [&](auto&& adaptor_pair) {

            nw_grp->open_group(std::get<Acc::name>(adaptor_pair))
              ->open_dataset(label, { num_vertices })
              ->write(v, v_end, [&](auto&& vd) {
                  return std::get<Acc::adaptor>(adaptor_pair)(g, vd);
              });
        };

        boost::hana::for_each(
            std::forward<std::tuple<Adaptors...>>(adaptor_tuple), writer_f);

        log->debug("Graph vertex properties saved with label '{}'.", label);
    }

    // ... and edge_descriptor
    else if constexpr (std::is_same_v<ObjectType,
                                      typename Graph::edge_descriptor>)
    {

        // Collect some information on the graph
        const auto num_edges = boost::num_edges(g);

        // Make edge iterators
        typename Graph::edge_iterator ei, ei_end;
        boost::tie(ei, ei_end) = boost::edges(g);

        // assert scalar property of adaptor return type
        static_assert(
            std::conjunction_v<std::is_scalar<
                std::invoke_result_t<std::tuple_element_t<1, Adaptors>,
                                     decltype(g), decltype(*ei)&>>...>,
            "Error: Adaptors need to return scalar types.");

        // Save data given by an adaptor to a new dataset, fire-and-forget
        // adaptor_pair = [name, adaptor_function]
        auto writer_f = [&](auto&& adaptor_pair) {
            nw_grp->open_group(std::get<Acc::name>(adaptor_pair))
                ->open_dataset(label, { num_edges })
                ->write(ei, ei_end, [&](auto&& ed) {
                    return std::get<Acc::adaptor>(adaptor_pair)(g, ed);
              });
        };

        boost::hana::for_each(
            std::forward<std::tuple<Adaptors...>>(adaptor_tuple), writer_f);

        log->debug("Graph edge properties saved with label '{}'.", label);
    }
}

// TODO add functions here to open datasets for edge or node attributes.

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_GRAPH_UTILS_HH
