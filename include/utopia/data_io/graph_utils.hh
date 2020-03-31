#ifndef UTOPIA_DATAIO_GRAPH_UTILS_HH
#define UTOPIA_DATAIO_GRAPH_UTILS_HH

#include <tuple>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/hana/all_of.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/for_each.hpp>

#include <hdf5.h>

#include "../core/logging.hh"
#include "../core/utils.hh"
#include "../core/graph/iterator.hh"
#include "hdfdataset.hh"
#include "hdfgroup.hh"

using namespace std::literals::string_literals;

namespace Utopia {
namespace DataIO {
/*!
 * \addtogroup DataIO
 * \{
 */

namespace GraphUtilsHelper{

/** This function opens the vertex and edge datasets for a single graph and
 *  adds attributes.
 * @details Via the attributes the size of the graph is stored and the trivial
 *          coordinates (i.e. vertex idx and edge idx) are added.
 *
 * @tparam        Graph
 *
 * @param g       The graph to save
 * @param grp     The HDFGroup the graph should be stored in
 *
 * @return std::pair<std::shared_ptr<HDFDataset<HDFGroup>>,
 *                   std::shared_ptr<HDFDataset<HDFGroup>>>
 *         Pair containing [vertex dataset, edge dataset]
 */
template <typename Graph>
std::pair<std::shared_ptr<HDFDataset>,
          std::shared_ptr<HDFDataset>>
setup_graph_containers(const Graph& g, const std::shared_ptr<HDFGroup>& grp)
{
    // Collect some information on the graph
    const auto num_vertices = boost::num_vertices(g);
    const auto num_edges    = boost::num_edges(g);

    // Get a logger to use here (Note: needs to have been setup beforehand)
    spdlog::get("data_io")->info("Saving graph with {} vertices and {} edges "
                                 "...", num_vertices, num_edges);

    // Store additional metadata in the group attributes
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);

    // Initialize datasets to store vertices and edges in
    auto dset_vertices = grp->open_dataset("_vertices", { num_vertices });
    auto dset_edges = grp->open_dataset("_edges", { 2, num_edges });
    // NOTE Need shape to be {2, num_edges} because write writes line by line.

    // Set attributes
    dset_vertices->add_attribute("dim_name__0", "vertex_idx");
    dset_vertices->add_attribute("coords_mode__vertex_idx", "trivial");

    dset_edges->add_attribute("dim_name__0", "label");
    dset_edges->add_attribute("coords_mode__label", "values");
    dset_edges->add_attribute("coords__label",
                              std::vector<std::string>{"source", "target"});
    dset_edges->add_attribute("dim_name__1", "edge_idx");
    dset_edges->add_attribute("coords_mode__edge_idx", "trivial");

    return std::make_pair(dset_vertices, dset_edges);
}

/// Builds new tuple containing all elements but the first two
template <typename First, typename Second, typename... Tail>
constexpr std::tuple<Tail...> tuple_tail(const std::tuple<First, Second,
                                                          Tail...>& t)
{
    return std::apply([](auto, auto, auto... tail) {
        return std::make_tuple(tail...);}, t);
}

/// Creates empty vector with element-type CoordT. If CoordT is string-like
/// std::string is enforced
template <typename CoordT>
auto coords_container()
{
    if constexpr (Utils::is_string_v<CoordT>) {
        return std::vector<std::string>{};
    }
    else {
        return std::vector<CoordT>{};
    } 
}

/// Kinds of Entities to get properties from
enum class EntityKind {
    vertex,
    edge
};

/** This function generates a write function for graph entity properties.
* @details The generated lambda function can then be called on each write
*          specification in the adaptor tuple. It writes the specified data to
*          a new dataset and adds attributes, fire-and-forget. Depending on the
*          shape of the write specifications 1d or 2d datasets are written, see
*          \ref Utopia::DataIO::save_graph_entity_properties for more detail.
*          Only relying on the given iterator pair, this function allows to
*          handle vertex and edge iterations equivalently.
*
* @tparam EntityKind   The kind of entity to get the data from. This can be
*                      either vertices or edges.
* @tparam Graph
* @tparam NWGroup
* @tparam ItPair
*
* @param g             The graph from which to save vertex or edge properties.
* @param nw_grp        The HDFGroup the data should be written to.
* @param label         The name of the dataset to which the adaptors write data.
* @param it_pair       The iterator pair used to access the desired entities. 
* @param num_entities  The number of entities of kind 'EntityKind' in graph g.
*
* @return auto The generated write function
*/
template <EntityKind entity_kind,
          typename Graph,
          typename NWGroup,
          typename ItPair>
auto generate_write_function(const Graph& g,
                             NWGroup&& nw_grp,
                             std::string label,
                             ItPair&& it_pair,
                             std::size_t num_entities)
{
    static_assert(entity_kind == EntityKind::vertex or
                  entity_kind == EntityKind::edge,
                  "Error, 'entity_kind' has to be either 'vertex' or 'edge'");

    return
        [&g,
         nw_grp{std::forward<NWGroup>(nw_grp)},
         label{std::move(label)},
         it_pair{std::forward<ItPair>(it_pair)},
         num_entities{std::move(num_entities)}] (auto&& write_spec)
        // By-reference captures are used where possible to avoid additional
        // copies. Note that this is not possible for label and num_entities.
        {

        std::string prop_prefix;

        if constexpr (entity_kind == EntityKind::vertex) {
            prop_prefix = "vertex";
        }
        else {
            prop_prefix = "edge";
        }

        const auto grp =
            nw_grp->open_group(std::get<0>(write_spec));

        // ... 1D case if second element of write_spec is a callable object
        //     such that the write specifications are of the form
        //     write_spec = (std::string name_adaptor, lambda adaptor)
        if constexpr (Utils::is_callable_object_v<std::tuple_element_t<1, 
                                    std::decay_t<decltype(write_spec)>>>)
        {
            const auto dset = grp->open_dataset(label, { num_entities });

            dset->write(it_pair.first, it_pair.second,
                [&](auto&& desc) {
                    return std::get<1>(write_spec)(desc, g);
                }
            );

            grp->add_attribute("is_"s + prop_prefix + "_property", true);
            dset->add_attribute("dim_name__0", prop_prefix + "_idx");
            dset->add_attribute("coords_mode__"s + prop_prefix + "_idx",
                                "trivial");
        }

        // ... 2D case otherwise
        //     write_spec = (adaptor_name, dim0_name,
        //                        (coord1, adaptor1),
        //                        (coord2, adaptor2), ...)
        else {

            static_assert(Utils::is_string_v<std::tuple_element_t<1,
                                    std::decay_t<decltype(write_spec)>>>,
                          "Error, the name of dimension 0 has to be s string");

            // Extract the adaptor pairs (coord, adaptor) from write_spec tuple
            const auto adaptor_pairs = tuple_tail(write_spec);
            // FIXME This cannot be calculated at compile time since
            //       hana::for_each uses non-constexpr access.

            // Get number of adaptors
            constexpr auto num_adaptors = std::tuple_size_v<std::decay_t<
                                                    decltype(adaptor_pairs)>>;

            // Open a 2D dataset
            const auto dset = grp->open_dataset(label,
                                                {num_adaptors, num_entities});

            // Deduce coordinate container type from adaptor_pairs
            using WriteSpecT = std::decay_t<decltype(write_spec)>;
            using FirstTupElementT = std::tuple_element_t<2, WriteSpecT>;
            using CoordT = std::tuple_element_t<0, FirstTupElementT>;

            // Create the (empty) coordinates container
            auto coords = coords_container<CoordT>();

            // Create the lambda that saves the data to the captured
            // fire-and-forget dataset and stores the coordinate value
            auto apply_adaptor = [&](auto&& adaptor_pair){
                // adaptor_pair = (coordinate, adaptor_function)
                dset->write(it_pair.first, it_pair.second,
                    [&](auto&& desc) {
                        return std::get<1>(adaptor_pair)(desc, g);
                    }
                );

                static_assert(
                    // assert matching coordinate types
                    std::is_same_v<
                            std::tuple_element_t<0,
                                    std::decay_t<decltype(adaptor_pair)>>,
                            CoordT>,
                    "Error, coordinate types do not match! Check that all "
                    "coordinates are of the same type");

                coords.push_back(std::get<0>(adaptor_pair));
            };

            boost::hana::for_each(adaptor_pairs, apply_adaptor);

            // Extract the 0th dimension name from the write spec
            const std::string dim0_name = std::get<1>(write_spec);

            // Set attributes
            grp->add_attribute("is_"s + prop_prefix + "_property", true);
            dset->add_attribute("dim_name__0", dim0_name);
            dset->add_attribute("coords_mode__"s + dim0_name, "values");
            dset->add_attribute("coords__"s + dim0_name, coords);

            dset->add_attribute("dim_name__1", prop_prefix + "_idx");
            dset->add_attribute("coords_mode__"s + prop_prefix + "_idx",
                                "trivial");
        }
    };
}

} // namespace GraphUtilsHelper

/*!
 * \addtogroup GraphUtilities
 * \{
 */
/**
 * \page GraphUtils Graph Utilities Module
 *
 * \section what Overview
 * This module implements functions which wrap common tasks people face when
 * they wish to extract data from boost::graph objects.
 *
 * \section impl Implementation
 * The provided functions can be separated into two groups, one which saves
 * the entire graph, i.e., its edge-vertex structure, and a second one which
 * allows to extract data associated with either edges or vertices via
 * user supplied functions.
 *
 */

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
template <typename Graph>
std::shared_ptr<HDFGroup>
create_graph_group(const Graph&                       g,
                   const std::shared_ptr<HDFGroup>&   parent_grp,
                   const std::string&                 name)
{
    // Create the group for the graph and store metadata in its attributes
    auto grp = parent_grp->open_group(name);

    grp->add_attribute("content", "graph");
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
template <typename Graph>
void
save_graph(const Graph& g, const std::shared_ptr<HDFGroup>& grp)
{
    // Create and prepare the datasets for vertices and edges
    auto [dset_vertices,
          dset_edges] = GraphUtilsHelper::setup_graph_containers(g, grp);

    // Save vertex list
    auto [v, v_end] = boost::vertices(g);
    dset_vertices->write(v, v_end, [&](auto vd) {
        return boost::get(boost::vertex_index_t(), g, vd);
    });

    // Save edges
    // NOTE Need to call write twice to achieve the desired data shape.
    auto [e, e_end] = boost::edges(g);
    dset_edges->write(e, e_end, [&](auto ed) {
        return boost::get(boost::vertex_index_t(), g, boost::source(ed, g));
    });

    dset_edges->write(e, e_end, [&](auto ed) {
        return boost::get(boost::vertex_index_t(), g, boost::target(ed, g));
    });

    spdlog::get("data_io")->debug("Graph saved.");
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
template <typename Graph, typename PropertyMap>
void
save_graph(const Graph&                       g,
           const std::shared_ptr<HDFGroup>&   grp,
           const PropertyMap                  vertex_ids)
{
    // Create and prepare the datasets for vertices and edges
    auto [dset_vertices,
          dset_edges] = GraphUtilsHelper::setup_graph_containers(g, grp);

    // Save vertex list
    auto [v, v_end] = boost::vertices(g);
    dset_vertices->write(
        v, v_end, [&](auto vd) { return boost::get(vertex_ids, vd); });

    // Save edges
    // NOTE Need to call write twice to achieve the desired data shape.
    auto [e, e_end] = boost::edges(g);
    dset_edges->write(e, e_end, [&](auto ed) {
        return boost::get(vertex_ids, boost::source(ed, g));
    });

    dset_edges->write(e, e_end, [&](auto ed) {
        return boost::get(vertex_ids, boost::target(ed, g));
    });

    spdlog::get("data_io")->debug("Graph saved.");
}

/** This function writes the results of all functions in a named tuple,
*   applied to all vertices/edges of a boost::graph into a HDFGroup.
*
* @details For each adaptor, the data written by this function is available at
*          the path ``nw_grp/adaptor_name/label``, where label is a dataset of
*          size ``{num_edges/vertices}`` in the 1d case or
*          ``{num_adaptors, num_edges/vertices}`` if data from more than one
*          adaptor is to be written.
*          
*          Attributes are added to the datasets that determine the dimension
*          names and the coordinates. The name of dimension -1 is set to
*          ``vertex_idx``/``edge_idx``. The respective coordinates are the
*          trivial ones. In the case of 2d data custom dimension name and
*          coordinates for dimension need to be given. Note that the former has
*          to be string-like and the latter have to be of the same type.
*
* @warning The coordinate type is deduced from the first coordinate. All other
*          coordinates have to be of the same type.
*
* @tparam IterateOver  The type of iterator that is used to access the graph.
*                      The entity properties can be accessed either via
*                      edge-descriptors (IterateOver::edges) or via
*                      vertex-descriptors (IterateOver::vertices).
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
*                      of one of the following forms:
*                      If (adaptor_name, adaptor) then 1d data is written.
*                      If (adaptor_name, dim0_name,
*                               (coord1, adaptor1),
*                               (coord2, adaptor2), ...)
*                      then 2d data is written.
*
* @return void
*/
template <IterateOver iterate_over, typename Graph, typename... Adaptors>
void
save_graph_entity_properties(const Graph&                       g,
                             const std::shared_ptr<HDFGroup>&   nw_grp,
                             const std::string&                 label,
                             const std::tuple<Adaptors...>&     adaptor_tuple)
{

    static_assert(
        // assert stringlike-ness of first thing
        std::conjunction_v<Utils::is_string<
            std::tuple_element_t<0, std::decay_t<Adaptors>>>...>,
        "Error, the first entry of each entry of 'adaptor_tuple' has to be "
        "string like, to name the adaptor");

    static_assert(
        iterate_over == IterateOver::vertices or
        iterate_over == IterateOver::edges, "Error, unknown iterator type! "
        "Has to be either 'IterateOver::vertices' or 'IterateOver::edges'"
    );

    static_assert(std::is_literal_type_v<std::decay_t<decltype(adaptor_tuple)>>,
                  "Error, 'adaptor_tuple' has to be a literal_type. Hence all "
                  "elements in 'adaptor_tuple' (including captured objects) "
                  "have to be a literal_type!");

    // Get a logger
    auto log = spdlog::get("data_io");

    using GraphUtilsHelper::EntityKind;
    using GraphUtilsHelper::generate_write_function;

    // ... vertex_iterator ...
    if constexpr (iterate_over == IterateOver::vertices)
    {
        // Collect some information on the graph
        const auto num_vertices = boost::num_vertices(g);

        // Make vertex iterator pair
        auto it_pair = GraphUtils::iterator_pair<IterateOver::vertices>(g);

        // Get write function that saves data given by the adaptor(s) to new
        // dataset(s), fire-and-forget
        auto write_f = generate_write_function<EntityKind::vertex>(
                                                        g, nw_grp,
                                                        label,
                                                        it_pair,
                                                        num_vertices);

        boost::hana::for_each(adaptor_tuple, write_f);

        log->debug("Graph vertex properties saved with label '{}'.", label);
    }

    // ... and edge_iterator
    else {
        // Collect some information on the graph
        const auto num_edges = boost::num_edges(g);

        // Make edge iterator pair
        auto it_pair = GraphUtils::iterator_pair<IterateOver::edges>(g);

        // Get write function that saves data given by the adaptor(s) to new
        // dataset(s), fire-and-forget
        auto write_f = generate_write_function<EntityKind::edge>(
                                                        g, nw_grp,
                                                        label,
                                                        it_pair,
                                                        num_edges);

        boost::hana::for_each(adaptor_tuple, write_f);

        log->debug("Graph edge properties saved with label '{}'.", label);
    }
}

/** This function writes the results of all functions in a named tuple,
*   applied to all vertices of a boost::graph into a HDFGroup.
*
* @details This function calls the general save_graph_entity_properties method
*          using IterateOver::vertices. See
*          \ref Utopia::DataIO::save_graph_entity_properties for more detail.
*
* @tparam Graph
* @tparam Adaptors
*
* @param g             The graph from which to save vertex properties.
* @param nw_grp        The HDFGroup the data should be written to.
* @param label         Under which label the results of the adaptors should be
*                      stored.
* @param adaptor_tuple Which vertex--associated properties to write.
*
* @return void
*/
template <typename Graph, typename... Adaptors>
void
save_vertex_properties(Graph&&                            g,
                       const std::shared_ptr<HDFGroup>&   nw_grp,
                       const std::string&                 label,
                       const std::tuple<Adaptors...>&     adaptor_tuple)
{
    save_graph_entity_properties<IterateOver::vertices>(std::forward<Graph>(g),
                                                        nw_grp,
                                                        label,
                                                        adaptor_tuple);
}

/** This function writes the results of all functions in a named tuple,
*   applied to all edges of a boost::graph into a HDFGroup.
*
* @details This function calls the general save_graph_entity_properties method
*          using IterateOver::edges. See
*          \ref Utopia::DataIO::save_graph_entity_properties for more detail.
*
* @tparam Graph
* @tparam Adaptors
*
* @param g             The graph from which to save edge properties.
* @param nw_grp        The HDFGroup the data should be written to.
* @param label         Under which label the results of the adaptors should be
*                      stored.
* @param adaptor_tuple Which edge-associated properties to write.
*
* @return void
*/
template <typename Graph, typename... Adaptors>
void
save_edge_properties(Graph&&                            g,
                     const std::shared_ptr<HDFGroup>&   nw_grp,
                     const std::string&                 label,
                     const std::tuple<Adaptors...>&     adaptor_tuple)
{
    save_graph_entity_properties<IterateOver::edges>(std::forward<Graph>(g),
                                                     nw_grp,
                                                     label,
                                                     adaptor_tuple);
}

// TODO add functions here to open datasets for edge or vertex attributes.

/*! \} */ // end of group GraphUtilities
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_GRAPH_UTILS_HH
