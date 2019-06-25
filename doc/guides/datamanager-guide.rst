Utopia Datamanager - How to
===========================

This guide shows you how to set up your model to use the Datamanager layer of the
Utopia data-IO module.

.. contents:: Outline
    :local:
    :depth: 2

.. note:: If you just want to know what to do to get your model up and running
          with the datamanager, jump to section `Usage`. The `Overview` and
          `Structure` sections are meant as a supplement for developers, or for
          the curious, but the information they contain are not needed for using
          it.

Overview
--------
The Datamanager layer exists to allow the user to write any data from the model
under any conditions she defines to the hard disk, without having to fiddle with
HDF5. In fact, it is build such that it is largely independent of HDF5, and in
general tried to relie on a minimal set of assumptions. Its goal is, in agreement
with the general Utopia setup, usability without restrictivity, i.e., the most
common tasks are made easy, but you always can dive into the code and change
and customize everything to solve your specific problem should the need arise.

The basic idea is that the process of acquiring resources to write data to,
getting the data, processing them, and writing out the results to the mentioned
resources, can be seen as an abstract pipeline - a process which has to be
worked through each time data should be written out from a source. This source is,
in our case, our Utopia model.
Hence, we bundled this pipeline into a ``Task``, a class which represents this
pipeline. The Datamanager layer is designed around this idea, and relies heavily
on it. In order to use it, you have to formulate your data output process in
terms of the above pipeline.

What is still missing now is a way to decide when to acquire resources for writing
data to,  for instance a new HDF5 group or dataset, and when to write data to it,
both based on some condition. For us, these conditions are derived from the model,
but they can be anything, and therefore the base implementation is not restrictive
here. The object that tells when to acquire resources is called ``Trigger``, and
the one which tells when to write data to it is called ``Decider``. Each ``Task``
**must** be linked to one ``Decider`` and one ``Trigger``, but each ``Decider`` and
``Trigger`` can manage arbitrarily many tasks. These two maps are completely independent
of each other.

All the ``DataManager`` does then is to manage deciders and triggers with their
associated tasks, i.e., link them together based on some user input, and
orchestrate their execution. It therefore links a source of data to a traget,
and adds some processing capabilities inbetween, thus is a
`C++ stream <https://en.cppreference.com/w/cpp/io>`_.

As an aside, the execution process can be customized, too!


Structure
---------

The implementation of thhe entire Datamanager layer is comprised of two core parts:

* the ``DataManager`` class, which manages ``WriteTasks``. This class is indepenent
  of HDF5, hence could be used with your favorite csv-library, some other binary format
  like `NetCDF <https://www.unidata.ucar.edu/software/netcdf/docs/netcdf_introduction.html>`_,
  as long as you adhere to the task structure the entire module is build around.

* the ``WriteTask`` class, which represents an encapsulated task for determining
  when to write, what data to write, and how and when to acquire and release
  resources. Each ``WriteTask`` is bound to a ``Trigger``, which tells it when to
  acquire resources to write data to, and a ``Decider``, which tells it when to
  actually write data. As mentioned, ``Trigger`` and a ``Decider`` are functions which get
  some input and return either true or false. The WriteTask, as currently implemented,
  implicitly references HDF5, but is exchangeable should the need arise.

To make life easier, there are two further parts of the layer, which however
are functionally not required:

* The ``Defaults``, which define default types and implementations for the
  ``WriteTasks``, as well as ``Triggers`` and ``Deciders`` for the most common cases.
  It also provides a default execution process is the heart of the datamanager
  class and orchestrates the execution of the ``WriteTasks``. More on the latter
  below.
  In about ninety percent of cases, you should be fine with selecting from what
  is provided.

* The *Factory*. This implements, well, factories - one for the ``WriteTasks``,
  and one for the ``DataManager`` itself. They are used to integrate the
  datamanager into the model class and allow you as user to supply fewer and
  simpler arguments to the model which are then augmented using the model config
  and finally employed to construct the datamanager.


WriteTask
^^^^^^^^^
A ``WriteTask`` is a class which holds five functions:

* A function which builds a HDF5 Group where all the written data shall go to.
  This is called ``BasegroupBuilder``.

* A function which builds a HDF5 Dataset to which the currently written data is
  dumped. This functions is just called ``Builder``, because it is needed more often.

* A function which writes data to the dataset - the most important part of course.
  This is very creatively named ``Writer``.

* The forth function is called ``AttributeWriterGroup``, it writes metadata to the
  basegroup which has been build by the basegroup builder.

* The last function is called ``AttributeWriterDataset``, and writes metadata to
  the dataset.

Obviously, the last two functions are only useful if you intend to write
metadata, and hence they are not mandatory.


DataManager
^^^^^^^^^^^
The ``DataManager`` class internally holds the five dictionaries (maps) which
decay into two groups. The first three store the needed objects and identify them:

* The first associates a name with a single Task. It's called *TaskMap*.

* The second associates a name with a Trigger. It's called *TriggerMap*.

* The third associates a name with a Decider.  It's called *DeciderMap*.

The last two then link them together:

* The first is called links a single decider to a collection of tasks, via their
  respective names. This is called *DeciderTaskMap*.

* The second does the same for triggers and tasks, and is called *TriggerTaskMap*.

Additionally, the heart of the entire system, the process of executing the
triggers, deciders and tasks together such that data is written to disk, is
called *ExecutionProcess*, and is a function held by the DataManager, and needs
to be supplied by the user. We provided one in the defaults which should suffice
unless you want to do something special.

Default Types
^^^^^^^^^^^^^
Here, the Utopia and HDF5 specifics come in. The defaults provide types and
classes needed for the usage of the datamanager with an Utopia model.
First, we need types for the five functions a ``WriteTask`` holds.

* ``DefaultBaseGroupBuilder``: A function which gets a reference to an HDF5 group as
  input and returns another HDF5Group as output.

* ``DefaultDataWriter``: A function which gets a reference to a HDF5Dataset and a
  reference to the model as input and returns nothing.

* ``DefaultBuilder``: A function which gets a reference to an HDFGroup and a reference
  to the model as input and returns a new HDFDataset.

* ``DefaultAttributeWriterGroup``: A function which gets a reference to a HDFGroup and
  a reference to the model, and returns nothing.

* ``DefaultAttributeWriterDataset``: A function which gets a HDFDataset and a reference
  to the model as input and returns nothing.

All of these are implemented as `std::function` so that we can use (generic)
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

Then there is the ``DefaultWriteTask``, which is a ``WriteTask`` build with
the default functions defined above.

Finally, there is the ``DefaultExecutionProcess``, which assumes that the
datamanager it belongs to uses default functions as defined above. 
The executionprocess orchestrates the calling of the tasks, triggers and deciders
with their respective argument in a sensible way, which is too long to describe
here. Refer to the C++ documentation if you really want to know exactly what is
going on.

Default Triggers and Deciders
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Of prime importance are the defaults for the triggers and the deciders. These
are not mere types, but full fledged (if small) classes.
A *DefaultDecider* is a function which gets a reference to a model and returns a
boolean. If you paid attention, you noticed that deciders and triggers are ultimatly
the same, (using a model to derive a boolean), and hence the *DefaultTriggers*
are identical to the *DefaultDeciders*.

Five default deciders are provided:

``IntervalDecider`` 
"""""""""""""""""""

This holds a list of intervals ``[a, b)``. If model time is in
the foremost of these intervals, it returns true, else it
returns false, and if model time hits `b+1`, it removes the
foremost interval of the list. It continues this until either
the model has run out or its list is empty.

``SliceDecider``
""""""""""""""""

This returns true if model time is in, using numpy slice notation now,
``[a, b, n]``, where `n` is the step with which to go through the
interval [a, b).

``OnceDecider`` 
"""""""""""""""

This returns true if model time is equal to the value it holds,
and otherwise it returns false.

``AlwaysDecider``
"""""""""""""""""

This returns true always.

``SimpleCompositeDecider``
""""""""""""""""""""""""""

This combines the ``IntervalDecider`` and ``SliceDecider``
such that you can have intervals in which every timestep
is written out, while outside of these intervals only
every nth timestep is written out.


These deciders are stored in a global map called ``default_decidertypes``, which
looks like this:

+----------------------+----------------------------+
|         Name         |        Decidertype         |
+======================+============================+
| ``always``           | ``Alwaysdecider``          |
+----------------------+----------------------------+
| ``once``             | ``OnceDecider``            |
+----------------------+----------------------------+
| ``interval``         | ``IntervalDecider``        |
+----------------------+----------------------------+
| ``slice``            | ``SliceDecider``           |
+----------------------+----------------------------+
| ``simple_composite`` | ``SimpleCompositeDecider`` |
+----------------------+----------------------------+


This is also used for triggers.

The factories are left out here for brevity, they are only needed for convenience
and model integration.

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



.. note:: Currently, you only have a  all-or-nothing choice. If you write one
    task using the full function signature, you have to provide all of them
    like this. We are aware that this is unfortunate, and will change in the
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

        Model model(name, parent, args1, args2 /*, put more here...*/);


How to write the config file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In your model config, you need to supply a 'data_manager' node, which then
has three subnodes.
.. note:: In the following , the data_manager node is listed at top of each
example, but of course you only have to specify it once in your config, and
the others than follow. 

Deciders
""""""""

This node has an arbitrary number of subnodes which represent the name of
a decider. Below this comes the name of the type of the decider, i.e., the
name under which it is stored in the deciders dictionary presented in
`Default Triggers and Deciders`. After this, a node follows named "args",
which, you guessed it, contains the arguments for the deciders you want.
These are listed in the following:

+----------------------+----------------------------+------------------------------+
| Name                 |      Decidertype           |        Arguments             |
+======================+============================+==============================+
| ``always``           | ``Alwaysdecider``          | nothing                      |
+----------------------+----------------------------+------------------------------+
| ``once``             | ``OnceDecider``            | time to return true at       |
+----------------------+----------------------------+------------------------------+
| ``interval``         | ``IntervalDecider``        | array of intervals           |
+----------------------+----------------------------+------------------------------+
| ``slice``            | ``SliceDecider``           | array [start, end, stride    |
+----------------------+----------------------------+------------------------------+
| ``simple_composite`` | ``SimpleCompositeDecider`` | arguments for slice, interval|
+----------------------+----------------------------+------------------------------+

For instance, the deciders node could look like this:

.. code-block:: yaml

  data_manager:
    # this builds the deciders
    deciders:
      write_slice:
        type: slice
        args:
          start: 0
          stop: 100
          step: 10

      write_interval:
        type: interval
        args:
          intervals:
            - [50, 75]
            - [500, 1000]
            - [10000, 11000]
            - [20000, 25000]

      write_composite:
        type: simple_composite
        args:
          interval:
            intervals:
                - [100, 200]
                - [1000, 1100]
          slice:
            start: 0
            stop:   2000
            step: 100

      write_once:
        type: once
        args:
          time: 144

      write_always:
        type: always


Triggers
""""""""

This node has an arbitrary number of subnodes which represent the name of
a trigger each. Since the default triggers are identical to the deciders,
this section shows how to reuse some decider nodes instead of repeating
the last one. `Yaml anchors <https://blog.daemonl.com/2016/02/yaml.html>`
are employed to achieve this reusability.

.. code-block:: yaml

    data_manager:
        deciders:
          # The & sets an anchor...
          write_slice: &slicer
            type: slice
            args:
              start: 0
              stop: 100
              step: 10

        triggers:
          build_once:
            type: once
            args:
              time: 42

          # which can be used via *. Like c++ pointers...
          build_slice: *slicer

Tasks
"""""

This is the final, and biggest, subnode of the data_manager node.
It follows more or less the same principles as the other two, but with some
additions. Without further ado, lets dive into it.

The full node for a task looks like this 

.. code-block: yaml 

    tasks: 
      taskname1: 
        active: true/false
        decider: decider_name 
        trigger: trigger_name 
        basegroup_path: path/to/basegroup
        typetag: plain/vertex_descriptor/edge_descriptor/vertex_property/edge_property
        dataset_path: path/to/dataset/in/basegroup$(keyword)
        
        # optional 
        capacity:  some integer number or 2d array 
        chunksize: some integer number or 2d array 
        compression: 1... 10

      taskname2: 
        active: true/false 
        ...
            
Let's go through this.

* The first node tells the name of the task in analogy to what we saw for  deciders and triggers. 

* The ``active`` node tells us if this task shall be used or not 

* The ``Decider`` and ``Trigger`` nodes tell to which decider and trigger this 
  task  is to be bound, respectively. 

* ``basegroup_path`` tells where, from the model root group, the base_group of 
  the task shall be build. 

* The ``typetag`` node this is something peculiar. It's a concession to boost::graph, 
  and we get a uniform interface for all containers we can get data from, graphs included. 
  It basically tells us how to access the data in a graph if we want to write out 
  graph data. If we do not deal with graphs, use *plain* here. 

* ``dataset_path`` represents path of the dataset in the basegroup, may include 
  intermediate groups. Now you probably took note of the ``$keyword`` there. This 
  is basically string interpolation like you may know it from how variables are 
  treated in bash programming. Currently, however, there is only one keyword 
  available, which is ``time``. So if you put ``some/path/to/dataset$time`` 
  there, you get out, if you write at timesteps 5 and 10:  
  ``some/path/to/dataset_5`` and ``some/path/to/dataset_10``.
    
Now come some optional dataset related parameters, which you may know from the 
HDF5 interface already: 

* ``capacity`` tells how big the dataset can be at maximum

* ``chunksize`` represents the size of chunks of the data to be written, i.e. how 
  big the bites are the system takes of the data to write to file at once. 

* ``compression`` is the most important thing probably, because it tells the HDF5 
  backend to compress the data written via zlib. Reduction in data size can be 
  signficant, but so can be the loss of speed.

.. note:: 
    Note that the `$` based string interpolation can be extended upon request. 

.. note:: For all of the optional parameters the following advice holds: 
            Use them only when you know what you are doing. The automatic
            guesses (or default values), are typically good enough.

A realistic ``WriteTasks`` node looks like this, for instance:

.. code-block:: yaml

    data_manager:
      tasks:
        state_writer:
          active: true
          decider: write_slice
          trigger: build_slice
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

And then finally, an entire ``data_manager`` node in a conifg could look something 
like this: 

.. code-block:: yaml

    data_manager: 
      # this builds the deciders
      deciders:
        write_slice: &slicer
          type: slice
          args: 
            start: 0
            stop: 100
            step: 10

        write_interval:
          type: interval
          args: 
            intervals:
              - [50, 75]

      # this builds the triggers, here deciders are used
      triggers:
        build_slice: *slicer

        build_once:
          type: once
          args: 
            time: 50

      tasks:
        state_writer:
          active: true
          decider: write_slice
          trigger: build_slice
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


