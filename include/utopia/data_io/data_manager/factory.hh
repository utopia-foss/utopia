#ifndef UTOPIA_DATAIO_FACTORY_HH
#define UTOPIA_DATAIO_FACTORY_HH

// stl includes for having shared_ptr, hashmap, vector, swap
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

// boost includes
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/remove_at.hpp>
#include <boost/hana/transform.hpp>

// utopia includes
#include "../../core/logging.hh"
#include "../../core/type_traits.hh"
#include "../cfg_utils.hh"
#include "data_manager.hh"
#include "defaults.hh"
#include "utils.hh"

namespace Utopia
{
namespace DataIO
{

/**
 *  \addtogroup DataManagerFactories Factories
 *  \{
 *  \ingroup DataManager
 */

/**
 * @page DataManagerFactories Factories module
 *  \section what Overview
 *  This file provides a functor for producing default writetasks from complete
 *  or simplified arguments and another functor for producing a default
 * datamanager. \section impl Implementation The writetask factory is
 * implemented as class because its encapsulation together with its helper
 * functions is natural. It takes care of building writetasks from simplified
 * user supplied arguments, and is build to be extendible. It distinguishes
 * between the use of these simplified arguments and the usage of the full
 * argument list needed to invoke the writetask constructor directly. The
 * datamanager factory is much simpler in its logic, and hences is implemented
 * as a template function.
 *
 */

/**
 * @brief Descriptor for a dataset. Contains:
 *        path: string giving the path of the dataset in its group or file
 *        with_time_postfix: whether the current model time is appended to the
 * dataset path dataset_capacity: vector giving capacity of the dataset per
 * dimension dataset_chunksize: vector giving chunksize per dimension of the
 * dataset dataset_compression: integer giving compression strength (0 to 10)
 */
struct DatasetDescriptor
{
    std::string            path;
    std::vector< hsize_t > dataset_capacity    = {};
    std::vector< hsize_t > dataset_chunksize   = {};
    int                    dataset_compression = 0;
};

/**
 * @brief TypeTag enumerates the kind of access which is used to write data.
 *        It became necessary after integrating graphs.
 *        Has 5 values:
 *        plain             : use for everything that is not a graph
 *        vertex_property   : for writing graphs using boost::vertex_property
 *        edge_property     : for writing graphs using boost::edge_property
 *        vertex_descriptor : for writing graphs using boost::vertex_descriptor
 *        edge_descriptor   : for writing graphs using boost::edge_descriptor
 *
 */
enum struct TypeTag
{
    plain,
    vertex_property,
    edge_property,
    vertex_descriptor,
    edge_descriptor
};

// Making the factory a functor allows for the separation of template parameters
// which are user defined (model, graphtag) and automatically determined ones,
// which are given to the call operator.

/**
 * @brief Functor for building a writetask from arguments.
 *
 * @tparam Model Model type.
 * @tparam TypeTag::plain Typetag, indicates what type of data source is used,
 *         has only to be changed when using graphs.
 */
template < class Model, TypeTag typetag = TypeTag::plain >
class TaskFactory
{
  private:
    static std::unordered_map<
        std::string,
        std::function< std::string(std::string, Model&) > >
        _modifiers;

    /**
     * @brief Function which produces functions for writing attributes to a
     *        dataset
     *        or group, and which are used by a writer task to write attributes
     *        to the dataset and basegroup it creates.
     *
     * @tparam Func Function type to produce
     * @tparam Attribute automatically determined

     * @param attr Either 'Nothing' if no attributes shall be written, or a
     *             tuple/pair (name, attribute_data) or a function convertible
     *             to type Func which takes care of writing  dataset
     *             attributes, and receives a reference to a model as argument
     *
     * @return Func Function which writes attributes as constructed from
     *              arguments.
     */
    template < class Func, class AttributeHandle >
    Func
    _make_attribute_writer(AttributeHandle&& attr)
    {
        using AttrType = std::decay_t< AttributeHandle >;
        Func writer{};

        if constexpr (std::is_same_v< AttrType, Nothing >)
        {
            // do nothing here, because nothing shall be done ;)
        }
        else if constexpr (Utopia::Utils::has_static_size_v< AttrType >)
        {

            using std::get;
            using Nametype =
                std::decay_t< std::tuple_element_t< 0, AttrType > >;

            // if the first thing held by the Dataset_attribute tuplelike is
            // not convertible to stirng, and hence cannot be used to name
            // the thing, error is thrown.
            static_assert(
                Utils::is_string_v< Nametype >,
                "Error, first entry of Dataset_attribute must be a string "
                "naming the attribute");

            // check if the $ indicating string interpolation like behavior
            // is found. If yes, invoke path modifier, else leave as is
            std::string name = get< 0 >(attr);
            auto        pos  = name.find('$'); // find indicator

            if (pos != std::string::npos)
            {
                auto        path_builder = _modifiers[name.substr(pos + 1)];
                std::string new_path     = name.substr(0, pos);

                writer = [attr, path_builder, new_path](auto&& hdfobject,
                                                        auto&& m) -> void {
                    hdfobject->add_attribute(path_builder(new_path, m),
                                             get< 1 >(attr));
                };
            }
            else
            {
                writer = [attr](auto&& hdfobject, auto &&) -> void {
                    hdfobject->add_attribute(get< 0 >(attr),
                                             get< 1 >(attr));
                };
            }
        }
        else
        {
            static_assert(
                Utils::is_callable_v< AttrType >,
                "Error, if the given attribute argument is not a tuple/pair "
                "and not 'Nothing', it has to be a function");

            writer = attr;
        }

        return writer;
    }

    /**
     * @brief Function producing a dataset builder function of type
     *        Default::DefaultBuilder<Model>, which is responsible for
     *        creating new HDF5 datasets on request.
     *
     * @param dataset_descriptor Describes dataset properties.
     * @return auto Builder object
     */
    Default::DefaultBuilder< Model >
    _make_dataset_builder(DatasetDescriptor dataset_descriptor)
    {
        Default::DefaultBuilder< Model > dataset_builder;

        auto pos = dataset_descriptor.path.find('$'); // find indicator

        // $ indicates that string interpolation shall be used
        if (pos != std::string::npos)
        {
            // get the path builder corresponding to the flag extracted after
            // indicator
            auto path_builder =
                _modifiers[dataset_descriptor.path.substr(pos + 1)];

            std::string new_path = dataset_descriptor.path.substr(0, pos);
            // put the latter into the dataset builder
            dataset_builder =
                [dataset_descriptor, path_builder, new_path](
                    auto&& group,
                    auto&& model) -> std::shared_ptr< HDFDataset > {
                return group->open_dataset(
                    path_builder(new_path, model),
                    dataset_descriptor.dataset_capacity,
                    dataset_descriptor.dataset_chunksize,
                    dataset_descriptor.dataset_compression);
            };
        }
        else // no string to interpolate something
        {

            dataset_builder =
                [dataset_descriptor](
                    std::shared_ptr< HDFGroup >& group,
                    Model&) -> std::shared_ptr< HDFDataset > {

                return group->open_dataset(
                    dataset_descriptor.path,
                    dataset_descriptor.dataset_capacity,
                    dataset_descriptor.dataset_chunksize,
                    dataset_descriptor.dataset_compression);
            };
        }
        return dataset_builder;
    }

    /**
     * @brief Function which adapts getter functions for the correct
     *        graph accessor type, i.e., vertex_descriptor etc.
     *
     * @tparam SourceGetter automatically determined
     * @tparam Getter automatically determined
     * @param get_source Function which extracts the source to get data from
     *                   from its superior structure, e.g., extracting a
     *                   vertex list from a graph
     * @param getter Function which extracts the data to write from the source,
     *               e.g., from the vertex
     * @return Default::DefaultDataWriter< Model >
     */
    template < class SourceGetter, class Getter >
    Default::DefaultDataWriter< Model >
    _adapt_graph_writer(SourceGetter&& get_source, Getter&& getter)
    {
        Default::DefaultDataWriter< Model > writer;

        using GraphType = std::decay_t<
            std::invoke_result_t< std::decay_t< SourceGetter >, Model& > >;

        if constexpr (typetag == TypeTag::vertex_property)
        {
            writer = [&getter, &get_source](
                         std::shared_ptr< HDFDataset >& dataset,
                         Model&                                     m) -> void {
                // Collect some information on the graph
                auto& graph = get_source(m);

                // Make vertex iterators
                typename GraphType::vertex_iterator v, v_end;
                boost::tie(v, v_end) = boost::vertices(graph);

                dataset->write(v, v_end, [&getter, &graph](auto&& vd) {
                    return getter(graph[vd]);
                });
            };
        }
        else if constexpr (typetag == TypeTag::edge_property)
        {
            writer = [&getter, &get_source](
                         std::shared_ptr< HDFDataset >& dataset,
                         Model&                                     m) -> void {
                auto& graph = get_source(m);

                // Make edge iterators
                typename GraphType::edge_iterator v, v_end;
                boost::tie(v, v_end) = boost::edges(graph);

                dataset->write(v, v_end, [&getter, &graph](auto&& vd) {
                    return getter(graph[vd]);
                });
            };
        }
        else if constexpr (typetag == TypeTag::vertex_descriptor and
                           not std::is_same_v< std::decay_t< Getter >,
                                               std::function< void() > >)
        {

            writer = [&getter, &get_source](
                         std::shared_ptr< HDFDataset >& dataset,
                         Model&                                     m) -> void {
                auto& graph = get_source(m);

                // Make edge iterators
                typename GraphType::vertex_iterator v, v_end;
                boost::tie(v, v_end) = boost::vertices(graph);
                dataset->write(v, v_end, [&getter, &graph](auto&& vd) {
                    return getter(graph, vd);
                });
            };
        }
        else if constexpr (typetag == TypeTag::edge_descriptor and
                           not std::is_same_v< std::decay_t< Getter >,
                                               std::function< void() > >)
        {
            writer = [&getter, &get_source](
                         std::shared_ptr< HDFDataset >& dataset,
                         Model&                                     m) -> void {
                auto& graph = get_source(m);

                // Make edge iterators
                typename GraphType::edge_iterator v, v_end;
                boost::tie(v, v_end) = boost::edges(graph);

                dataset->write(v, v_end, [&getter, &graph](auto&& vd) {
                    return getter(graph, vd);
                });
            };
        }
        // for writing pure graphs

        // vertex
        else if constexpr (std::is_same_v< std::decay_t< Getter >,
                                           std::function< void() > > and
                           typetag == TypeTag::vertex_descriptor)
        {
            writer = [&get_source](
                         std::shared_ptr< HDFDataset >& dataset,
                         Model&                                     m) -> void {
                auto& graph = get_source(m);

                auto [v, v_end] = boost::vertices(graph);
                dataset->write(v, v_end, [&](auto&& vd) {
                    return boost::get(boost::vertex_index_t(), graph, vd);
                });
            };
        }
        // edges
        else if constexpr (std::is_same_v< std::decay_t< Getter >,
                                           std::function< void() > > and
                           typetag == TypeTag::edge_descriptor)
        {
            writer =
                [get_source](std::shared_ptr< HDFDataset >& dataset,
                             Model& m) -> void {
                auto& graph = get_source(m);

                auto [e, e_end] = boost::edges(graph);

                dataset->write(e, e_end, [&](auto&& ed) {
                    return boost::get(boost::vertex_index_t(),
                                      graph,
                                      boost::source(ed, graph));
                });

                dataset->write(e, e_end, [&](auto&& ed) {
                    return boost::get(boost::vertex_index_t(),
                                      graph,
                                      boost::target(ed, graph));
                });
            };
        }
        else
        {
            // user fucked up
            throw std::invalid_argument("Unknown ObjectType.");
        }

        return writer;
    }

  public:

    /**
     * @brief Basic factory function producing Default::DefaultWriteTask<Model>
     *        intstances, for writing out data. It is inteded to make the setup 
     *        of a WriteTask simpler for common cases.
     *
     * @tparam Model The model class the task refers to. The model is the
     *               ultimate source of data in utopia context, hence has to
     *               be given.
     *
     * @tparam SourceGetter  A container type, in this context something which
     *                       has an iterator, automatically determined.
     *
     * @tparam Getter Unary function type getting SourceGetter::value_type as
     *                argument and returning data to be written, automatically 
     *                determined.
     *
     * @tparam Group_attribute Either callable of type 
     *                         Default::GroupAttributeWriter or a tuplelike 
     *                         object.
     *
     * @tparam Dataset_attribute  Either callable of type
     *                            Default::DatasetAttributeWriter or a 
     *                            tuplelike object.
     *
     * @param name String naming this task, to be used with config.
     *
     * @param basegroup_path Path in the HDF5 file to the base_group this task
     *                       stores its produced datasets in.
     *
     * @param get_source Function which returns a container or graph holding the
     *                   data to use for write
     *
     * @param getter Unary function getting a SourceGetter::value_type argument
     *               and returning data to be written.
     *
     * @param dataset_descriptor DatasetDescriptor instance which gives the
     *                           properties constructed datasets should have, 
     *                           at the very least its path in the basegroup.
     *
     * @param group_attribute Either a callable of type
     *                        DefaultGroupAttributeWriter or a tuplelike object 
     *                        containing [attribute_name, attribute_data]. 
     *                        Usually, the latter is a descriptive string. 
     *                        The last possiblity is to give 'Nothing', which 
     *                        means that the Attribute should be ignored
     *
     * @param dataset_attribute Either a callable of type 
     *                          DefaultGroupAttributeWriter or some a tuplelike 
     *                          object containing [attribute_name, attribute_data]. 
     *                          Usually, the latter is a descriptive string. 
     *                          The last possiblity is to give 'Nothing', which 
     *                          means that the Attribute should be ignored
     *
     * @return std::pair<std::string, Default::DefaultWriteTask<Model>>
     *         pair containing a name and a writetask, to be used with the
     *         datamanager.
     */
    template < class SourceGetter,
               class Getter,
               class Group_attribute   = Nothing,
               class Dataset_attribute = Nothing >
    std::pair< std::string,
               std::shared_ptr< Default::DefaultWriteTask< Model > > >
    operator()(
        // is in config
        std::string       name,
        std::string       basegroup_path,
        DatasetDescriptor dataset_descriptor,
        // has to be given
        SourceGetter&&      get_source,
        Getter&&            getter,
        Group_attribute&&   group_attribute   = Nothing{},
        Dataset_attribute&& dataset_attribute = Nothing{})
    {
        // return type of get_source when used with model
        using Container =
            std::decay_t< decltype(get_source(std::declval< Model& >())) >;

        // asserts that the SourceGetter-type is really a container type.
        static_assert(
            Utils::is_container_v< std::decay_t< Container > > or
                Utils::is_graph_v< std::decay_t< Container > >,
            "Error, the argument 'get_source' must return a container "
            "type or graph, i.e., "
            "a type with an iterator");

        // make basegroup builder and attribute builders

        Default::DefaultBuilder< Model > dataset_builder =
            _make_dataset_builder(dataset_descriptor);

        auto group_attribute_writer = _make_attribute_writer<
            Default::DefaultAttributeWriterGroup< Model > >(
            std::forward< Group_attribute >(group_attribute));

        auto dataset_attribute_writer = _make_attribute_writer<
            Default::DefaultAttributeWriterDataset< Model > >(
            std::forward< Dataset_attribute >(dataset_attribute));

        Default::DefaultDataWriter< Model > datawriter;

        // assert that the given template argument combination is valid:
        // A graph cannot be written with TypeTag::plain

        static_assert(not(Utils::is_graph_v< std::decay_t< Container > > and
                          (typetag == TypeTag::plain)),
                      "Error in WriteTask factory:, a graph cannot be written "
                      "with TypeTag::plain, see documentation of TypeTag enum");

        if constexpr (Utils::is_graph_v< std::decay_t< Container > > and
                      (typetag != TypeTag::plain))
        {
            datawriter =
                _adapt_graph_writer(std::forward< SourceGetter >(get_source),
                                    std::forward< Getter >(getter));
        }
        else
        {

            datawriter = [getter, get_source](
                             std::shared_ptr< HDFDataset >& dataset,
                             Model& m) -> void {
                dataset->write(
                    get_source(m).begin(), get_source(m).end(), getter);
            };
        }

        // build a defaultwriteTask
        return std::make_pair(
            name,
            std::make_shared< Default::DefaultWriteTask< Model > >(
                // builds the basegroup to write datasets in
                Default::DefaultBaseGroupBuilder(
                    [basegroup_path](std::shared_ptr< HDFGroup > parent) {
                        return parent->open_group(basegroup_path);
                    }),
                // writes out data
                datawriter,
                // builds datasets as needed
                dataset_builder,
                // writes attributes to base group
                group_attribute_writer,
                // writes attributes to dataset
                dataset_attribute_writer));
    }

    /**
     * @brief Thin wrapper around the writetask constructor which allows to
     *        construct a writetask via the factory by providing all the functions
     *        the latter employs by hand. This is intended for cases where the
     *        other operator() is too restrictive.
     *
     * @param name Name of the task to be build
     * @param group_builder Function to build a HDFGroup to store datasets in
     * @param writer Function which writes out data
     * @param dataset_builder Function to build Datasets
     * @param group_attr Function writing attributes to the task's base group
     * @param dset_attr Function writing attrbutes to the task's currently
     * active dataset
     * @return std::pair<std::string, Default::DefaultWriteTask<Model>>
     */
    std::pair< std::string,
               std::shared_ptr< Default::DefaultWriteTask< Model > > >
    operator()(std::string                                     name,
               Default::DefaultBaseGroupBuilder                group_builder,
               Default::DefaultDataWriter< Model >             writer,
               Default::DefaultBuilder< Model >                dataset_builder,
               Default::DefaultAttributeWriterGroup< Model >   group_attr,
               Default::DefaultAttributeWriterDataset< Model > dset_attr)
    {
        return std::make_pair(
            name,
            std::make_shared< Default::DefaultWriteTask< Model > >(
                group_builder, writer, dataset_builder, group_attr, dset_attr));
    }
};

/**
 * @brief Initialization of the modifier map
 *
 * @tparam Model Modeltype to use.
 * @tparam typetag
 */
template < typename Model, TypeTag typetag >
std::unordered_map< std::string,
                    std::function< std::string(std::string, Model&) > >
    TaskFactory< Model, typetag >::_modifiers =
        std::unordered_map< std::string,
                            std::function< std::string(std::string, Model&) > >{
            { "time",
              [](std::string path, Model& m) {
                  return path + "_" + std::to_string(m.get_time());
              } } // for time addition
        };

/**
 * @brief Factory function which produces a Datamanager of type
 *        Default::DefaultDataManager<Model> from a config and argumets
 *        from which to construct writetasks.
 *
 * @tparam Model Modeltype to use.
 * @tparam Args Tuple types <TypeTag, std::string, argstypes_for_task...>
 * @param model Reference to the model the produced datamanagere shall belong to
 * @param w_args Tuples, have to contain [tagtype, name, argumnets...]
 * @return auto DefaultDatamanager with DefaultWriteTasks produced from model
 */
template < class Model >
class DataManagerFactory
{

    /**
     * @brief Function which calls the taskfactory with argument tuples,
     *        and takes care of using the correct type-tags
     *
     * @tparam ArgTpl automatically determined
     * @param typetag string indicating which type tag to use
     * @param arg_tpl tuple of arguments to invoke the taskfactory with
     * @return Default::DefaultWriteTask<Model> produced by invoking the
     *         TaskFactory with the passed argument tuple
     */
    template < typename ArgTpl >
    std::pair< std::string,
               std::shared_ptr< Default::DefaultWriteTask< Model > > >
    _call_taskfactory(std::string typetag, ArgTpl&& arg_tpl)
    {

        if (typetag == "plain")
        {
            DataIO::TaskFactory< Model, TypeTag::plain > factory;

            return std::apply(factory, std::forward< ArgTpl >(arg_tpl));
        }
        else if (typetag == "edge_property")
        {
            DataIO::TaskFactory< Model, TypeTag::edge_property > factory;

            return std::apply(factory, std::forward< ArgTpl >(arg_tpl));
        }
        else if (typetag == "vertex_descriptor")
        {
            DataIO::TaskFactory< Model, TypeTag::vertex_descriptor > factory;

            return std::apply(factory, std::forward< ArgTpl >(arg_tpl));
        }
        else if (typetag == "vertex_property")
        {
            DataIO::TaskFactory< Model, TypeTag::vertex_property > factory;

            return std::apply(factory, std::forward< ArgTpl >(arg_tpl));
        }
        else
        {
            DataIO::TaskFactory< Model, TypeTag::edge_descriptor > factory;

            return std::apply(factory, std::forward< ArgTpl >(arg_tpl));
        }
    }

  public:
    /**
     * @brief Builds a new datamanager from a config file and a tuple of tuples
     *        of arguments.
     * @details The latter are supplemented by the arguments given
     *        by the config, and then passed to the writetask factory, each.
     *        The result is then used for creating the datamanager. The
     *        arguments that need to be supplied in the code are
     *        - function which returns the data source, preferably by reference.
     *          Mind that you need to use decltype(auto) as return type if using
     *          a lambda.
     *        - the getter function which extracts data from the sources values,
     *          same thing as employed by dataset->write.
     *        - callable to write  group attribute or tuple containing (name,
     *          data) or 'Nothing{}' if no attributes are desired
     *        - callable to write  dataset attribute or tuple containing (name,
     *          data) or 'Nothing{}' if no attributes are desired
     *
     * @tparam Args automatically determined
     * @param conf config node giving the 'data_manager' configuration nodes
     * @param args tuple of argument tuples
     * @return auto DefaultDatamanager<Model> with the tasks built from config
     * and arguments
     */
    template < typename... Args >
    auto
    operator()(const Config& conf, 
        const std::tuple< Args... >& args,
        const Default::DefaultDecidermap< Model >& deciderfactories =
            Default::default_deciders<Model>,
        const Default::DefaultTriggermap< Model >& triggerfactories =
            Default::default_triggers<Model>)
    {
        // Get the global data manager logger
        const auto _log = spdlog::get("data_mngr");

        if constexpr (sizeof...(Args) == 0)
        {
            _log->info("Empty argument tuple for DataManager factory, "
                       "building default ...");
            return Default::DefaultDataManager< Model >{};
        }
        else
        {
            // read the tasks from the config into a map
            // has the consequence that the ordering need to match
            std::map< std::string, Config > tasknodes;
            for (auto&& node : conf["tasks"])
            {
                _log->info("Name of current task: {}",
                           node.first.as< std::string >());

                tasknodes[node.first.as< std::string >()] = node.second;
            }

            // this transforms the tuple of argument tuples std::tuple< Args...
            // >
            std::unordered_map<
                std::string,
                std::shared_ptr< Default::DefaultWriteTask< Model > > >
                tasks;

            boost::hana::for_each(
                args,
                // function gets a tuple representing arguments for the
                // writetask extracts config-supplied arguments from the
                // respective config node if the latter is found, puts them in
                // their correct place, then forwards the rest to the
                // taksfactory. If no config node for a given taskname is found,
                // exception is thrown
                [&](const auto& arg_tpl) {
                    using std::get;
                    std::string name          = "";
                    std::string name_in_tpl   = get< 0 >(arg_tpl);
                    auto        tasknode_iter = tasknodes.find(name_in_tpl);

                    // find the current task in the config, if not found throw
                    if (tasknode_iter != tasknodes.end())
                    {
                        name = tasknode_iter->first;
                    }
                    else
                    {
                        // .. and throw if it is not
                        throw std::invalid_argument(
                            "A task with name '" + name_in_tpl +
                            "' was not found in the config!");
                    }

                    // read out the typetag from the config
                    std::string typetag = "";
                    if (not tasknode_iter->second["typetag"])
                    {
                        typetag = "plain";
                    }
                    else
                    {
                        get_as< std::string >("typetag", tasknode_iter->second);
                    }

                    // make a tuple of types from the argtuple except the first
                    // element which is the sentinel name
                    auto type_tuple = boost::hana::transform(
                        boost::hana::remove_at(arg_tpl,
                                               boost::hana::size_t< 0 >{}),
                        [](auto&& t) {
                            return boost::hana::type_c<
                                std::decay_t< decltype(t) > >;
                        });

                    constexpr bool is_all_callable =
                        decltype(boost::hana::unpack(
                            type_tuple,
                            boost::hana::template_<
                                _DMUtils::all_callable >))::type::value;

                    // all arguments are callables
                    if constexpr (is_all_callable)
                    {
                        _log->info("Building write task '{}' via factory ...",
                                   name);
                        tasks.emplace(_call_taskfactory(typetag, arg_tpl));
                    }
                    // not all-callable arguments
                    else
                    {

                        // extract arguments from the configfile
                        auto config_args = std::make_tuple(
                            // name
                            name_in_tpl,
                            // basegroup_path
                            get_as< std::string >("basegroup_path",
                                                  tasknode_iter->second),
                            // dataset_descriptor
                            DatasetDescriptor{
                                get_as< std::string >("dataset_path",
                                                      tasknode_iter->second),
                                (tasknode_iter->second["capacity"]
                                     ? get_as< std::vector< hsize_t > >(
                                           "capacity", tasknode_iter->second)
                                     : std::vector< hsize_t >{}),
                                (tasknode_iter->second["chunksize"]
                                     ? get_as< std::vector< hsize_t > >(
                                           "chunksize", tasknode_iter->second)
                                     : std::vector< hsize_t >{}),
                                (tasknode_iter->second["compression"]
                                     ? get_as< int >("compression",
                                                     tasknode_iter->second)
                                     : 0) });

                        // remove the sentinel name and concat the tuples, which
                        // then is forwarded to args
                        auto full_arg_tpl = std::tuple_cat(
                            config_args,
                            boost::hana::remove_at(arg_tpl,
                                                   boost::hana::size_t< 0 >{}));

                        _log->info("Building write task '{}' via factory ...",
                                   name);

                        tasks.emplace(_call_taskfactory(typetag, full_arg_tpl));
                    }
                });

            _log->info("Forwarding arguments to DataManager constructor ...");
            // then produce default datamanager with all default deciders,
            // triggers etc

            return Default::DefaultDataManager< Model >(
                conf,
                tasks,
                deciderfactories,
                triggerfactories,
                Default::DefaultExecutionProcess());
        }
    }
};
/*! @} */

} // namespace DataIO
} // namespace Utopia

#endif
