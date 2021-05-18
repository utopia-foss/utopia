#ifndef UTOPIA_MODELS_OPINIONET_HH
#define UTOPIA_MODELS_OPINIONET_HH

#include <functional>
#include <boost/graph/random.hpp>

#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/types.hh>
#include <utopia/core/graph.hh>
#include <utopia/data_io/graph_utils.hh>

#include "modes.hh"
#include "revision.hh"


namespace Utopia::Models::Opinionet {

using Modes::Interaction_type;
using Modes::Opinion_space_type;
using Modes::Rewiring;

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
template<typename NWType=NetworkUndirected>
class Opinionet:
    public Model<Opinionet<NWType>, OpinionetTypes>
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

    /// Modes
    const Interaction_type _interaction;
    const Opinion_space_type _opinion_space;
    const Rewiring _rewire;

    /// Network and model dynamics parameters
    NWType _nw;
    const double _tolerance;
    const double _susceptibility;
    const double _weighting;

    /// A uniform probability distribution
    std::uniform_real_distribution<double> _uniform_prob_distr;

    // Datasets and Datagroups
    const std::shared_ptr<DataGroup> _dgrp_nw;
    const std::shared_ptr<DataSet> _dset_opinion;
    const std::shared_ptr<DataSet> _dset_edge_weights;

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
        // Initialize modes
        _interaction(this->initialize_interaction()),
        _opinion_space(this->initialize_opinion_space()),
        _rewire(this->initialize_rewiring()),
        // Initialize the network
        _nw(this->initialize_nw()),
        // Initialize the model parameters
        _tolerance(get_as<double>("tolerance", this->_cfg)),
        _susceptibility(get_as<double>("susceptibility", this->_cfg)),
        _weighting(get_as<double>(
            "weighting", this->_cfg["network"]["edges"])),
        _uniform_prob_distr(0., 1.),
        // Create datagroups and datasets
        _dgrp_nw(create_graph_group(_nw, this->_hdfgrp, "nw")),
        _dset_opinion(this->create_dset("opinion", _dgrp_nw,
                                        {boost::num_vertices(_nw)})),
        _dset_edge_weights(this->create_edge_weight_dset())
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

        if (_rewire == Rewiring::RewiringOff) {
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

    Interaction_type initialize_interaction() {
        if (get_as<std::string>("interaction_function", this->_cfg)
            == "Deffuant")
        {
            return Interaction_type::Deffuant;
        }
        else {
            return Interaction_type::HegselmannKrause;
        }
    }

    Opinion_space_type initialize_opinion_space() {
        if (get_as<std::string>("type", this->_cfg["opinion_space"])
            == "discrete")
        {
            return Opinion_space_type::discrete;
        }
        else {
            return Opinion_space_type::continuous;
        }
    }

    Rewiring initialize_rewiring() {
        if (get_as<bool>("rewiring", this->_cfg["network"]["edges"])) {
            return Rewiring::RewiringOn;
        }
        else {
            return Rewiring::RewiringOff;
        }
    }

    void initialize_properties() {
        this->_log->debug("Initializing the properties ...");

        // Continuous opinion space: draw opinions from a continuous interval
        if (_opinion_space == Opinion_space_type::continuous) {
            const std::pair<double, double> opinion_interval =
                get_as<std::pair<double, double>>(
                    "interval", this->_cfg["opinion_space"]
                );

            if (opinion_interval.first >= opinion_interval.second) {
              throw std::invalid_argument("Error: The given opinion interval"
              " is invalid! Specify an interval of the kind [a, b], "
              "with a<b!");
            }

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
                    Utils::get_rand<int>(
                        std::make_pair<int, int>(0, opinion_values-1),
                        *this->_rng
                    );
            }
        }

        // For directed network: Initialize weights depending on the opinion
        // distances. In the undirected case, weights are not required.
        if constexpr (Utils::is_directed<NWType>()) {
            for (const auto v : range<IterateOver::vertices>(_nw)) {
                if (boost::out_degree(v, _nw) != 0) {
                    Utils::set_and_normalize_weights(v, _nw, _weighting);
                }
            }
        }
    }

    NWType initialize_nw() const {
        this->_log->debug("Creating the network ...");

        auto g = Graph::create_graph<NWType>(this->_cfg["network"], *this->_rng);

        this->_log->debug("Network created.");

        return g;
    }

    // Only initialize edge weight dataset for directed graphs
    std::shared_ptr<DataSet> create_edge_weight_dset() {
        if constexpr (Utils::is_directed<NWType>()) {
            return this->create_dset(
                "edge_weights", _dgrp_nw, {boost::num_edges(_nw)}
            );
        }
        else {
            return 0;
        }
    }


public:

    // .. Runtime functions ...................................................

    /** @brief Iterate a single step
     *  @detail Each step consists of an opinion update and edge rewiring.
     *          Opinion update: Apply the interaction function to a randomly
     *              chosen vertex.
     *          Rewiring (if enabled): Rewire a random edge based on
     *              selective exposure.
     */
    void perform_step ()
    {
        Revision::revision(
            _nw,
            _susceptibility,
            _tolerance,
            _weighting,
            _interaction,
            _opinion_space,
            _rewire,
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
    void monitor () {
        double mean_opinion = 0.;
        double opinion_std = 0.;
        double min_opinion = std::numeric_limits<double>::infinity();
        double max_opinion = -std::numeric_limits<double>::infinity();

        double temp_op;
        for (const auto v : range<IterateOver::vertices>(_nw)) {
            temp_op = _nw[v].opinion;
            mean_opinion += temp_op;

            if (temp_op < min_opinion) {
                min_opinion = temp_op;
            }

            if (temp_op > max_opinion) {
                max_opinion = temp_op;
            }
        }

        mean_opinion /= boost::num_vertices(_nw);

        for (const auto v : range<IterateOver::vertices>(_nw)) {
            opinion_std += std::pow(_nw[v].opinion - mean_opinion, 2.);
        }

        opinion_std = std::sqrt(opinion_std / (boost::num_vertices(_nw) - 1.));

        this->_monitor.set_entry("mean_opinion", mean_opinion);
        this->_monitor.set_entry("opinion_std", opinion_std);
        this->_monitor.set_entry("min_opinion", min_opinion);
        this->_monitor.set_entry("max_opinion", max_opinion);
    }


    /// Write data
    void write_data ()
    {
        // Get the vertex iterators
        auto [v, v_end] = boost::vertices(_nw);

        // Write opinions
        _dset_opinion->write(
            v, v_end, [this](auto vd) { return _nw[vd].opinion; }
        );

        // Write edges
        if (_rewire == Rewiring::RewiringOn){
            // Adaptor tuple that allows to save the edge data
            const auto get_edge_data = std::make_tuple(
                std::make_tuple("_edges", "type",
                    std::make_tuple("source",
                        [](auto& ed, auto& _nw) {
                            return boost::get(
                                boost::vertex_index_t(), _nw,
                                boost::source(ed, _nw));
                        }
                    ),
                    std::make_tuple("target",
                        [](auto& ed, auto& _nw) {
                            return boost::get(
                                boost::vertex_index_t(), _nw,
                                boost::target(ed, _nw));
                        }
                    )
                )
            );
            // Save the edge data using the current time as label.
            save_edge_properties(
                _nw, _dgrp_nw, std::to_string(this->get_time()), get_edge_data
            );
        }

        // Write edge weights
        if constexpr (Utils::is_directed<NWType>()) {

            auto [e, e_end] = boost::edges(_nw);

            _dset_edge_weights->write(
                e, e_end, [this](auto ed) { return _nw[ed].weight; }
            );
        }
    }

    // .. Getters and setters .................................................
    // Add getters and setters here to interface with other model
};

} // namespace Utopia::Models::Opinionet

#endif // UTOPIA_MODELS_OPINIONET_HH
