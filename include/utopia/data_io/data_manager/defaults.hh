#ifndef UTOPIA_DATAIO_DATA_MANAGER_DEFAULTS_HH
#define UTOPIA_DATAIO_DATA_MANAGER_DEFAULTS_HH

#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/zip.hpp>

#include "../cfg_utils.hh"
#include "write_task.hh"

namespace Utopia
{
namespace DataIO
{
namespace Default
{

/**
 *  \addtogroup DataManagerDefaults Defaults
 *  \{
 *  \ingroup DataManager
 *  @details Here, default implementations for common deciders, triggers and
 *          write tasks for the ::DataManager are defined, as well as a
 *          default execution process which is to run the actual writer tasks.
 */

// =============================================================================
// ================================ Writer Tasks ===============================
// =============================================================================

/// Type of the default group builder
using DefaultBaseGroupBuilder =
    std::function< std::shared_ptr< HDFGroup >(std::shared_ptr< HDFGroup >&&) >;

/// Type of the default data writer
template < typename Model >
using DefaultDataWriter =
    std::function< void(std::shared_ptr< HDFDataset< HDFGroup > >&, Model&) >;

/// Type of the default dataset builder
template < typename Model >
using DefaultBuilder = std::function< std::shared_ptr< HDFDataset< HDFGroup > >(
    std::shared_ptr< HDFGroup >&, Model&) >;

/// Type of the default attribute writer for groups
template < typename Model >
using DefaultAttributeWriterGroup =
    std::function< void(std::shared_ptr< HDFGroup >&, Model&) >;

/// Type of the default attribute writer for datasets
template < typename Model >
using DefaultAttributeWriterDataset =
    std::function< void(std::shared_ptr< HDFDataset< HDFGroup > >&, Model&) >;

/**
 * @brief  A default WriteTask definition
 *
 * @details This uses the other ``Default``-prefixed builders and writers from
 *         this namespace.
 *
 * @tparam Model The type of the model that is made available to each callable
 */
template < typename Model >
using DefaultWriteTask = WriteTask< DefaultBaseGroupBuilder,
                                    DefaultDataWriter< Model >,
                                    DefaultBuilder< Model >,
                                    DefaultAttributeWriterGroup< Model >,
                                    DefaultAttributeWriterDataset< Model > >;

// =============================================================================
// ================================ Execution Process ==========================
// =============================================================================

/**
 * @brief Functor representing what is considered the most widely used
 *        execution process for writing data.
 *
 * @details First runs over all triggers, and checks whether new datasets need
 *         to be built. If yes, the builder in the respective dataset is
 *         called.
 */
struct DefaultExecutionProcess
{
    /**
     * @brief Call operator for executing the execution process
     *
     * @tparam Datamanager The DataManager; automatically determined
     * @tparam Model       The Model; automatically determined
     *
     * @param dm Reference to a DataManager instance
     * @param m  Reference to a model instance
     */
    template < class Datamanager, class Model >
    void
    operator()(Datamanager& dm, Model& m)
    {
        auto& tasks = dm.get_tasks();
        for (auto& taskpair : tasks)
        {
            // TODO Instead of nullptr-check, check boolean?!
            if (taskpair.second->base_group == nullptr)
            {
                taskpair.second->base_group =
                    taskpair.second->build_basegroup(m.get_hdfgrp());
            }
        }

        // FIXME: make this such that no dataset is built
        // when the deciders do not write!
        for (auto& [name, trigger] : dm.get_triggers())
        {
            if ((*trigger)(m))
            {
                for (auto& taskname : dm.get_trigger_task_map()[name])
                {
                    tasks[taskname]->active_dataset =
                        tasks[taskname]->build_dataset(
                            tasks[taskname]->base_group, m);

                    if (tasks[taskname]->write_attribute_basegroup)
                    {
                        tasks[taskname]->write_attribute_basegroup(
                            tasks[taskname]->base_group, m);
                    }
                }
            }
        }

        for (auto& [name, decider] : dm.get_deciders())
        {
            if ((*decider)(m))
            {
                for (auto& taskname : dm.get_decider_task_map()[name])
                {
                    tasks[taskname]->write_data(tasks[taskname]->active_dataset,
                                                m);
                    if (tasks[taskname]->write_attribute_active_dataset)
                    {
                        tasks[taskname]->write_attribute_active_dataset(
                            tasks[taskname]->active_dataset, m);
                    }
                }
            }
        }
    }
};

// =============================================================================
// ================================ Decider ====================================
// =============================================================================

/// The base class for deciders
template < typename Model >
struct DefaultDecider
{

    /// A functor called in operator(), can be used for specialization
    std::function< bool(Model&) > func;

    /// Invokes the boolean functor this decider is specialized with
    virtual bool
    operator()(Model& m)
    {
        return func(m);
    }

    DefaultDecider()                      = default;
    DefaultDecider(const DefaultDecider&) = default;
    DefaultDecider(DefaultDecider&&)      = default;
    DefaultDecider&
    operator=(const DefaultDecider&) = default;
    DefaultDecider&
    operator=(DefaultDecider&&) = default;

    /// Construct the decider from a configuration
    DefaultDecider(const Config&) : DefaultDecider()
    {
    }

    /// Construct the decider with a given functor
    DefaultDecider(std::function< bool(Model&) > f) : func(f)
    {
    }

    virtual ~DefaultDecider() = default;
};

/// A decider that returns true when within certain time intervals
template < typename Model >
struct IntervalDecider : DefaultDecider< Model >
{
    /// The base class
    using Base = DefaultDecider< Model >;

    /// The sequence of intervals within to return true
    std::list< std::array< std::size_t, 2 > > intervals;

    /**
     * @brief Construct a new IntervalDecider object from a configuration
     *
     * @param cfg  The configuration to extract information from. Expects as
     *             arguments a key ``intervals`` which is a sequence of pairs,
     *             where each pair determines the half-open [start, end)
     *             interval in which writing is to occur.
     */
    IntervalDecider(const Config& cfg) :
        intervals(get_as< std::list< std::array< std::size_t, 2 > > >(
            "intervals", cfg))
    {
        // Set the functor that evaluates the intervals given the time
        this->func = [this](Model& m) -> bool {
            if (intervals.size() != 0)
            {
                // Check if within [start, end) interval
                if (m.get_time() >= intervals.front()[0] and
                    m.get_time() < intervals.front()[1])
                {
                    return true;
                }

                // Are at the end of the current interval; pop it, such that
                // at next invocation the front is the new interval
                if (m.get_time() == intervals.front()[1])
                {
                    intervals.pop_front();
                }
            }
            return false;
        };
    }

    IntervalDecider()                       = default;
    IntervalDecider(const IntervalDecider&) = default;
    IntervalDecider(IntervalDecider&&)      = default;
    IntervalDecider&
    operator=(const IntervalDecider&) = default;
    IntervalDecider&
    operator=(IntervalDecider&&) = default;
    virtual ~IntervalDecider()   = default;
};

/**
 * @brief Represents a decider which decides to write according to a slice
 *        definition
 *
 */
template < typename Model >
struct SliceDecider : DefaultDecider< Model >
{
    /// The type of the base decider class
    using Base = DefaultDecider< Model >;

    /// Construct a SliceDecider with a given configuration
    SliceDecider(const Config& cfg) :
        // Set up using base class, passing a functor to it
        Base([&cfg]() {
            const auto start = get_as< std::size_t >("start", cfg);
            const auto stop  = get_as< std::size_t >("stop", cfg);
            const auto step  = get_as< std::size_t >("step", cfg);

            // Create and return the functor that evaluates these slices
            return [start, stop, step](Model& m) -> bool {
                return ((m.get_time() >= start) and (m.get_time() < stop) and
                        ((m.get_time() - start) % step == 0));
            };
        }())
    {
    }

    SliceDecider()                    = default;
    SliceDecider(const SliceDecider&) = default;
    SliceDecider(SliceDecider&&)      = default;
    SliceDecider&
    operator=(const SliceDecider&) = default;
    SliceDecider&
    operator=(SliceDecider&&) = default;
    virtual ~SliceDecider()   = default;
};

/**
 * @brief Decider which only returns true at a certain time
 */
template < typename Model >
struct OnceDecider : DefaultDecider< Model >
{
    /// The type of the base decider class
    using Base = DefaultDecider< Model >;

    /// Construct a OnceDecider from a configuration. Expects key ``time``.
    OnceDecider(const Config& cfg) :
        // Set up using base class, passing a functor to it
        Base([&cfg]() {
            const auto time = get_as< std::size_t >("time", cfg);

            // Create and return the functor
            return [time](Model& m) -> bool { return (m.get_time() == time); };
        }())
    {
    }

    /// Construct a OnceDecider that evaluates to true at time zero
    OnceDecider() :
        Base([]() {
            return [](Model& m) -> bool { return (m.get_time() == 0); };
        }())
    {
    }

    OnceDecider(const OnceDecider&) = default;
    OnceDecider(OnceDecider&&)      = default;
    OnceDecider&
    operator=(const OnceDecider&) = default;
    OnceDecider&
    operator=(OnceDecider&&) = default;
    virtual ~OnceDecider()   = default;
};

/**
 * @brief Decider which always returns true
 */
template < typename Model >
struct AlwaysDecider : DefaultDecider< Model >
{
    /// The type of the base decider class
    using Base = DefaultDecider< Model >;

    AlwaysDecider() : Base([](Model&) -> bool { return true; })
    {
    }

    AlwaysDecider(const Config&) : Base([](Model&) -> bool { return true; })
    {
    }

    AlwaysDecider(const AlwaysDecider&) = default;
    AlwaysDecider(AlwaysDecider&&)      = default;
    AlwaysDecider&
    operator=(const AlwaysDecider&) = default;
    AlwaysDecider&
    operator=(AlwaysDecider&&) = default;
    virtual ~AlwaysDecider()   = default;
};

/**
 * @brief Combines a number of deciders; returns true if any of them is true
 *
 * @tparam Model     The model
 * @tparam Deciders  The decider types to combine as composite
 */
template < typename Model, typename... Deciders >
struct CompositeDecider : DefaultDecider< Model >
{
    /// The type of the base decider class
    using Base = DefaultDecider< Model >;

    /// Tuple of associated decider objects
    std::tuple< Deciders... > held_deciders;

    /**
     * @brief Evaluates the composite deciders; returns true if any is true
     *
     * @param m The model reference, passed on to decider objects
     */
    virtual bool
    operator()(Model& m) override
    {
        return boost::hana::fold(
            held_deciders, false, [&m](bool res, auto&& decider) {
                return (res or decider(m));
            });
    }

    /// Set up a composite decider from a configuration
    CompositeDecider(const Config& cfg)
    {
        // BAD: User has to be smart about it, because it breaks
        // if args are not the same order as Deciders...
        std::array< const Config&, sizeof...(Deciders) > configs;

        // make the args node for each decider into an array
        auto conf_it = configs.begin();

        for (auto it = cfg.begin(); it != cfg.end(); ++it, ++conf_it)
        {
            *conf_it = it->second["args"];
        }

        // then iterate over the zipped confs and deciders and built the
        // deciders with their respective config
        boost::hana::for_each(boost::hana::zip(configs, held_deciders),
                              [](auto& conf_dcd_pair) {
                                  auto& [c, d] = conf_dcd_pair;
                                  d            = std::decay_t< decltype(d) >(c);
                              });
    }

    CompositeDecider()                        = default;
    CompositeDecider(const CompositeDecider&) = default;
    CompositeDecider&
    operator=(const CompositeDecider&) = default;
    CompositeDecider&
    operator=(CompositeDecider&&) = default;
    virtual ~CompositeDecider()   = default;
};

/**
 * @brief A composite of the slice and the interval deciders
 *
 * @tparam Model
 *
 * @TODO Make it configurable to allow AND-connected operator()
 */
template < typename Model >
struct SimpleCompositeDecider : DefaultDecider< Model >
{
    /// The type of the base decider class
    using Base = DefaultDecider< Model >;

    /// The slice decider
    SliceDecider< Model > slice_decider;

    /// The interval decider
    IntervalDecider< Model > interval_decider;

    /// Evaluates to true if the slice OR interval decider evalute to true
    virtual bool
    operator()(Model& m) override
    {
        return slice_decider(m) or interval_decider(m);
    }

    /// Set up a SimpleCompositeDecider from configuration node.
    /** @details Expects keys ``slice`` and ``interval`` to pass to the
     *         respective deciders
     */
    SimpleCompositeDecider(const Config& cfg) :
        slice_decider(SliceDecider< Model >(cfg["slice"])),
        interval_decider(IntervalDecider< Model >(cfg["interval"]))
    {
    }

    SimpleCompositeDecider()                              = default;
    SimpleCompositeDecider(const SimpleCompositeDecider&) = default;
    SimpleCompositeDecider&
    operator=(const SimpleCompositeDecider&) = default;
    SimpleCompositeDecider&
    operator=(SimpleCompositeDecider&&) = default;
    virtual ~SimpleCompositeDecider()   = default;
};

// =============================================================================
// ================================ Triggers ===================================
// =============================================================================

/// The function to decide whether a writer's builder will be triggered -
/// default signature
/// These are only aliases for the deciders to avoid copy pasta.
/// Keep this in mind if messing with types!
template < typename Model >
using DefaultTrigger = DefaultDecider< Model >;

template < typename Model >
using SliceTrigger = SliceDecider< Model >;

template < typename Model >
using IntervalTrigger = IntervalDecider< Model >;

template < typename Model >
using BuildOnceTrigger = OnceDecider< Model >;

template < typename Model >
using BuildAlwaysTrigger = AlwaysDecider< Model >;

template < typename Model, typename... Deciders >
using CompositeTrigger = CompositeDecider< Model, Deciders... >;

template < typename Model >
using SimpleCompositeTrigger = SimpleCompositeDecider< Model >;

// =============================================================================
// =========================== Default type maps  ==============================
// =============================================================================

/// Default decider types
template < typename Model >
auto default_decidertypes = std::make_tuple(
    std::make_pair("default", DefaultDecider< Model >()),
    std::make_pair("always", AlwaysDecider< Model >()),
    std::make_pair("once", OnceDecider< Model >()),
    std::make_pair("interval", IntervalDecider< Model >()),
    std::make_pair("slice", SliceDecider< Model >()),
    std::make_pair("simple_composite", SimpleCompositeDecider< Model >()));

/// Default trigger types
template < typename Model >
auto default_triggertypes = default_decidertypes< Model >;

/**
 *  \}  // endgroup DataManager::Defaults
 */

} // namespace Default
} // namespace DataIO
} // namespace Utopia

#endif
