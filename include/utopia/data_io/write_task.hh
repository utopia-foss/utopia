#ifndef UTOPIA_DATAIO_WRITE_TASK_HH
#define UTOPIA_DATAIO_WRITE_TASK_HH

// for hdf5 support
#include "hdfdataset.hh"
#include "hdfgroup.hh"

// metaprogramming
#include "../core/compiletime_algos.hh"
#include "../core/utils.hh"

namespace Utopia
{
namespace DataIO
{

/**
 * @brief Encapsulate a task for writing data to a destination.
 *        Containes a callable 'writer' responisible for writing data to a held
 *        dataset. Contains a callabel 'build_dataset' which builds or opens a dataset
 *        for writing to in a held HDFGroup. A WriteTask is bound to a group
 *        for its entire lifetime.
 * @tparam W automatically determined
 * @tparam B automatically determined
 */
template <class W, class B, class A>
struct WriteTask
{
    using DataWriter = W;
    using Builder = B;
    using AttributeWriter = A;

    /**
     * @brief pointer to the hdfgroup in which all produced datasets live.
     *
     */
    std::shared_ptr<HDFGroup> base_group;

    /**
     * @brief pointer to the dataset which is currently active
     *
     */
    std::shared_ptr<HDFDataset<HDFGroup>> active_dataset;
    
    /**
     * @brief Callable to write data
     */
    DataWriter write_data;

    /**
     * @brief Callable to build new dataset
     *
     */
    Builder build_dataset;

    /**
     * @brief Callable to write attributes to dataset; invoked after builder
     * 
     */
    AttributeWriter write_attribute;

    /**
     * @brief Get the path of the base_group
     *
     * @return std::string
     */
    std::string get_base_path()
    {
        return base_group->get_path();
    }

    /**
     * @brief Get the path of active dataset relative to the base_group
     *
     * @return std::string
     */
    std::string get_active_path()
    {
        return active_dataset->get_path();
    }

    /**
     * @brief Swap the state of the caller with 'other'
     *
     * @param other Different WriteTask object
     */
    void swap(WriteTask& other)
    {
        if (this == &other)
        {
            return;
        }
        else
        {
            using std::swap;

            swap(base_group, other.base_group);
            swap(active_dataset, other.active_dataset);
            swap(write_data, other.write_data);
            swap(build_dataset, other.build_dataset);
            swap(write_attribute, other.write_attribute);
        }
    }

    /**
     * @brief Construct a new WriteTask object, which bundles a data writing
     *        procedure: build a dataset, write attributes, write data ...
     *
     * @tparam W automatically determined
     * @tparam B automatically determined
     * @tparam Builderargs automatically determined
     *
     * @param group         The group to produce datasets in
     * @param path_to_dset  The path to the new dataset
     * @param w   The callable taking care of writing data; signature has to be
     *            write(dataset_to_writer_to, arbitrary...)
     * @param b   The callable taking care of building a new dataset; the
     *            signature has to be
     *            build_dataset(a group, path_of_new_dataset, arbitrary....)
     * @param bargs  Additional arguments to call 'build' with
     */
    template <typename Writertype,
              typename Buildertype,
              typename AWritertype,
              typename... Builderargs>
    WriteTask(std::shared_ptr<HDFGroup> group,
              std::string path_to_dset,
              Writertype&& w,
              Buildertype&& b,
              AWritertype&& a,
              Builderargs&&... bargs)
        : base_group(group),
          write_data(w),
          build_dataset(b),
          write_attribute(a)
    {
        active_dataset = build_dataset(base_group, path_to_dset,
                                       std::forward<Builderargs>(bargs)...);
    }

    /**
     * @brief Construct a new writer Task object
     *
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
    WriteTask& operator=(const WriteTask& other) = default;

    /**
     * @brief Move assign from 'other'
     *
     * @param other Object to assign from.
     * @return WriteTask&
     */
    WriteTask& operator=(WriteTask&& other) = default;

    /**
     * @brief Destroy the writer Task object
     *
     */
    virtual ~WriteTask() = default;
};

/**
 * @brief Swaps the state of lhs and rhs
 *
 * @tparam Writer Type of writer member of the WriteTask
 * @tparam Builder Type of build_dataset member of the WriteTask
 * @param lhs
 * @param rhs
 */
template <typename DataWriter, typename Builder, typename AWritertype>
void swap(WriteTask<DataWriter, Builder, AWritertype>& lhs, WriteTask<DataWriter, Builder, AWritertype>& rhs)
{
    lhs.swap(rhs);
}

// Deduction guide for WriteTask
template <typename Writertype,
          typename Buildertype,
          typename AWritertype,
          typename... Builderargs>
WriteTask(std::shared_ptr<HDFGroup> group,
          std::string path_to_dset,
          Writertype&& w,
          Buildertype&& b,
          AWritertype&& a,
          Builderargs&&... bargs)
    -> WriteTask<Writertype, Buildertype, AWritertype>;

} // namespace DataIO
} // namespace Utopia

#endif
