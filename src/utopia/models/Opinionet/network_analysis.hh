#ifndef UTOPIA_MODELS_OPINIONET_NETWORK_ANALYSIS
#define UTOPIA_MODELS_OPINIONET_NETWORK_ANALYSIS

#include <vector>
#include <algorithm>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/betweenness_centrality.hpp>
#include <boost/property_map/property_map.hpp>

namespace Utopia::Models::Opinionet::NetworkAnalysis {

// TODO Add tests for the network analysis functions
// TODO The algorithms can be improved, e.g. by using a union-find data
//      structure (there is a boost implementation:
//      https://www.boost.org/doc/libs/1_75_0/boost/pending/disjoint_sets.hpp).

// .. Helper functions ........................................................


// Starting from a given vertex, iteratively collect all vertices in tolerance
// range that are connected through an in edge or out edge.
template<typename NWType>
void fill_opinion_cluster(
            const size_t v,
            std::vector<size_t>& c,
            const double tolerance,
            NWType& nw)
{
    if (std::find(c.begin(), c.end(), v) == c.end()) {
        c.push_back(v);
        for (const auto w : range<IterateOver::neighbors>(v, nw)) {
            if (fabs(nw[v].opinion - nw[w].opinion) <= tolerance)
            {
                fill_opinion_cluster(w, c, tolerance, nw);
            }
        }
        for (const auto e : range<IterateOver::in_edges>(v, nw)) {
            if (fabs(nw[v].opinion - nw[boost::source(e, nw)].opinion)
                <= tolerance)
            {
                fill_opinion_cluster(boost::source(e, nw), c, tolerance, nw);
            }
        }
    }
}


// Starting from a given vertex, iteratively collect all vertices in tolerance
// range which are connected through an in edge or out edge and have a weight
// larger than the given threshold `min_weight`. Requires edge properties
// containing a `weight` member.
template<typename NWType>
void fill_weighted_opinion_cluster(
            const size_t v,
            std::vector<size_t>& c,
            const double tolerance,
            const double min_weight,
            NWType& nw)
{
    if (std::find(c.begin(), c.end(), v) == c.end()) {
        c.push_back(v);
        for (const auto w : range<IterateOver::neighbors>(v, nw)) {
            if ((fabs(nw[v].opinion - nw[w].opinion) <= tolerance)
                and (nw[boost::edge(v, w, nw).first].weight
                     * boost::out_degree(v, nw) >= min_weight))
            {
                fill_weighted_opinion_cluster(w, c, tolerance, min_weight, nw);
            }
        }
        for (const auto e : range<IterateOver::in_edges>(v, nw)) {
            if ((fabs(nw[v].opinion - nw[boost::source(e, nw)].opinion)
                 <= tolerance)
                and (nw[e].weight * out_degree(source(e, nw), nw) >= min_weight))
            {
                fill_weighted_opinion_cluster(boost::source(e, nw), c,
                                              tolerance, min_weight, nw);
            }
        }
    }
}


// Starting from a given vertex, iteratively collect all adjacent vertices.
template<typename NWType>
void fill_community(
            const size_t v,
            std::vector<size_t>& c,
            NWType& nw)
{
    if (std::find(c.begin(), c.end(), v) == c.end()) {
        c.push_back(v);
        for (const auto w : range<IterateOver::neighbors>(v, nw)) {
            fill_community(w, c, nw);
        }
    }
}


// .. Network topology analysis functions .....................................


// Calculate the reciprocity for a single node (= fraction of outgoing
// links for which the mutual link exists as well).
template<typename NWType, typename VertexDescType>
double reciprocity(NWType& nw, const VertexDescType v)
{
    double r = 0.;
    for (const auto w : range<IterateOver::neighbors>(v, nw)) {
        if (edge(w, v, nw).second) {
            r += 1.;
        }
    }

    return r / double(boost::out_degree(v, nw));
}


// Calculate the reciprocity of the whole graph (= fraction of mutual links).
template<typename NWType>
double reciprocity(NWType& nw)
{
    double r = 0.;
    for (const auto e : range<IterateOver::edges>(nw)) {
        if (boost::edge(boost::target(e, nw), boost::source(e, nw), nw).second)
        {
            r += 1.;
        }
    }

    return r / double(boost::num_edges(nw));
}


// Calculate the betweenness centrality of each vertex.
template<typename NWType>
std::vector<double> betweenness_centrality(NWType& nw)
{
    std::vector<double> centrality(boost::num_vertices(nw));
    
    boost::brandes_betweenness_centrality(nw,
            boost::make_iterator_property_map(centrality.begin(),
                                              get(boost::vertex_index, nw),
                                              double())
            );

    return centrality;
}


// Calculate the relative betweenness centrality for each vertex
// (normalized with the highest possible value which would be reached
//  if a node is crossed by every single shortest path).
template<typename NWType>
std::vector<double> relative_betweenness_centrality(NWType& nw)
{
    std::vector<double> centrality(num_vertices(nw));
    
    boost::brandes_betweenness_centrality(nw,
            boost::make_iterator_property_map(centrality.begin(),
                                              get(boost::vertex_index, nw),
                                              double())
            );

    boost::relative_betweenness_centrality(nw,
            boost::make_iterator_property_map(centrality.begin(),
                                              get(boost::vertex_index, nw),
                                              double())
            );

    // Division by 2 is needed for directed graphs.
    for (auto& val: centrality) {
        val /= 2.;
    }

    return centrality;
}


// Identify groups of agents that are connected via out-edges.
// NOTE that completely isolated vertices are also identified
//      as closed community.
template<typename NWType>
std::vector<std::vector<size_t>> closed_communities(NWType& nw) {
    
    std::vector<std::vector<size_t>> cc;
    std::vector<size_t> temp_c;
    bool next;

    // Find all communities through a loop over all vertices as the
    // source of the community.
    for (const auto v : range<IterateOver::vertices>(nw)) {

        next = false;

        // If vertex is part of an already discovered community
        // its community has to be the same (if out-degree > 0).
        for (auto& c: cc) {
            if (std::find(c.begin(), c.end(), v) != c.end()) {
                next = true;
            }
        }

        if (next) {
            continue;
        }
        
        else {
            if (boost::in_degree(v, nw) < 2) {
                // This is the case of a 'loner'.
                for (auto& c: cc) {
                    for (auto& w: c) {
                        if (boost::edge(v, w, nw).second) {
                            c.push_back(v);
                            next = true;
                            break;
                        }
                    }
                }
            }
            else {
                // Else get the community originating from the vertex.
                temp_c.clear();
                fill_community(v, temp_c, nw);
                cc.push_back(temp_c);
            }
        }
    }

    return cc;
}


// .. Opinion analysis functions ..............................................


// Identify groups of agents with similar (within tolerance range) opinions.
template<typename NWType>
std::vector<std::vector<size_t>> opinion_groups(NWType& nw,
                                                const double tolerance)
{
    // First, get pairs of opinion values and vertices
    std::vector<std::pair<double, size_t>> op_v;
    std::vector<std::vector<size_t>> groups;

    for (const auto v : range<IterateOver::vertices>(nw)) {
        op_v.emplace_back(std::make_pair(nw[v].opinion, v));
    }

    // sort along the opinion values
    std::sort(op_v.begin(), op_v.end());

    // loop over opinions and make a cut wherever the opinion distance
    // is larger than the tolerance range
    size_t start = 0;
    for (size_t i=0; i!=op_v.size()-1; i++) {

        if (fabs(op_v[i].first - op_v[i+1].first) >= tolerance) {
            std::vector<size_t> group;

            for (size_t j=start; j!=i+1; j++) {
                group.push_back(op_v[j].second);
            }

            start = i+1;
            groups.push_back(group);
        }
    }

    // Add last group
    std::vector<size_t> group;
    for (size_t j=start; j!=op_v.size(); j++) {
        group.push_back(op_v[j].second);
    }
    groups.push_back(group);

    return groups;
}


// Identify groups of agents with similar (within tolerance range) opinions
// that are connected on the network.
template<typename NWType>
std::vector<std::vector<size_t>> opinion_clusters(NWType& nw,
                                                  const double tolerance)
{
    std::vector<std::vector<size_t>> opinion_clusters;
    std::vector<size_t> temp_c;
    bool next;

    // Find all opinion clusters through a loop over all vertices as the
    // source of the cluster.
    for (const auto v : range<IterateOver::vertices>(nw)) {

        next = false;

        // If vertex is part of an already discovered opinion cluster
        // its cluster is the same (by definition).
        for (auto& c: opinion_clusters) {
            if (std::find(c.begin(), c.end(), v) != c.end()) {
                next = true;
            }
        }

        if (next) {
            continue;
        }

        // Else get the opinion cluster around this vertex.
        else {
            temp_c.clear();
            fill_opinion_cluster(v, temp_c, tolerance, nw);
            opinion_clusters.push_back(temp_c);
        }
    }

    return opinion_clusters;
}


// Identify groups of agents with similar (within tolerance range) opinions
// that are connected on the network (with in or out edges that have a weight
// larger than a certain threshold). Requires edge properties containing a
// `weight` member.
template<typename NWType>
std::vector<std::vector<size_t>> weighted_opinion_clusters( 
                                                NWType& nw,
                                                const double tolerance,
                                                double min_weight)
{
    std::vector<std::vector<size_t>> opinion_clusters;
    std::vector<size_t> temp_c;
    bool next;

    // Find all opinion clusters through a loop over all vertices as the
    // source of the cluster.
    for (const auto v : range<IterateOver::vertices>(nw)) {

        next = false;

        // If vertex is part of an already discovered opinion cluster
        // its cluster is the same (by definition).
        for (auto& c: opinion_clusters) {
            if (std::find(c.begin(), c.end(), v) != c.end()) {
                next = true;
            }
        }

        if (next) {
            continue;
        }

        // Else get the opinion cluster around this vertex.
        else {
            temp_c.clear();
            fill_weighted_opinion_cluster(v, temp_c, tolerance, min_weight, nw);
            opinion_clusters.push_back(temp_c);
        }
    }

    return opinion_clusters;
}

} // namespace Utopia::Models::Opinionet::NetworkAnalysis

#endif // UTOPIA_MODELS_OPINIONET_NETWORK_ANALYSIS
