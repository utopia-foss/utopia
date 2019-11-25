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
    Vertices,

    /// Iterate over edges.
    Edges,

    /// Iterate over neighbors (adjacent_vertices). 
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    Neighbors,

    /// Iterate inversely over neighbors (adjacent_vertices). 
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    InvNeighbors,

    /// Iterate over the in edges of a vertex. 
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    InEdges,

    /// Iterate over the out edges of a vertex.
    /** This iteration requires a vertex descriptor who's neighbors' to iterate 
     *  over.
     */
    OutEdges
};



/// Get an iterator pair over selected graph entities
/** 
 * \tparam iterate_over Specify over which graph entities to iterate
 *                      Valid options: 
 *                          - Over::Vertices
 *                          - Over::Edges
 * \tparam Graph        The graph type
 * 
 * \param g             The graph
 * 
 * \return decltype(auto) The iterator pair
 */
template<Over iterate_over, typename Graph>
decltype(auto) iterate(const Graph& g){
    if constexpr (iterate_over == Over::Vertices){
        return boost::vertices(g);
    }
    else if constexpr (iterate_over == Over::Edges){
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
 *                          - Over::Neighbors
 *                          - Over::InvNeighbors
 *                          - Over::InEdges
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
    if constexpr (iterate_over == Over::Neighbors){
        return boost::adjacent_vertices(e, g);
    }
    else if constexpr (iterate_over == Over::InvNeighbors){
        return boost::inv_adjacent_vertices(e, g);
    }
    else if constexpr (iterate_over == Over::InEdges){
        return boost::in_edges(e, g);
    }
    else if constexpr (iterate_over == Over::OutEdges){
        return boost::out_edges(e, g);
    }
}


/// Get the iterator range over specified graph entities
/** 
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options: 
 *                          - Over::Vertices
 *                          - Over::Edges
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
 *                          - Over::Neighbors
 *                          - Over::InvNeighbors
 *                          - Over::InEdges
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
template<Over iterate_over, typename Network, typename EntityDesc>
decltype(auto) range(EntityDesc e, const Network& g){
    return boost::make_iterator_range(iterate<iterate_over>(e, g));
}

// end group GraphIterators
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_ITERATORS_HH
