.. _dataio_DataManager:

Utopia Datamanager — How to
===========================

This guide shows you how to set up your model to use the ``DataManager`` layer of the Utopia Data I/O module.


.. contents::
    :local:
    :depth: 2

.. note::

    If you just want to know what to do to get your model up and running with the datamanager, jump to the :ref:`dataio_DataManager_usage` section.
    The :ref:`dataio_DataManager_overview` and :ref:`dataio_DataManager_structure` sections are meant as a supplement for developers, or for the curious, but the information they contain is not needed for *using* the datamanager.

.. hint::

    For the ``DataManager`` C++ documentation, have a look `here <../../../doxygen/html/group___data_manager.html>`__.

.. note::

    This page is about the Utopia C++ library's ``DataManager``, which is something else than the :ref:`object with the same name in the frontend <utopya_data_manager>`.

.. _dataio_DataManager_overview:

Overview
--------
The Datamanager layer exists to allow the user to write any data from the model
under any conditions she defines to the hard disk, without having to fiddle
with HDF5. In fact, it is build such that it is largely independent of HDF5,
and in general tries to rely on a minimal set of assumptions. Its goal is, in
agreement with the general Utopia setup, usability without restrictivity,
i.e., the most common tasks are made easy, but you always can dive into the
code and change and customize everything to solve your specific problem, should
the need arise.

The basic idea is that the process of acquiring resources to write data to,
getting the data, processing them, and writing out the results to the mentioned
resources, can be seen as an abstract pipeline — a process which has to be
worked through each time data has to be written out from a source. This source
is, in our case, our Utopia model.
Hence, we bundled this pipeline into a ``Task``, a class which represents this
pipeline. The Datamanager layer is designed around this idea, and relies
heavily on it. In order to use it, you have to formulate your data output
process in terms of the above pipeline.

What is still missing now is a way to decide when to acquire resources for
writing data to, for instance a new HDF5 group or dataset, and when to write
data to it, both based on some condition. For us, these conditions are derived
from the model, but they can be anything, and therefore the base
implementation is not restrictive here. The object that tells when to acquire
resources is called ``Trigger``, and the one which tells when to write data to
it is called ``Decider``. Each ``Task`` **must** be linked to one ``Decider``
and one ``Trigger``, but each ``Decider`` and ``Trigger`` can manage
arbitrarily many tasks. These two maps are completely independent of each
other.

All the ``DataManager`` does then is to manage deciders and triggers with their
associated tasks, i.e., link them together based on some user input, and
orchestrate their execution. It therefore links a source of data to a target,
and adds some processing capabilities inbetween; it is thus a
`C++ stream <https://en.cppreference.com/w/cpp/io>`_.

.. admonition:: Side Note

    The execution process can be customized too!



.. _dataio_DataManager_structure:

Structure
---------

The implementation of the entire Datamanager layer is comprised of two core
parts:

* The ``DataManager`` class, which manages ``WriteTasks``. This class is
  indepenent of HDF5, hence could be used with your favorite csv-library or some
  other binary format like `NetCDF <https://en.wikipedia.org/wiki/NetCDF>`_,
  as long as you adhere to the task structure the entire module is built
  around.

* The ``WriteTask`` class, which represents an encapsulated task for
  determining when to write, what data to write, and how and when to acquire
  and release resources. Each ``WriteTask`` is bound to a ``Trigger``, which
  tells it when to acquire resources to write data to, and a ``Decider``,
  which tells it when to actually write data. As mentioned, ``Trigger`` and
  ``Decider`` are functions which get some input and return either true or
  false. The WriteTask, as currently implemented, implicitly references HDF5,
  but is exchangeable should the need arise.

To make life easier, there are two further parts of the layer, which however
are functionally not required:

* The ``Defaults``, which define default types and implementations for the
  ``WriteTasks``, as well as ``Triggers`` and ``Deciders`` for the most common
  cases. It also provides a default execution process is the heart of the
  datamanager class and orchestrates the execution of the ``WriteTasks``. More
  on the latter below.
  In about 90% of cases, you should be fine by selecting from what
  is provided.

* The *Factory*. This implements, well, factories — one for the ``WriteTasks``,
  and one for the ``DataManager`` itself. They are used to integrate the
  datamanager into the model class and allow you as a user to supply fewer and
  simpler arguments to the model, which are then augmented using the model
  config and finally employed to construct the datamanager.


WriteTask
^^^^^^^^^
A ``WriteTask`` is a class which holds five functions:

* A function which builds an HDF5 group where all the written data goes to.
  This is called ``BasegroupBuilder``.

* A function which builds an HDF5 dataset to which the currently written data is
  dumped. This functions is just called ``Builder``, because it is needed more
  often.

* A function which writes data to the dataset — the most important part of
  course. This is creatively named ``Writer``.

* The fourth function is called ``AttributeWriterGroup``; it writes metadata to
  the basegroup which has been built by the basegroup builder.

* The last function is called ``AttributeWriterDataset``, and writes metadata
  to the dataset.

Obviously, the last two functions are only useful if you intend to write
metadata, and hence they are not mandatory.


DataManager
^^^^^^^^^^^
The ``DataManager`` class internally holds the five dictionaries (maps) which
decay into two groups. The first three store the needed objects and identify
them:

* The first associates a name with a single Task. It is called *TaskMap*.

* The second associates a name with a Trigger. It is called *TriggerMap*.

* The third associates a name with a Decider.  It is called *DeciderMap*.

The last two then link them together:

* The first is called links a single decider to a collection of tasks, via
  their respective names. This is called *DeciderTaskMap*.

* The second does the same for triggers and tasks, and is called
  *TriggerTaskMap*.

Additionally, the heart of the entire system, the process of executing the
triggers, deciders and tasks together such that data is written to disk, is
called *ExecutionProcess*, and is a function held by the DataManager, and needs
to be supplied by the user. We provided one in the defaults which should
suffice unless you want to do something special.

Default Types
^^^^^^^^^^^^^
Here, the Utopia and HDF5 specifics come in. The defaults provide types and
classes needed for the usage of the datamanager with an Utopia model.
First, we need types for the five functions a ``WriteTask`` holds.

* ``DefaultBaseGroupBuilder``: a function which gets a reference to an ``HDFGroup`` as input and returns another ``HDFGroup`` as output.

* ``DefaultDataWriter``: a function which gets a reference to an ``HDFDataset`` and a reference to the model as input and returns nothing.

* ``DefaultBuilder``: a function which gets a reference to an ``HDFGroup`` and a reference to the model as input and returns a new ``HDFDataset``.

* ``DefaultAttributeWriterGroup``: a function which gets a reference to an ``HDFGroup`` and a reference to the model, and returns nothing.

* ``DefaultAttributeWriterDataset``: a function which gets an ``HDFDataset`` and a reference to the model as input and returns nothing.

All of these are implemented as ``std::function`` so that we can use (generic)
lambdas to supply them:

.. code-block:: c++

    // e.g. basegroup builder
    auto groupbuilder = [](auto&& model_basegroup){
        return model_basegroup->open_group("name_of_quantity");
    };


    // or writer
    auto writer = [](auto&& dataset, auto&& model){
        dataset->write(model.cells().begin(),
                       model.cells().end(),
                       [](auto&& cell){return cell.state;});
    };

Then there is the ``DefaultWriteTask``, which is a ``WriteTask`` built with
the default functions defined above.

Finally, there is the ``DefaultExecutionProcess``, which assumes that the
datamanager it belongs to uses default functions as defined above.
The execution process orchestrates the calling of the tasks, triggers, and
deciders with their respective argument in a sensible way, which is too long
to describe here.
Refer to the C++ documentation if you want to know exactly what is
going on.

.. _data_mngr_default_triggers_and_deciders:

Default Triggers and Deciders
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Of prime importance are the defaults for the triggers and the deciders. These
are not mere types, but fullly-fledged (if small) classes.
A *Decider* is a function which gets a reference to a model and returns a
boolean. You may have noticed that deciders and triggers are
ultimately the same (using a model to derive a boolean), and hence the
*Triggers* are identical to the *Decider*.

The following default deciders are provided:

``IntervalDecider``
"""""""""""""""""""
For an interval ``[start, stop, step]`` the decider returns true exactly when for model time ``t`` it holds that ``(start <= t < stop) && t%step == 0``.
The default value for ``step`` is 1.
If model time is in the foremost of these intervals, it returns true every ``step``-th time, else it returns false, and if model time hits ``stop``, it removes the foremost interval of the list.
It continues this until either the model has run out or its list of intervals becomes empty.
Note that the ``start`` of an interval must be larger or equal to ``stop`` of the previous interval.

``OnceDecider``
"""""""""""""""
This returns true if model time is equal to the value it holds, otherwise it returns false.

``AlwaysDecider``
"""""""""""""""""
This always returns true.

These deciders are stored in a global map called ``default_decidertypes``,
which looks like this:

+----------------------+----------------------------+
|         Name         |        Decidertype         |
+======================+============================+
| ``always``           | ``Alwaysdecider``          |
+----------------------+----------------------------+
| ``once``             | ``OnceDecider``            |
+----------------------+----------------------------+
| ``interval``         | ``IntervalDecider``        |
+----------------------+----------------------------+


This is also used for triggers.

The factories are left out here for brevity, they are only needed for
convenience and model integration.



.. _dataio_DataManager_usage:

Usage
-----

What to do in the code
^^^^^^^^^^^^^^^^^^^^^^

You have two choices:

* Write all five functions for each task yourself. You have to use the default
  signatures, because the model integrates a default datamanager only in its
  base class. You can have arbitrarily many tasks.
  Supply the functions as a tuple, for instance like this:

.. code-block:: c++

    auto args1 = std::make_tuple(
    // basegroup builder
    [](std::shared_ptr<HDFGroup>&& grp) -> std::shared_ptr<HDFGroup> {
        return grp->open_group("datagroup/1");
    },
    // writer function
    [](auto& dataset, Model& m) { dataset->write(m.x); },
    // builder function
    [](auto& group, Model& m) {
        return group->open_dataset("testgroup/initial_dataset1_" + m.name);
    },
    // attribute writer for basegroup
    [](auto& hdfgroup, Model& m) {
        hdfgroup->add_attribute(
            "dimension names for " + m.name,
            std::vector<std::string>{ "X", "Y", "Z" });
    },
    // attribute writer for dataset
    [](auto& hdfdataset, Model& m) {
        hdfdataset->add_attribute(
            "cell_data",
            std::vector<std::string>{ "resources", "traitlength", m.name });
    }
    );



.. note:: Currently, you only have an all-or-nothing choice. If you write one
    task using the full function signature, you have to provide all of them
    like this. We are aware that this is unfortunate, and will change this in the
    future.

* Write a minimal set with abbreviated arguments, translated by the factories
  into functions:


.. code-block:: c++

    auto args1 = std::make_tuple(

            // name of the task
            "adaption",

            // function for getting the source of the data, in this case, the agents
            [](auto& model) -> decltype(auto) {
                return model.get_agentmanager().agents();
            },

            // getter function used by dataset->write method. Same as in the past.
            [](auto&& agent) -> decltype(auto) {
                return agent->state()._adaption;
            },

            // tuple containing name and data to be written as basegroup attribute
            std::make_tuple("Content", "This contains agent highres data"),

            // tuple containing name and data to be written as dataset attribute
            std::make_tuple("Content", "This contains adaption data")),

    auto args2 = std::make_tuple(
            // name of the task
            "age",

            // function for getting the source of the data, in this case, the agents
            [](auto& model) -> decltype(auto) {
                return model.get_agentmanager().agents();
            },

            // getter function used by dataset->write method. Same as in the past.
            [](auto& agent) -> decltype(auto) { return agent->state()._age; },

            // 'empty' indicates that no attribute shall be written
            "empty",

            // tuple containing name and data to be written as dataset attribute
            std::make_tuple("content", "This contains age data"))


* Then supply these to your model:

    .. code-block:: c++

        Model model(name, parent, std::make_tuple(args1, args2, ...));

.. _data_mngr_custom_deciders:

How to use custom deciders or triggers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Currently, all the deciders and triggers supplied per default are bound to
some timestep value, be it a slice, an interval, or just one or every value
occuring.
There may be cases where one might need something more sophisticated, for
instance writing some data when the density of some quantity goes below some
value, or when some variable changes more rapidly than some given limit in
order to capture the dynamic episodes of the model.
To accomodate such needs, a user can supply their own deciders and/or triggers.

Before starting, a little background knowledge is necessary:
the model base class expects the deciders and triggers to be derived from
``Utopia::DataIO::Default::Decider<MyModel>`` and
``Utopia::DataIO::Default::DefaultTrigger<MyModel>``, respectively, where
``MyModel`` is the name of the model class we implemented and are using the
datamanager with.
Currently, these two interfaces are *identical*, with the default-trigger just
being an alias for the default-decider.

All deciders, (and triggers), have the same abstract base class from which
every other decider and trigger is assumed to inherit:

.. code-block:: c++

    template<typename Model>
    struct Decider {

      virutal bool operator()(Model& m) = 0;
      virtual void set_from_cfg(const Config&) = 0;
    };

The ``operator()(Model& m)`` is responsible for evaluating a condition based
on data supplied by the model, and tells if data should be written (or, if
this were a trigger, if a new dataset should be created).
``set_from_cfg`` is a function that receives a config node and uses it to set
up the decider, e.g., reading the interval in which the decider should return
true from the config (as is done for ``IntervalDecider`` for instance).

Once we know the basics, we can start implementing our own decider:
the first step consists of writing a class, called ``CustomDecider`` here,
which inherits from the ``Decider`` interface, and hence must implement the
``operator()(Model&)`` and also the ``set_from_cfg(Config&)`` functions:

.. code-block:: c++

    template<typename Model>
    struct CustomDecider: Decider<Model>
    {
      // some member variables may go here
      double limit;

      bool operator()(Model& m) override
      {
        // compute some quotient and return true whenever it is smaller than some value
        return m.some_porperty()/m.some_other_property() < limit;
      }

      void set_from_cfg(Config& cfg) override
      {
        // the limit for the output comparison above can be given in the config node
        // of the decider
        limit = get_as<double>("density_limit", cfg);
      }
    };

You can do this in your main ``model.cc`` file, but if you do it multiple
times, a new header file where all the data-IO things go may be more appropriate.

The second step consists of instantiating the "dicitionary" that maps names to
functions producing deciders.
This too can happen in your main file:

.. code-block:: c++

    // in model.cc

    auto deciders = Utopia::DataIO::Default::Decider<MyModel>;

The third step is to extend this dictionary (which in actuality is a C++
``std::unordered_map``) with a function which produces a ``std::shared_ptr``
holding this decider.
This is to make your custom decider known to the datamanager factory that
builds the datamanager for the model to use.

.. code-block:: c++

    // in model.cc

    deciders["name_of_custom_decider"] =
      []() -> std::shared_ptr<Utopia::DataIO::Default::Decider<MyModel>> {
        return std::make_shared<CustomDecider<MyModel>>();
    };

You now see why we have the ``DefautDecider`` base class: by using dynamic
polymorphism, we can build deciders and triggers with wildly varying
functionality but store them in one homogeneous container without having to
resort to metaprogramming magic.
The fourth and final step is to supply this map to your model:

.. code-block:: c++

    // in model.cc

    MyModel model(
      parent,
      std::make_tuple(/* all the dataIO tasks arguments go here as before */),
      deciders);

Now we can use the custom decider in our model config. How this works is
explained in the next paragraph.

If you have custom triggers as well, you need to repeat the process for your
custom triggers.
Note that since ``DefaultTrigger`` is just an alias for ``Decider``, every
custom decider you write can double as a trigger and vice versa.
So in order to use our custom decider from above as trigger as well, we have
to repeat step two and three and modify step four:

Step two: instantiate deciders **and** triggers:

.. code-block:: c++

    // in model.cc

    auto deciders = Utopia::DataIO::Default::DefaultDecidermap<MyModel>;
    auto triggers = Utopia::DataIO::Default::DefaultTriggermap<MyModel>;


Step three: add the custom trigger factory function:

.. code-block:: c++

    // in model.cc

    triggers["name_of_custom_trigger"] =
      []() -> std::shared_ptr<Utopia::DataIO::Default::DefaultTrigger<MyModel>> {
        return std::make_shared<CustomDecider<MyModel>>();
    };


Step four: add the custom decider **and** trigger dictionaries to the model
constructor

.. code-block:: c++

    MyModel model(
      parent,
      std::make_tuple(/* all the dataIO tasks arguments go here as before */),
      deciders,
      triggers);

Finally, note that as long as you stick to the type of the dictionary/map that
holds associates names to functions producing deciders or triggers, and you
always inherit from ``Decider`` or ``DefaultTrigger``, you can essentially do
whatever you see fit:
you do not have to instantiate the default dictionaries and extend them, but
can build completely new ones, filled with your own deciders and triggers in
step three:

.. code-block:: c++

    // in model.cc

    auto deciders = Utopia::DataIO::Default::DefaultDecidermap<MyModel>{
      std::make_pair("custom_decider", []() -> std::shared_ptr<Utopia::DataIO::Default::Decider<MyModel>>
                                      { return std::make_shared<CustomDecider<Model>>(); },
      std::make_pair("next_custom_decider", []() -> std::shared_ptr<Utopia::DataIO::Default::Decider<MyModel>>
                                      { return std::make_shared<NextCustomDecider<Model>>(); },
      /* ... */
    };

Everything else plays out as shown above.


How to write the config file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In your model config, you need to supply a 'data_manager' node, which then
has three subnodes.

.. note:: In the following, the 'data_manager' node is listed at the top of each
    example, but of course you only have to specify it once in your config, and
    the others then follow.
    

Deciders
""""""""
This node has an arbitrary number of subnodes which represent the name of
a decider. Below this comes the name of the type of the decider, i.e., the
name under which it is stored in the deciders dictionary presented in
:ref:`data_mngr_default_triggers_and_deciders` or discussed under
:ref:`data_mngr_custom_deciders`.
After this, a node  named "args" follows, which contains the arguments for the
deciders you want.
The default deciders and their respective arguments are listed in the
following:

+----------------------+----------------------------+------------------------------+
| Name                 |      Decidertype           |        Arguments             |
+======================+============================+==============================+
| ``always``           | ``Alwaysdecider``          | nothing                      |
+----------------------+----------------------------+------------------------------+
| ``once``             | ``OnceDecider``            | time to return true at       |
+----------------------+----------------------------+------------------------------+
| ``interval``         | ``IntervalDecider``        | array of intervals           |
|                      |                            | [start, end), stride         |
+----------------------+----------------------------+------------------------------+

For instance, the deciders node could look like this:

.. code-block:: yaml

  data_manager:
    # this builds the deciders
    deciders:
      write_interval:
        type: interval
        args:
          intervals:
            - [50, 75] # default stride: 1
            - [500, 1000, 1]
            - [1000, 10000, 10]
            - [10000, 11000, 5]

      write_once:
        type: once
        args:
          time: 144

      write_always:
        type: always


If you have added a custom decider as described under :ref:`data_mngr_custom_deciders`, you can add its config node in the same way:

  .. code-block:: yaml

    data_manager:
    # this builds the deciders
    deciders:
      write_interval:
        type: interval
        args:
          intervals:
            - [50, 75] # default stride: 1
            - [500, 1000, 1]
            - [1000, 10000, 10]
            - [10000, 11000, 5]

      write_once:
        type: once
        args:
          time: 144

      # here comes a custom node now
      write_when_density_is_low:
        type: name_of_custom_decider
        args:
          limit: 0.3 # this is the limit we used in the example above


Triggers
""""""""

This node has an arbitrary number of subnodes which represent the name of
a trigger each. Since the default triggers are identical to the deciders,
this section shows how to reuse some decider nodes instead of repeating
the last one. `Yaml anchors <https://blog.daemonl.com/2016/02/yaml.html>`_
are employed to achieve this reusability.

.. code-block:: yaml

    data_manager:
        deciders:
          # The & sets an anchor...
          write_interval: &interval
            type: interval
            args:
              intervals:
                - [0, 100, 10]

        triggers:
          build_once:
            type: once
            args:
              time: 42

          # which can be used via *. Like c++ pointers...
          build_interval: *interval

Custom triggers work in the exact same way as shown for custom deciders above,
and hence the example is not repeated here.

Tasks
"""""

This is the final, and biggest, subnode of the data_manager node.
It follows more or less the same principles as the other two, but with some
additions. The full node for a task looks like this:

.. code-block:: yaml

    tasks:
      taskname1:
        active: true/false
        decider: decider_name
        trigger: trigger_name
        basegroup_path: path/to/basegroup
        typetag: plain/vertex_descriptor/edge_descriptor/vertex_property/edge_property
        dataset_path: path/to/dataset/in/basegroup$<keyword>

        # optional
        capacity:  some integer number or 2d array
        chunksize: some integer number or 2d array
        compression: 1... 10

      taskname2:
        active: true/false
        ...

Let's go through this.

* The first node tells the name of the task in analogy to what we saw for
  deciders and triggers.

* The ``active`` node tells us if this task shall be used or not

* The ``decider`` and ``trigger`` nodes tell to which decider and trigger this
  task is to be bound, respectively.

* ``basegroup_path`` tells where, from the model root group, the base_group of
  the task is to be built.

* The ``typetag`` node is somewhat particular. It's a concession to
  boost::graph, and we get a uniform interface for all containers we can get
  data from, graphs included.
  Basically, it tells us how to access the data in a graph if we want to write
  out graph data. If you don't intend do deal with graphs, just use *plain* here.

* ``dataset_path`` represents the path of the dataset in the basegroup, and may include
  intermediate groups. You probably took note of the ``$keyword`` there.
  This is basically string interpolation, the way you may  be familiar with from how
  variables are treated in bash programming.
  Currently, however, there is only one keyword available, which is ``time``.
  So if you put ``some/path/to/dataset$time`` there, you get out, if you write
  at timesteps 5 and 10: ``some/path/to/dataset_5`` and
  ``some/path/to/dataset_10``.

Now come some optional dataset related parameters, which you may already know from the
HDF5 interface:

* ``capacity`` tells how big the dataset can be at a maximum.

* ``chunksize`` represents the size of chunks of the data to be written, i.e.
  how big the bites are the system takes of the data to write to file at once.

* ``compression`` is possibly the most important thing, because it tells the
  HDF5 backend to compress the data written via zlib. Reduction in data size
  can be signficant, though it can also slow everything down.

.. note::
    Note that the `$` based string interpolation can be extended upon request.

.. note:: For all of the optional parameters the following advice holds:
            use them only when you know what you are doing. The automatic
            guesses (or default values) are typically good enough.

As an example, a realistic ``WriteTasks`` node might look like this:

.. code-block:: yaml

    data_manager:
      tasks:
        state_writer:
          active: true
          decider: write_interval
          trigger: build_interval
          basegroup_path: state_group
          # typetag can be given or not, if not given, defaults to plain
          typetag: plain
          # the dollar here marks string interpolation with the current timestep
          # separated by underscore. so the datasetpath will be state_144 or so
          dataset_path: state$time
          # uncomment to set, else default
          # capacity:
          # chunksize:
          compression: 1

        state_writer_x2:
          active: true
          decider: write_interval
          trigger: build_once
          typetag: plain
          basegroup_path: state_group
          dataset_path: state_x2$time
          # this sets everything to auto
          # capacity:
          # chunksize:
          # compression: 0


And then finally, an entire ``data_manager`` node in a conifg could look
something like this:

.. code-block:: yaml

    data_manager:
      # this builds the deciders
      deciders:
        write_interval &interval
          type: interval
          args:
            intervals:
              - [0, 100, 10]

        write_interval:
          type: interval
          args:
            intervals:
              - [50, 75]

      # this builds the triggers, here deciders are used
      triggers:
        build_interval *interval

        build_once:
          type: once
          args:
            time: 50

      tasks:
        state_writer:
          active: true
          decider: write_interval
          trigger: build_interval
          basegroup_path: state_group
          # typetag can be given or not, if not, is plain
          typetag: plain
          # the dollar here marks string interpolation with the current timestep
          # separated by underscore. so the datasetpath will be state_144 or so
          dataset_path: state$time
          # uncomment to set, else default
          # capacity:
          # chunksize:
          compression: 1

        state_writer_x2:
          active: true
          decider: write_interval
          trigger: build_once
          typetag: plain
          basegroup_path: state_group
          dataset_path: state_x2$time
          # this sets everything to auto
          # capacity:
          # chunksize:
          # compression: 0
