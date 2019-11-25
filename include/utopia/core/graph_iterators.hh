#ifndef UTOPIA_CORE_GRAPH_ITERATORS_HH
#define UTOPIA_CORE_GRAPH_ITERATORS_HH

#include <boost/graph/adjacency_list.hpp>

namespace Utopia {
/**
 *  \addtogroup GraphIterators
 *  \{
 */


/// Over which graph entity to iterate 
enum class Over {
    /// Iterate over vertices.
    vertices,

    /// Iterate over edges.
    edges,

    /// Iterate over neighbors (adjacent_vertices). 
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    neighbors,

    /// Iterate inversely over neighbors (adjacent_vertices). 
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    inv_neighbors,

    /// Iterate over the in edges of a vertex. 
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    in_edges,

    /// Iterate over the out edges of a vertex.
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    out_edges
};



/// Get an iterator pair over selected graph entities
/** 
 * \tparam iterate_over Specify over which graph entities to iterate
 *                      Valid options: 
 *                          - Over::vertices
 *                          - Over::edges
 * \tparam Graph        The graph type
 * 
 * \param g             The graph
 * 
 * \return decltype(auto) The iterator pair
 */
template<Over iterate_over, typename Graph>
decltype(auto) iterate(const Graph& g){
    if constexpr (iterate_over == Over::vertices){
        return boost::vertices(g);
    }
    else if constexpr (iterate_over == Over::edges){
        return boost::edges(g);
    }
}


/// Get an iterator pair over selected graph entities
/** This function returns the iterator pair wrt. another graph entity.
 *  For example iteration over neighbors (adjacent_vertices) needs a references
 *  vertex.
 * 
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options: 
 *                          - Over::neighbors
 *                          - Over::inv_neighbors
 *                          - Over::in_edges
 *                          - Over::OutEgdes
 * \tparam Graph        The graph type
 * \tparam EntityDesc   The graph entity descriptor that is the reference point 
 *                      for the iteration.
 * 
 * \param e             The graph entity that serves as reference
 * \param g             The graph
 * 
 * \return decltype(auto) The iterator pair
 */
template<Over iterate_over, typename Graph, typename EntityDesc>
decltype(auto) iterate(EntityDesc e, const Graph& g){
    if constexpr (iterate_over == Over::neighbors){
        return boost::adjacent_vertices(e, g);
    }
    else if constexpr (iterate_over == Over::inv_neighbors){
        return boost::inv_adjacent_vertices(e, g);
    }
    else if constexpr (iterate_over == Over::in_edges){
        return boost::in_edges(e, g);
    }
    else if constexpr (iterate_over == Over::out_edges){
        return boost::out_edges(e, g);
    }
}


/// Get the iterator range over specified graph entities
/** 
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options: 
 *                          - Over::vertices
 *                          - Over::edges
 * \tparam Graph        The graph type
 * 
 * \param g             The graph
 * 
 * \return decltype(auto) The iterator range
 */
template<Over iterate_over, typename Graph>
decltype(auto) range(const Graph& g){
    return boost::make_iterator_range(iterate<iterate_over>(g));
}


/// Get the iterator range over specified graph entities
/** This function returns the iterator range wrt. another graph entity.
 *  For example iterating of the neighbors (adjacent_vertices) of a vertex
 *  requires a vertex descriptor as reference. 
 * 
 * 
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options: 
 *                          - Over::neighbors
 *                          - Over::inv_neighbors
 *                          - Over::in_edges
 *                          - Over::OutEgdes
 * \tparam Graph        The graph type
 * \tparam EntityDesc   The graph entity descriptor that is the reference point 
 *                      for the iteration.
 * 
 * \param e             The graph entity that serves as reference
 * \param g             The graph
 * 
 * \return decltype(auto) The iterator range
 */
template<Over iterate_over, typename Graph, typename EntityDesc>
decltype(auto) range(EntityDesc e, const Graph& g){
    return boost::make_iterator_range(iterate<iterate_over>(e, g));
}

// end group GraphIterators
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_ITERATORS_HH
