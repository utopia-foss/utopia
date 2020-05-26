#ifndef UTOPIA_DATAIO_DATA_MANAGER_DEFAULTS_HH
#define UTOPIA_DATAIO_DATA_MANAGER_DEFAULTS_HH

#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/zip.hpp>
#include <unordered_map>

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
    std::function< void(std::shared_ptr< HDFDataset >&, Model&) >;

/// Type of the default dataset builder
template < typename Model >
using DefaultBuilder = std::function< std::shared_ptr< HDFDataset >(
    std::shared_ptr< HDFGroup >&, Model&) >;

/// Type of the default attribute writer for groups
template < typename Model >
using DefaultAttributeWriterGroup =
    std::function< void(std::shared_ptr< HDFGroup >&, Model&) >;

/// Type of the default attribute writer for datasets
template < typename Model >
using DefaultAttributeWriterDataset =
    std::function< void(std::shared_ptr< HDFDataset >&, Model&) >;

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
        for (auto& [name, trigger] : dm.get_triggers())
        {
            if ((*trigger)(m))
            {
                for (auto& taskname : dm.get_trigger_task_map()[name])
                {
                    // using this ref variable avoids recomputing 
                    // 'taskname''s hash all the time, should give minor 
                    // performance improvement
                    auto& task = tasks[taskname];

                    task->active_dataset =
                        task->build_dataset(
                            task->base_group, m);

                    if (task->write_attribute_basegroup)
                    {
                        task->write_attribute_basegroup(
                            task->base_group, m);
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
                    auto& task = tasks[taskname];

                    task->write_data(task->active_dataset,
                                                m);
                    if (task->write_attribute_active_dataset)
                    {
                        task->write_attribute_active_dataset(
                            task->active_dataset, m);
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

    /// Invokes the boolean functor this decider is specialized with
    virtual bool
    operator()(Model&)
    {
        return false;
    }

    /**
     * @brief Set the decider up from a given config node
     *
     * @param cfg config node containing arguments for this decider
     */
    virtual void
    set_from_cfg(const Config&)
    {
    }

    DefaultDecider()                      = default;
    DefaultDecider(const DefaultDecider&) = default;
    DefaultDecider(DefaultDecider&&)      = default;
    DefaultDecider&
    operator=(const DefaultDecider&) = default;
    DefaultDecider&
    operator=(DefaultDecider&&) = default;

    virtual ~DefaultDecider() = default;
};

/// A decider that returns true when within certain time intervals
/** Every interval is of shape `[start, stop), stride` where the third argument
 *  is optional and defines a stepping size.
 */
template < typename Model >
struct IntervalDecider : DefaultDecider< Model >
{

    /// The base class
    using Base = DefaultDecider< Model >;

    /// The sequence of intervals within to return true
    std::list< std::array< std::size_t, 3 > > intervals;

    virtual bool
    operator()(Model& m) override
    {
        // Are at the end of the current interval; pop it, such that
        // at next invocation the front is the new interval
        if (intervals.size() != 0 and m.get_time() == intervals.front()[1])
        {
            intervals.pop_front();
        }

        if (intervals.size() != 0)
        {
            const auto [start, stop, step] = intervals.front();
            // Check if within [start, end) interval
            if ((m.get_time() >= start) and (m.get_time() < stop) and
                ((m.get_time() - start) % step == 0))
            {
                return true;
            }
        }
        return false;
    }
    /**
     * @brief Set the decider up from a given config node
     *
     * @param cfg config node containing arguments for this decider
     */
    virtual void
    set_from_cfg(const Config& cfg) override
    {
        auto tmp = get_as< std::list< std::vector< std::size_t > > >(
            "intervals", cfg);
        for (std::vector< std::size_t > tmp_interval : tmp)
        {
            if (tmp_interval.size() == 2)
            {
                tmp_interval.push_back(1);
            }
            else if (tmp_interval.size() != 3)
            {
                throw Utopia::KeyError("intervals", cfg,
                    fmt::format("Array of unexpected length {}! Expected array "
                        "of length 2 or 3 [start, stop, step] with step "
                        "optional (default 1)."));
            }

            std::array< std::size_t, 3 > interval;
            std::copy(tmp_interval.begin(), tmp_interval.end(),
                      interval.begin());
            intervals.push_back(interval);
        }
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
 * @brief Decider which only returns true at a certain time
 */
template < typename Model >
struct OnceDecider : DefaultDecider< Model >
{
    /// The type of the base decider class
    using Base = DefaultDecider< Model >;
    std::size_t time;

    virtual bool
    operator()(Model& m) override
    {
        return (m.get_time() == time);
    }

    /**
     * @brief Set the decider up from a given config node
     *
     * @param cfg config node containing arguments for this decider
     */
    virtual void
    set_from_cfg(const Config& cfg) override
    {
        time = get_as< std::size_t >("time", cfg);
    }

    /// Construct a OnceDecider that evaluates to true at time zero
    OnceDecider() = default;

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

    virtual bool
    operator()(Model&) override
    {
        return true;
    }

    AlwaysDecider() = default;

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

    /**
     * @brief Set the decider up from a given config node
     * @warning The ordering of he decider nodes in the config needs to be the
     *          same as the ordering of the arguments given to the constructor
     *          of this class
     * @param cfg config node containing arguments for this decider
     */
    virtual void
    set_from_cfg(const Config& cfg) override
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
                              [](auto&& conf_dcd_pair) {
                                  auto&& [c, d] = conf_dcd_pair;
                                  d.set_from_cfg(c);
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

// =============================================================================
// =========================== Default type maps  ==============================
// =============================================================================

/**
 * @brief Map that names the deciders supplied by default such that they can be 
 *        addressed in a config file. 
 * @details This map does not provide decider objects
 *          or pointers to them in itself, but functions which create 
 *          shared_pointers to a particular decider function. This is made such 
 *          that we can use dynamic polymorphism and do not have to resort to 
 *          tuples.
 * 
 * @tparam Model A model type for which the deciders shall be employed.
 */
template < typename Model >
static std::unordered_map<
    std::string,
    std::function< std::shared_ptr< DefaultDecider< Model > >() > >
    default_decidertypes{
        { std::string("default"),
          []() { return std::make_shared< DefaultDecider< Model > >(); } },
        { std::string("always"),
          []() { return std::make_shared< AlwaysDecider< Model > >(); } },
        { std::string("once"),
          []() { return std::make_shared< OnceDecider< Model > >(); } },
        { std::string("interval"),
          []() { return std::make_shared< IntervalDecider< Model > >(); } }
    };

// =============================================================================
// ================================ Triggers
// ===================================
// =============================================================================

/// The function to decide whether a writer's builder will be triggered -
/// default signature
/// These are only aliases for the deciders to avoid copy pasta.
/// Keep this in mind if messing with types!
template < typename Model >
using DefaultTrigger = DefaultDecider< Model >;

template < typename Model >
using IntervalTrigger = IntervalDecider< Model >;

template < typename Model >
using BuildOnceTrigger = OnceDecider< Model >;

template < typename Model >
using BuildAlwaysTrigger = AlwaysDecider< Model >;

template < typename Model, typename... Deciders >
using CompositeTrigger = CompositeDecider< Model, Deciders... >;

/**
 * @brief Default trigger factories. Equal to deciders because while the 
 *        task they fullfill is different, their functionality is not.
 * @tparam Model Modeltype the triggers shall be used with
 */
template < typename Model >
auto default_triggertypes = default_decidertypes< Model >;

/**
 *  \}  // endgroup DataManager::Defaults
 */

} // namespace Default
} // namespace DataIO
} // namespace Utopia

#endif
