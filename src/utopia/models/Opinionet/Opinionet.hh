#ifndef UTOPIA_MODELS_OPINIONET_HH
#define UTOPIA_MODELS_OPINIONET_HH

#include <functional>
#include <boost/graph/random.hpp>

#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/types.hh>
#include <utopia/core/graph.hh>
#include <utopia/data_io/graph_utils.hh>

#include "network_analysis.hh"
#include "modes.hh"
#include "revision.hh"


namespace Utopia::Models::Opinionet {

using modes::Interaction_type;
using modes::Interaction_type::Deffuant;
using modes::Interaction_type::HegselmannKrause;
using modes::Opinion_space_type;
using modes::Opinion_space_type::continuous;
using modes::Opinion_space_type::discrete;
using modes::Rewiring;
using modes::Rewiring::RewiringOn;
using modes::Rewiring::RewiringOff;


/// Each node in the network accomodates a single agent
struct Agent {
    double opinion;
};

/// Each network edge has a weight representing an interaction probability
struct Edge {
    double weight;
};

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The vertex container type
using VertexContainer = boost::vecS;

/// The edge container type
using EdgeContainer = boost::vecS;

/// The undirected network type
using NetworkUndirected = boost::adjacency_list<
    EdgeContainer,
    VertexContainer,
    boost::undirectedS,
    Agent>;

/// The directed network type
using NetworkDirected = boost::adjacency_list<
    EdgeContainer,
    VertexContainer,
    boost::bidirectionalS,
    Agent,
    Edge>;

// The undirected network type
using NetworkUndir = boost::adjacency_list<
    EdgeContainer,
    VertexContainer,
    boost::undirectedS,
    Agent
>;

enum InteractionMode {
    deffuant,
    hegselmann_krause
};

/// Typehelper to define data types of the Opinionet model
using OpinionetTypes = Utopia::ModelTypes<>;

// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The Opinionet model class
/**
 *  This is a 1d opinion dynamics model with interactions based on
 *  bounded confidence.
 */
// TODO Find better argument names (?) Perhaps make capitalization consistent.
template<Interaction_type interaction_type=Deffuant,
         Opinion_space_type opinion_space=continuous,
         typename NWType=NetworkUndirected,
         Rewiring rewire=RewiringOff>
class Opinionet:
    public Model<Opinionet<interaction_type, opinion_space, NWType, rewire>,
                 OpinionetTypes>
{
public:
    /// The base model type
    using Base = Model<Opinionet, OpinionetTypes>;

    /// Data type that holds the configuration
    using Config = typename Base::Config;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _log, _space

    /// Network and model dynamics parameters
    NWType _nw;
    const double _tolerance;
    const double _susceptibility;

    /// A uniform probability distribution
    std::uniform_real_distribution<double> _uniform_prob_distr;

    // Datasets and Datagroups
    std::shared_ptr<DataGroup> _dgrp_nw;
    std::shared_ptr<DataSet> _dset_edge_weights;
    std::shared_ptr<DataSet> _dset_opinion;

public:
    /// Construct the Opinionet model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    Opinionet (const std::string name,
               ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),
        // Initialize the network
        _nw(this->initialize_nw()),
        // Initialize the model parameters
        _tolerance(get_as<double>("tolerance", this->_cfg)),
        _susceptibility(get_as<double>("susceptibility", this->_cfg)),
        _uniform_prob_distr(0., 1.),
        // Create datagroups and datasets
        _dgrp_nw(create_graph_group(_nw, this->_hdfgrp, "nw")),
        // TODO Only if network is directed
        _dset_edge_weights(this->create_dset("edge_weights", _dgrp_nw,
                                        {boost::num_edges(_nw)})), //only do this if directed
        _dset_opinion(this->create_dset("opinion", _dgrp_nw,
                                        {boost::num_vertices(_nw)}))
    {
        this->_log->debug("Constructing the Opinionet Model ...");

        // Initialize the network properties
        this->initialize_properties();

        this->_log->info(
            "Initialized network with {} vertices and {} edges. Directed: {}",
            boost::num_vertices(_nw),
            boost::num_edges(_nw),
            boost::is_directed(_nw)
        );

        // Mark the datasets as vertex properties and add dimensions and
        // coordinates.
        _dset_opinion->add_attribute("is_vertex_property", true);
        _dset_opinion->add_attribute("dim_name__1", "vertex_idx");
        _dset_opinion->add_attribute("coords_mode__vertex_idx", "trivial");

        if constexpr (rewire == Rewiring::RewiringOn) {
            save_graph(_nw, _dgrp_nw);
            this->_log->debug("Network saved.");
        }
        else {
            // Write the vertex data once, as it does not change with time
            auto _dset_vertices = _dgrp_nw->open_dataset(
                "_vertices", {boost::num_vertices(_nw)}
            );
            auto [v, v_end] = boost::vertices(_nw);
            _dset_vertices->write(
                v, v_end,
                [&](auto vd){
                    return boost::get(boost::vertex_index_t(), _nw, vd);
                }
            );
            _dset_vertices->add_attribute("dim_name__0", "vertex_idx");
            _dset_vertices->add_attribute(
                "coords_mode__vertex_idx", "trivial"
            );
        }
    }

private:

    // .. Setup functions .....................................................

    void initialize_properties() {
        this->_log->debug("Initializing the properties ...");

        // Continuous opinion space: draw opinions from a continuous interval
        if constexpr (opinion_space == continuous) {
            const std::pair<double, double> opinion_interval =
                get_as<std::pair<double, double>>(
                    "interval", this->_cfg["opinion_space"]
                );

            for (const auto v : range<IterateOver::vertices>(_nw)) {
                _nw[v].opinion = Utils::get_rand<double>(
                    opinion_interval, *this->_rng);
            }
        }
        // Discrete opinion space: draw opinions from a discrete set
        else {
            const int opinion_values =
                get_as<int>("num_opinions", this->_cfg["opinion_space"]);

            for (const auto v : range<IterateOver::vertices>(_nw)) {
                _nw[v].opinion =
                    Utils::get_rand<int>(opinion_values, *this->_rng);
            }
        }

        // For directed network: set the initial edge weights to 1/distance
        // in opinion space. In the undirected case, weights are not required.
        if constexpr (Utils::is_directed<NWType>()) {
            for (const auto v : range<IterateOver::vertices>(_nw)) {
                if (boost::out_degree(v, _nw) != 0) {
                    Utils::set_and_normalize_weights(v, _nw);
                }
            }
        }
    }

    NWType initialize_nw() {
        this->_log->debug("Creating the network ...");

        return Graph::create_graph<NWType>(this->_cfg["network"], *this->_rng);
    }

public:

    // .. Runtime functions ...................................................

    /** @brief Iterate a single step
     *  @detail Each step consists of ... TODO fill in
     */
    void perform_step ()
    {
        // TODO template for interaction function
        Revision::revision<interaction_type, opinion_space, rewire>(
            _nw,
            _susceptibility,
            _tolerance,
            _uniform_prob_distr,
            *this->_rng
        );
    }


    /// Monitor model information
    /** @detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small.
     */
    void monitor () {}


    /// Write data
    void write_data ()
    {
        // TODO Find out the best solution for writing data: Datamanager and
        //      deciders to configure in the run-cfg? Need flexible writing
        //      of edge data if topology changes?

        // Get the vertex iterators
        auto [v, v_end] = boost::vertices(_nw);

        // Write opinions
        _dset_opinion->write(
            v, v_end, [this](auto vd) { return _nw[vd].opinion; }
        );

        // save_vertex_properties(_nw, _dgrp_nw, std::to_string(this->get_time()),
        //     std::make_tuple(std::make_tuple("op", [](auto& vd, auto& _nw) { return _nw[vd].opinion; })));

        // auto [e, e_end] = boost::edges(_nw);
        //
        // // Write edge data
        // if constexpr (Utils::is_directed<NWType>()) {
        //     // Adaptor tuple that allows to save the edge data
        //     const auto get_edge_data_u = std::make_tuple(
        //         std::make_tuple("_edges", "type",
        //             std::make_tuple("source", [](auto& ed, auto& _nw) {
        //                         return boost::get(  boost::vertex_index_t(),
        //                                             _nw,
        //                                             boost::source(ed, _nw));}),
        //             std::make_tuple("target", [](auto& ed, auto& _nw) {
        //                         return boost::get(  boost::vertex_index_t(),
        //                                             _nw,
        //                                             boost::target(ed, _nw));}))
        //     );
        //
        //     // Save the edge data using the current time as label.
        //     save_edge_properties(
        //         _nw, _dgrp_nw, std::to_string(get_time()), get_edge_data_u
        //     );
        //
        //     _dset_edge_weights->write(
        //         e, e_end, [this](auto ed) { return _nw[ed].weight; }
        //     );
        // }
    }

    // .. Getters and setters .................................................
    // Add getters and setters here to interface with other model
};

} // namespace Utopia::Models::Opinionet

#endif // UTOPIA_MODELS_OPINIONET_HH
