#ifndef UTOPIA_DATAIO_DATA_MANAGER_WRITE_TASK_HH
#define UTOPIA_DATAIO_DATA_MANAGER_WRITE_TASK_HH

// metaprogramming
#include "../../core/metaprogramming.hh"
#include "../../core/utils.hh"

// for hdf5 support
#include "../hdfdataset.hh"
#include "../hdfgroup.hh"

namespace Utopia
{
namespace DataIO
{

/**
 *  \addtogroup DataManagerWriteTask WriteTask
 *  \{
 *  \ingroup DataManager
 */

/**
 * @page
 * \section what Overview
 * A writetask is an object which encapsulates the ability to acquire resources
 * for writing data, the functionality to write data from a given source,
 * and to handle metadata. This is geared towards use with HDF5 here.
 * \section impl Implementation
 * A writetask contains a function to build a basic hdfgroup, a function which
 * generates datasets in that group, and a function which writes data to these
 * datasets. Furthermore, functions for writing attributes to the basegroup
 * and the datasets are included.
 */

/**
 * @brief Encapsulate a task for writing data to a destination.
 *        Containes a callable 'writer' responisible for writing data to a held
 *        dataset. Contains a callabel 'build_dataset' which builds or opens a
 *        dataset for writing to in a held HDFGroup. A WriteTask is bound to a 
 *        group for its entire lifetime.
 * @tparam BGB Basegroup builder type, automatically determined
 * @tparam DW Data writer type, automatically determined
 * @tparam DB Dataset builder type, automatically deterimned
 * @tparam AWG Group attribute writer type, automatically determined
 * @tparam AWD Dataset attribute writer type, automatically determined
 */
template < class BGB, class DW, class DB, class AWG, class AWD >
struct WriteTask final // inheriting from this thing is useless, hence can be final
{
    using BasegroupBuilder       = BGB;
    using Writer                 = DW;
    using Builder                = DB;
    using AttributeWriterGroup   = AWG;
    using AttributeWriterDataset = AWD;

    /**
     * @brief Function building a base group
     */
    BasegroupBuilder build_basegroup;

    /**
     * @brief pointer to the hdfgroup in which all produced datasets live.
     */
    std::shared_ptr< HDFGroup > base_group;

    /**
     * @brief pointer to the dataset which is currently active
     */
    std::shared_ptr< HDFDataset> active_dataset;

    /**
     * @brief Callable to write data
     */
    Writer write_data;

    /**
     * @brief Callable to build new dataset
     */
    Builder build_dataset;

    /**
     * @brief Callable to write attributes to dataset; invoked after task write
     */
    AttributeWriterDataset write_attribute_active_dataset;

    /**
     * @brief Callabel to write attributes to base group, invoked after task
     * build
     */
    AttributeWriterGroup write_attribute_basegroup;

    /**
     * @brief Get the path of active dataset relative to the base_group
     *
     * @return std::string
     */
    std::string
    get_active_path()
    {
        if (active_dataset != nullptr)
        {
            return active_dataset->get_path();
        }
        else
        {
            return "";
        }
    }

    /**
     * @brief Get the path to the base group object
     *
     * @return std::string
     */
    std::string
    get_base_path()
    {
        if (base_group != nullptr)
        {
            return base_group->get_path();
        }
        else
        {
            return "";
        }
    }

    /**
     * @brief Swap the state of the caller with 'other'
     *
     * @param other Different WriteTask object
     */
    void
    swap(WriteTask& other)
    {
        if (this == &other)
        {
            return;
        }
        else
        {
            using std::swap;
            swap(build_basegroup, other.build_basegroup);
            swap(base_group, other.base_group);
            swap(active_dataset, other.active_dataset);
            swap(write_data, other.write_data);
            swap(build_dataset, other.build_dataset);
            swap(write_attribute_active_dataset,
                 other.write_attribute_active_dataset);
            swap(write_attribute_basegroup, other.write_attribute_basegroup);
        }
    }

    /**
     * @brief Construct a new Write Task object.
     *
     * @tparam Basegroupbuilder Automatically determined.
     * @tparam Writertype Automatically determined.
     * @tparam Buildertype Automatically determined.
     * @tparam AWritertypeG Automatically determined.
     * @tparam AWritertypeD Automatically determined.
     * @param bgb builder function for basegroup
     * @param w   writer function for wrtiing data
     * @param b builder function for dataset
     * @param ag group attribute writer
     * @param ad dataset attribute writer
     */
    template < typename Basegroupbuilder,
               typename Writertype,
               typename Buildertype,
               typename AWritertypeG,
               typename AWritertypeD >
    WriteTask(Basegroupbuilder&& bgb,
              Writertype&&       w,
              Buildertype&&      b,
              AWritertypeG&&     ag,
              AWritertypeD&&     ad) :
        build_basegroup(bgb),
        base_group(nullptr), active_dataset(nullptr), write_data(w),
        build_dataset(b), write_attribute_active_dataset(ad),
        write_attribute_basegroup(ag)
    {
    }

    /**
     * @brief Construct a new writer Task object
     */
    WriteTask() = default;

    /**
     * @brief
     *
     *
     * @param other WriteTask object to copy from
     */
    WriteTask(const WriteTask& other) = default;

    /**
     * @brief Construct a new writer Task object
     *
     * @param other WriteTask to move from
     */
    WriteTask(WriteTask&& other) = default;

    /**
     * @brief Copy assign caller from 'other'
     *
     * @param other Object to assign from
     * @return WriteTask&
     */
    WriteTask&
    operator=(const WriteTask& other) = default;

    /**
     * @brief Move assign from 'other'
     *
     * @param other Object to assign from.
     * @return WriteTask&
     */
    WriteTask&
    operator=(WriteTask&& other) = default;

    /**
     * @brief Destroy the writer Task object
     */
    ~WriteTask() = default;
};

/**
 * @brief Swaps the state of lhs and rhs
 *
 * @tparam Writer Type of writer member of the WriteTaskcl
 * @tparam Builder Type of build_dataset member of the WriteTask
 * @param lhs
 * @param rhs
 */
template < class BGB, class DW, class DB, class AWG, class AWD >
void
swap(WriteTask< BGB, DW, DB, AWG, AWD >& lhs,
     WriteTask< BGB, DW, DB, AWG, AWD >& rhs)
{
    lhs.swap(rhs);
}

// Deduction guide for WriteTask
template < typename Basegroupbuilder,
           typename Writertype,
           typename Buildertype,
           typename AWritertypeG,
           typename AWritertypeD >
WriteTask(Basegroupbuilder&& bgb,
          Writertype&&       w,
          Buildertype&&      b,
          AWritertypeG&&     ad,
          AWritertypeD&&     ag)
    ->WriteTask< std::decay_t< Basegroupbuilder >,
                 std::decay_t< Writertype >,
                 std::decay_t< Buildertype >,
                 std::decay_t< AWritertypeG >,
                 std::decay_t< AWritertypeD > >;

/**
 *  \}  // end of DataManagerWriteTask
 */

} // namespace DataIO
} // namespace Utopia

#endif
