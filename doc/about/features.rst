.. _utopia_features:

Feature List
============

This page aims to give an **overview** of Utopia features and improve feature discoverability.
The descriptions are brief and aim to convey the functionality of some feature and link it to internal names and the corresponding documentation.

This page links heavily to other pages which provide more information on the features.
Lines starting with the 📚 symbol also denote further reading material.

.. note::

    Please inform us about any outdated information or broken links on this page!

.. contents::
    :local:
    :depth: 3

|

|

|

----


Model Building
--------------

.. _feature_model_base_class:

The ``Model`` Base Class
^^^^^^^^^^^^^^^^^^^^^^^^
* Utopia Models are specializations of this base class
* It provides commonly needed functionality (see below) and allows control by the frontend.
* This common interface also allows *nesting* of models, thus allowing a model hierarchy.
* The following methods allow specialization of the model:

    * ``perform_step``: Performs the iteration step. You have total freedom here.
    * ``write_data``: Specifies the data that is to be stored to the associated HDF5 file.
    * ``monitor``: (Optional) Provides :ref:`monitoring data <feature_monitor>` to the frontend to control the simulation
    * ``prolog``: (Optional) Provides the possibility to perform a task before the model's first iteration. By default this includes writing of the initial state.
    * ``epilog``: (Optional) Provides the possibility to perform a task after the model's last iteration.

* Model traits can be used to specify the default types used by the model, e.g. the random number generator.
* 📚
  `Doxygen <../../doxygen/html/class_utopia_1_1_model.html>`__,
  `Model Traits <../../doxygen/html/struct_utopia_1_1_model_types.html>`_,
  :ref:`impl_step_by_step`



Basic data writing
""""""""""""""""""
* By default, the ``write_data`` method is invoked each time step. The ``write_every`` and ``write_start`` configuration arguments can be used to further control the time at which the data should be written.
* The ``Model`` base class provides two convenience methods to create datasets which already have the correct dimension names and coordinate labels associated: ``create_dset`` and ``create_cm_dset``.
* 📚
  `Doxygen <../../doxygen/html/classUtopia_1_1Model.html>`__,
  :ref:`feature_hdf5_library`



.. _feature_shared_rng:

Random Numbers
""""""""""""""
* All Utopia models have access to a *shared* random number generator with which you can create a random number through ``distr(*this->_rng)`` from your specified distribution ``distr``.
* By controlling the ``seed`` of this shared RNG, the generated random numbers allow replication.
* The default generator is the `Mersenne Twister <http://www.cplusplus.com/reference/random/mt19937/>`_ specified in the Utopia core type ``Utopia::DefaultRNG``.
* 📚
  `Doxygen <../../doxygen/html/struct_utopia_1_1_model_types.html>`__,
  `Random Number Distributions <https://en.cppreference.com/w/cpp/header/random>`_



.. _feature_logging:

Logging
"""""""
* Based on `spdlog <https://github.com/gabime/spdlog>`_, logging fast yet conveniently using the `fmt <https://github.com/fmtlib/fmt>`_ library for string parsing. No more ``std::cout``!
* Available as ``_log`` member in every ``Model``. Example:

    .. code-block:: cpp

        _log->debug("Creating {} entities now ...", num_new_entities);
        create_entities(num_new_entities);
        _log->info("Added {} new entities. Have a total of {} entities now",
                   num_new_entities, entities.size());

* **Verbosity** can be controlled for each ``Model`` using the ``log_level`` config entry. Default log levels are specified via the meta configuration, see :ref:`the base configuration <utopya_base_cfg>` for examples.
* 📚
  `Doxygen <../../doxygen/html/group___logging.html>`__


.. _feature_monitor:

Monitoring the state of the model
"""""""""""""""""""""""""""""""""
* Each ``Model`` contains a ``Monitor`` that regularly provides information to the frontend.
* The ``monitor()`` method is the place to provide that information
* It can be used for information purposes, but also to dynamically stop a simulation depending on the provided monitoring information (so-called :ref:`stop conditions <feature_stop_conditions>`).
* 📚
  `Doxygen <../../doxygen/html/group___monitor.html>`__




.. _feature_model_config:




Model Configuration
^^^^^^^^^^^^^^^^^^^
* All parameters a model is initialized with
* Available via ``_cfg`` member; ``Model`` base class takes care of supplying it.
* Each model needs to specify a **default model configuration**, but it is combined with other configurations before reaching the model instance, see :ref:`below <feature_meta_config>`.


.. _feature_reading_config:

Reading Config Entries
""""""""""""""""""""""
* Extract a config entry through, optionally providing a default value:

    .. code-block:: c++

        # Extract an entry; throws KeyError if the key is missing
        auto foo = get_as<int>("foo", this->_cfg);

        # Provide a default value when the key is missing
        auto bar = get_as<int>("bar", this->_cfg, 42)

* Supported types for ``get_as<T>`` are defined by yaml-cpp library and include basic types as well as some container types (``std::vector``, ``std::array``, also in nested form)
* There exist specializations to conveniently load entries as Armadillo types (vectors, matrices, …)
* 📚
  `Doxygen <../../doxygen/html/group___config_utilities.html>`__,
  `yamlcpp library <https://github.com/jbeder/yaml-cpp>`_



.. _feature_space:

The Physical ``Space`` a Model is embedded in
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Contains information on dimensionality, periodicity, and physical extent
* Each ``Model`` has, by default, a 2D space attached; periodicity and extent is set by the base ``Model`` using the :ref:`model configuration <feature_model_config>`.
* Is used by managers to map a :ref:`grid <feature_cell_manager>` to or control :ref:`agent <feature_agent_manager>` movement.
* 📚
  `Doxygen <../../doxygen/html/struct_utopia_1_1_space.html>`__


.. _feature_cell_manager:

The ``CellManager``
^^^^^^^^^^^^^^^^^^^
* Creates a grid discretization of the :ref:`physical space <feature_space>` and aims for being controllable from the configuration while providing a good performance.
* The grid discretization can be a ``square`` or a ``hexagonal`` lattice. This can be changed via the configuration, allowing easy evaluation of the effects of different discretizations.
* For example usage, see implemented models.
* 📚
  `Doxygen <../../doxygen/html/group___cell_manager.html>`__,
  :ref:`FAQ on Managers <managers>`


.. _feature_agent_manager:

The ``AgentManager``
^^^^^^^^^^^^^^^^^^^^
* Manage agents in a space and let them move to a relative or absolute position
* Makes sure that the agent does not leave the bounds specified by the :ref:`associated physical space <feature_space>` the model is embedded in.
* Note: Currently no efficient algorithm present to detect nearby agents.
* 📚
  `Doxygen <../../doxygen/html/group___agent_manager.html>`__,
  :ref:`FAQ on Managers <managers>`


.. _feature_apply_rule:

The ``apply_rule`` Interface – rule-based formulation of model dynamics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Apply a rule on Utopia ``Entity`` objects, e.g. ``Cell``, ``Agent``, or ``GraphEntity``. This can be used to change the state of an entity.
* Rules can be applied synchronously (in parallel) or asynchronously (sequentially)
* For asynchronous updates, the iteration order can be shuffled for each invocation. This avoids artifacts originating from a fixed application order.
* Code example:

    .. code-block:: c++

        // Apply a rule to all cells of a cell manager
        apply_rule<Update::async,             // Apply the rule asynchronously,
                                              // one cell after the other.
                   Shuffle::off>              // Do not shuffle the container
                                              // before applying the rule
        (
            [](const auto& cell){             // Operate on a cell
                auto& state = cell->state;    // Get the state reference
                state.age += 1;               // Increment the age member
                // return state;              // Optional for async update.
                                              // REQUIRED for sync update
            },
            _cm.cells()     // Apply the rule to all cells in the cell manager.
                            // This can however, also be any container of
                            // Utopia entities.
        );

        // Apply a rule to all vertices of a graph
        apply_rule<IterateOver::vertices, Update::async, Shuffle::off>(
            [](auto vertex, auto& g){
                g[vertex].state.property = 42;
            },
            g               // The graph to iterate over
        );

* With a rule that accepts more than one argument, additional container-like arguments can be passed to ``apply_rule``, leading to a ``zip``-iteration. For each entity, the arguments from the containers are then unpacked into the respective call to the rule function.
* ``apply_rule`` for manual state updates offers overloads with parallel execution policies.
    The rule will then be applied according to the selected policy, similar to a :ref:`parallel STL algorithm <feature_parallel_stl>` (it actually uses them internally).
    Even with a sequential policy (or none), internals of the ``apply_rule`` algorithms may parallelize if the feature is enabled.
    Enabling parallel features happens through the :ref:`parameter space configuration <feature_meta_config>`, or explicitly, see :ref:`feature_parallel_stl`.

    .. code-block:: c++

        // Apply a rule with multithreading
        apply_rule<Update::sync>(
            ExecPolicy::par,
            // NOTE: Rule must avoid data races!
            [](const auto& cell){
                return cell->state + 1;
            },
            _cm.cells()
        );

* 📚
  `Doxygen <../../doxygen/html/group___rules.html>`__,
  :ref:`apply_rule on graph entities <apply_rule_graph>`,
  :ref:`parallel STL algorithm overloads <feature_parallel_stl>`


.. _feature_entity:

The shared Utopia ``Entity`` type
"""""""""""""""""""""""""""""""""
* A shared type that holds a ``state``; the ``Agent`` and ``Cell`` types are derived from this base class.
* Makes the :ref:`apply_rule interface <feature_apply_rule>` possible.
* 📚
  `Doxygen <../../doxygen/html/group___entity.html>`__



.. _feature_select_entities:

The ``select`` Interface – Selecting entities using some condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Can be used to select entities from a manager in many different ways: sampling, via a probability, with a custom condition,...
* For the ``CellManager``: supports a clustering algorithm, selection of boundary cells, and creation of lanes in the grid to create different compartments.
* Fully controllable from the configuration.
* 📚
  `Doxygen <../../doxygen/html/group___entity_selection.html>`__,
  :ref:`FAQ on Entity Selection <entity_selection>`



.. _feature_graph_creation:

Graph Creation
^^^^^^^^^^^^^^
* Create a graph with the ``create_graph`` function using a selection of generating algorithms and a configuration-based interface
* Available algorithms for k-regular, fully-connected, random (Erdös-Renyi), small-world (Watts-Strogatz), highly clustered small-world (Klemm-Eguíluz), and scale-free (Barabási-Albert and Bollobás-Riordan) graphs (see :ref:`here <graph_gen_functions>`).
* Load a graph directly from GraphML or DOT (Graphviz) files. See :ref:`here<loading_a_graph_from_a_file>` for more details.
* 📚
  `Doxygen <../../doxygen/html/namespace_utopia_1_1_graph.html>`__,
  :ref:`Graph documentation entry <impl_graph>`,
  :ref:`Graph Creation requirements for the  apply_rule on Graphs <apply_rule_graph>`


Iterate Over Graph Entities
^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Conveniently loop over graph entities:

.. code-block:: c++

    include <utopia/graph/iterator.hh>
    // ...

    // Loop over all vertices and print their states
    for (auto vertex : range<IterateOver::vertex>(g)) {
        std::cout << g[vertex].property << "\n";
    }

    // Loop over all neighbors of vertex '0' and print their states
    for (auto neighbor : range<IterateOver::neighbor>(boost::vertex(0, g), g)) {
        std::cout << g[vertex].property << "\n";
    }

.. _feature_parallel_stl:

Parallel STL Algorithms
^^^^^^^^^^^^^^^^^^^^^^^

* Utopia overloads several STL algorithms with runtime execution policies for multithreading and vectorization.
  The code is agnostic to whether the optional dependencies for multithreading are installed.

  .. code-block:: c++

      #include <utopia/core/parallel.hh>

      // Enable parallel execution (will do nothing if requirements are not met)
      Utopia::ParallelExecution::set(Utopia::ParallelExecution::Setting::enabled);

      // Copy in parallel, if enabled
      std::vector<double> in(1E6, 1.0), out(1E6);
      std::copy(Utopia::ExecPolicy::par_unseq,
                begin(in),
                end(in),
                begin(out));

* Utopia ``ExecPolicy`` mirrors `STL execution policies <https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t>`_.
* Parallel features can be controlled via the :ref:`meta-configuration <feature_meta_config>`.
  The ``PseudoParent`` enables or disables them depending on the ``parallel_execution`` node in the parameter space.
* Depending on the execution policies, programmers will have to avoid `data races <https://en.cppreference.com/w/cpp/language/memory_model#Threads_and_data_races>`_.
* 📚
  `Doxygen <../../doxygen/html/group___parallel.html>`__,
  :ref:`Parallel apply_rule <feature_apply_rule>`

|

|

|

----

Writing Model Data
------------------

.. _feature_hdf5_library:

The Utopia HDF5 Library
^^^^^^^^^^^^^^^^^^^^^^^
* This library makes the HDF5 C library accessible in a convenient way.
* Beside the interface to the C library, it provides an intelligent chunking algorithm.
* 📚
  `Doxygen <../../doxygen/html/group___h_d_f5.html>`__,
  `Chunking <../../doxygen/html/group___chunking_utilities.html>`_,


.. _feature_backend_DataManager:

The ``DataManager``
^^^^^^^^^^^^^^^^^^^
* While writing simple data structures can easily be done directly with the :ref:`Utopia HDF5 library <feature_hdf5_library>`, this becomes rather difficult in more complex scenarios, e.g. when the number of agents in a system change.
* The Utopia ``DataManager`` allows to define the possible write operations and then control their execution mostly via the configuration file.
* 📚
  `Doxygen <../../doxygen/html/group___data_manager.html>`__


.. _feature_saving_graphs:

Saving Static Graphs
^^^^^^^^^^^^^^^^^^^^
* Use the ``create_graph_group`` function to create a graph group in which to save the graph using the ``save_graph`` functions to flawlessly recreate the graph in your plotting function.
* 📚
  `Doxygen <../../doxygen/html/group___graph_utilities.html>`__

Saving Dynamic Graphs
^^^^^^^^^^^^^^^^^^^^^
* Save a dynamic graph and its properties in a Utopia frontend compatible way with a single function.
* 📚
  `Doxygen <../../doxygen/html/group___graph_utilities.html>`__,
  :ref:`FAQ on saving node and edge properties <save_graph_properties>`




|

|

|

----

.. _feature_simulation_control:

Simulation Control & Configuration
----------------------------------
To generate simulation data from a model, a model needs to be executed.
This is controlled via the command line interface integrated into the Python frontend of Utopia, the :py:mod:`utopya` package.



.. _feature_CLI:

The ``utopia`` Command Line Interface (CLI)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Basic interface to control the generation of simulation data and its analysis

    .. code-block:: bash

        utopia run MyModel                       # ... using all defaults
        utopia run MyModel path/to/run_cfg.yml   # Custom run config

        utopia eval MyModel                      # Evaluate the last run
        utopia eval MyModel --plots-cfg path_to/plots_cfg.yml  # Custom plots

* Available in :ref:`Utopia's virtual environment <feature_utopia_env>`, ``utopia-env``.
* Allows setting parameters directly from the command line (have access to the whole :ref:`meta configuration <feature_meta_config>`):

    .. code-block:: bash

        utopia run MyModel --num-steps 1000 --set-params log_levels.model=debug --set-model-params my_param=12.345

* **Debug Mode:** by adding the ``--debug`` flag, logger verbosity is increased and errors are raised; this makes debugging easier.
* **Configuration Sets:** models may provide example configuration sets, which are basically pairs of run and eval configuration files.
  These can be selected via the ``--cfg-set`` command line argument and simplify running different scenarios of a model.
  See :ref:`config_sets` for more information.
* **Interactive Plotting:** for ``utopia run`` or ``utopia eval``, pass the ``--interactive`` flag to not quit the CLI after the plotting routine has finished.

    * The CLI will then give the option to change the plotting-related arguments, e.g. which plots are to be created or from which configuration file they should be created.
    * The already-loaded data is kept in memory and thus speeds-up the creation of plots, especially when large amounts of data are to be loaded.
    * *Not to confused with* the feature to work interactively with ``utopya`` using the Python interface, e.g. via IPython or Jupyter Notebook.
      See :ref:`below <feature_utopya_interactive>` for more info on that feature.

* **Copying a model:** The CLI helps a lot with that by copying all relevant files, renaming them, and even refactoring them. Copying between Utopia projects is also possible.

    .. code-block:: bash

        utopia models copy CopyMe --new-name MyFancyModel

* To learn about all possible commands:

    .. code-block:: bash

        utopia --help           # Shows all available subcommands
        utopia run --help       # Help for running a model
        utopia eval --help      # Help for evaluating a model run
        utopia config --help    # Help regarding the Utopia configuration
        utopia models --help    # Help regarding the model registry



.. _feature_meta_config:

Meta-Configuration – Controlling the Model Simulation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Every option in Utopia can be set through a configuration parameter. The complete set of configuration options of a simulation run is gathered in a meta configuration.
* Configuration levels, sequentially updating the defaults to arrive at the final meta configuration:

    #. **Base configuration:** all the default values
    #. **Model configurations:** model-specific defaults

        * Defined alongside the respective models, see :ref:`above <feature_model_config>`
        * Provide defaults not for the *whole* meta configuration but for the respective models; can be imported where needed.

    #. **User configuration:** user- or machine-specific *updates* to the defaults

        * Used for all simulation runs, regardless of the model.
        * Nonexistent by default. Deploy using ``utopia config user --deploy``; see ``utopia config --help`` for more info. The deployed version contains descriptions of all possible settings.

    #. **Run configuration:** updates for a specific simulation run
    #. **Temporary changes:** additional updates, defined via the CLI

* The ``parameter_space`` key of the meta config is passed to the model; it can be conveniently sweeped over (see :ref:`below <feature_parameter_sweeps>`).
* **Model parameters can be validated by the frontend.** This helps detecting wrongly-specified simulation runs *before* starting them and allows to reduce model implementation code.
* 📚
  :ref:`Multiverse Base Configuration <utopya_base_cfg>`,
  :py:class:`~utopya.multiverse.Multiverse`,
  :ref:`config_validation`,
  :ref:`FAQ Entry <faq_config>`



.. _feature_multiverse:

The Utopia *Multiverse* – Parallelization of Simulations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Comparing the simulation results for a set of different parameters is often required for the analysis of the model system. This is very easy in Utopia. First, some definitions:

    * A Utopia *Universe* refers to a single simulation carried out with Utopia, i.e. a specific model implementation that received a specific configuration as input.
    * A Utopia *Multiverse* refers to a *set* of such Universes with different configurations as input.

* These Universes can be naively parallelized, because they do not depend on each other. By default, when performing a *multiverse run*, Utopia automatically parallelizes their execution in this way.
* To control the behaviour, see the ``worker_manager``
* For the easy definition of different such configurations, see :ref:`below <feature_parameter_sweeps>`.
* 📚
  :py:class:`~utopya.multiverse.Multiverse`,
  :py:class:`~utopya.workermanager.WorkerManager`,
  :ref:`Multiverse Base Configuration <utopya_base_cfg>`


.. _feature_cluster_support:

Cluster Support
"""""""""""""""
* The :py:class:`~utopya.multiverse.Multiverse` also supports distributed execution, e.g. on a cluster. It detects which set of compute nodes a run is performed on and distributes the tasks accordingly.
* Cluster mode is controlled via the ``cluster_mode`` and ``cluster_params`` of the meta configuration.
* 📚
  :ref:`Multiverse Base Configuration <utopya_base_cfg>`,
  `bwForCluster Support Project <https://gitlab.com/blsqr/bwForCluster>`_

Reporter
""""""""
* The frontend also provides the :py:class:`~utopya.reporter.Reporter` classses which inform about the progress of the current tasks.
* They can be customized to do specific reporting tasks at defined trigger points, e.g. after a task (the simulation of a universe) was finished
* By default, they show an adaptive progress bar during simulation and generate a ``_report.txt`` file after the run which shows some run statistics.
* 📚
  :ref:`Multiverse Base Configuration <utopya_base_cfg>`,
  :py:class:`~utopya.reporter.Reporter`,
  :py:class:`~utopya.reporter.WorkerManagerReporter`


.. _feature_parameter_sweeps:

Defining Parameter Sweeps
^^^^^^^^^^^^^^^^^^^^^^^^^
* The ``parameter_space`` key of the :ref:`meta config <feature_meta_config>` is interpreted as a multidimensional object, a :py:class:`~paramspace.paramspace.ParamSpace`.
  The dimensions of this space are *parameters* that are assigned not a single value, but a set of values, a so-called *parameter dimension* or *sweep dimension*.
  The :py:class:`~paramspace.paramspace.ParamSpace` then contains all cartesian combinations of parameters.
  The :ref:`Multiverse <feature_multiverse>` can then iterate over all points in parameter space.
* To define parameter dimensions, simply use the ``!sweep`` and YAML tags in your **run** configuration. In the example below, a :math:`25 \times 4 \times 101`\ -sized parameter space is created.

    .. code-block:: yaml

        # Run configuration for MyModel
        ---
        parameter_space:
          seed: !sweep     # ... to have some statistics ...
            default: 42
            range: [25]    # unpacked to [0, 1, 2, ..., 24] using range(*args)

          MyModel:
            my_first_param: !sweep
              default: 42
              values: [-23, 0, 23, 42]

            my_second_param: !sweep
              default: 0.
              linspace: [0., 10., 101]   # also available: logspace

            another_param: 123.   # No sweep here

* The ``!coupled-sweep`` tag can be used to move one parameter *along* with another parameter dimension.

    .. code-block:: yaml

        # Run configuration for MyModel
        ---
        parameter_space:
          seed: !sweep
            default: 42
            values: [1, 2, 4, 8]

          MyModel:
            my_coupled_param: !coupled-sweep
              target_name: my_first_param
              # default and values from my_first_param used

            my_other_coupled_param: !coupled-sweep
              target_name: my_first_param
              default: foo
              values: [foo, bar, baz, spam] # has to have same length as target

* Sweeps are also possible for :ref:`plot configurations <feature_plots_config>`!
* 📚
  :py:class:`~paramspace.paramspace.ParamSpace`,
  :py:class:`~paramspace.paramdim.ParamDim`,
  :py:class:`~paramspace.paramdim.CoupledParamDim`,
  :ref:`run_parameter_sweeps`



.. _feature_yaml_extensions:

YAML and YAML Tags – Configuration files on steroids
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* YAML has many benefits as a configuration language, not only for :ref:`feature_parameter_sweeps` or :ref:`stop conditions <feature_stop_conditions>`.
* **Anchors** and **inheritance** make it easy to re-use definitions; avoid copy-paste at all costs! This is a built-in functionality of YAML:

    .. code-block:: yaml

        # Anchors: define with &, use with *
        some_value: &some_value 42
        some_other_value: *some_value  # ... will also be 42

        # Inheritance
        some_mapping: &some_mapping
          foo: bar
          spam: spam
        some_other_mapping_based_on_the_first_mapping:
          <<: [*some_mapping]          # Can also specify multiple anchors here
          spam: SPAM                   # Overwrite an inherited value

* Additional YAML tags help in creating configuration entries:

    .. code-block:: yaml

        seconds: !expr 60*60*24 + 1.5  # Evaluate mathematical expressions
        a_slice: !slice [10,100,5]     # Create a python slice object
        a_range: !range [0, 10, 2]     # Invokes python range(*args)
        bool1: !any [true, false]      # Evaluates a sequence of booleans
        bool2: !all [true, true]

* 📚
  :ref:`faq_config_YAML`,
  `YAML tags implemented by paramspace <https://paramspace.readthedocs.io/en/latest/yaml/supported_tags.html>`_,
  `YAML Wikipedia entry <https://en.wikipedia.org/wiki/YAML>`_,
  `YAML Tutorial <https://learnxinyminutes.com/docs/yaml/>`_



.. _feature_stop_conditions:

Stop Conditions – Dynamically stop simulations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Dynamically evaluate whether a certain simulation (or the whole run) should be stopped
* Reasons for stopping can be: timeout of individual simulation, timeout of multiverse run, or some specific :ref:`monitor <feature_monitor>` entry.
* Total timeout is controlled via ``run_kwargs.timeout`` key of :ref:`meta configuration <feature_meta_config>`.
* Can be configured via meta configuration by passing a list of conditions to the ``run_kwargs.stop_conditions`` key. Example:

    .. literalinclude:: ../../python/utopya/test/cfg/stop_conds.yml
        :language: yaml
        :start-after: ---

* 📚
  :ref:`stop_conds`


.. _feature_utopia_env:

The ``utopia-env``
^^^^^^^^^^^^^^^^^^
* A python virtual environment where all Utopia-related installation takes place; this insulates the installation of frontend dependencies from the rest of your system.
* Contains the :py:mod:`utopya` frontend package.
* Is created as part of the build process; checks dependencies and installs them if required.
* In order to be able to run the ``utopia`` CLI command, make sure to have activated the virtual environment:

    .. code-block:: bash

        $ source utopia/build/activate
        (utopia-env) $ utopia run dummy

* 📚
  :doc:`README <../README>`



|

|

|

----

Data Analysis & Plotting
------------------------

Data analysis and plotting is implemented in the Python frontend of Utopia, the :py:mod:`utopya` package.
It interfaces with the `dantro package <https://pypi.org/project/dantro/>`__ to supply a data evaluation pipeline, directly connected to the running of simulations.


.. _feature_frontend_DataManager:

Data handling with the :py:class:`~utopya.datamanager.DataManager`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Is used to load all generated simulation data and represent it in a *hierachical* fashion (the "data tree") with a **uniform interface**
* Is implemented in dantro and specialized for Utopia via the :py:class:`~utopya.datamanager.DataManager` class and the ``data_manager`` key of the meta configuration.
* Makes use of `xarray <http://xarray.pydata.org/>`_ to provide **labelled dimensions and coordinates**. This information is extracted from the HDF5 attributes.
* Supports **lazy loading**  of data using so-called :ref:`proxies <data_handling_proxy>`; these are only resolved when the data is actually needed (saves you a lot of RAM!).
  When the data is too large for the machine's memory, the :ref:`dask framework <data_handling_dask>` makes it possible to still work with the data.
* Can load data in parallel, which can speed up loading for data structures that are CPU-limited during loading. See the ``--load-parallel`` CLI option.
* ⚠️ This should not be confused with the *backend* ``DataManager`` used for *writing data*, see :ref:`above <feature_backend_DataManager>`.
* 📚
  `dantro documentation <https://dantro.readthedocs.io/en/stable/data_io/data_mngr.html>`__,
  :ref:`data_handling`,
  :py:class:`~utopya.datamanager.DataManager`,
  :ref:`Multiverse Base Configuration <utopya_base_cfg>`,
  :ref:`data_handling_load_parallel`


.. _feature_plotting:

Automatic Model Plots
^^^^^^^^^^^^^^^^^^^^^
Utopia couples tightly with the `dantro framework <https://pypi.org/project/dantro/>`_ and makes it easy to define plots alongside the model implementation.

* It is possible to configure a set of default plots which are automatically created after a model is run. For more control, plot configuration files specify the plots that are to be created.
* 📚
  `dantro documentation <https://dantro.readthedocs.io/>`__,
  :ref:`eval_plotting`


.. _feature_plots_config:

Configuring Plots
"""""""""""""""""
* Plots can be specified in a plot configuration file.
* Plot configurations can make use of so-called :ref:`base plot configurations <plot_cfg_inheritance>` to reduce copy-pasting. This also supports multiple inheritance.
* 📚
  :ref:`plot_cfg_overview`


Custom Plot functions
"""""""""""""""""""""
* Models can make use of both generic plot functions (implemented in utopya) or model-specific plot functions, which are defined in ``python/model_plots``. This allows a large flexibility in how the simulation data is analyzed and visualized.
* Plot functions can also be implemented in separate files.
* 📚
  :ref:`external_plot_creator`,
  :ref:`tutorial`


.. _feature_dag:

The Data Transformation Framework – Generic Data Processing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* This framework generalizes operations on data such that **arbitrary transformations** on the loaded data can be defined right from the configuration. It is implemented in dantro and integrated tightly with the plotting framework.
* Given some arguments, it creates a directed, acyclic graph (DAG), where each node is a transformation operation: given some input, it performs an operation, and creates some output.
* This allows **generalized plot functions** which can focus on visualizing the data they are provided with (instead of doing both: data analysis *and* visualization).
* The DAG framework provides a **file cache** that can store intermediate results such that they need not be re-computed every time the plots are generated. This makes sense for data transformations that take a long time to compute but only very little time to store to a file and load back in from there.
* 📚
  `dantro documentation <https://dantro.readthedocs.io/en/stable/data_io/transform.html>`__,
  :ref:`Usage for plotting <external_plot_creator_DAG_support>`


.. _feature_utopya_interactive:

Work interactively with utopya and dantro
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* The :py:class:`~utopya.model.Model` class makes it very easy to set up a model multiverse, run it, and load its data.

    .. code-block:: python

        import utopya

        # Create the model object
        ffm = utopya.Model(name="ForestFire")

        # Create a multiverse (mv), let it run with some config file, and then
        # load the data into the DataManager (dm)
        mv, dm = ffm.create_run_load(run_cfg_path="path/to/my/run_cfg.yml")

        # ... do something with the loaded data or the PlotManager (mv.pm)

* 📚
  :ref:`utopya_interactive`,
  :py:class:`~utopya.model.Model` class,
  :py:meth:`~utopya.model.Model.create_mv`,
  :py:meth:`~utopya.model.Model.create_run_load`,
  :py:meth:`~utopya.model.Model.create_frozen_mv` (when *loading* data from an existing run)



.. _feature_batch:

Batch-run and batch-evaluate simulations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* The :py:class:`~utopya.batch.BatchTaskManager` allows to configure multiple run and eval configurations, all from a *single* so-called "batch file".
  That way, all configuration options are in one place.
  This has several **benefits:**

    * Configuration options can be easily shared within the batch file, e.g. to define a common aesthetic for plots.
    * Creating output from multiple simulations becomes easier to replicate.
    * The batch file is self-documenting and can, in principle, be used as a lab book.
    * This feature can be especially helpful if performing data evaluation for a talk or thesis: one can set a specific output directory (e.g. ``my_thesis/figures``) and easily re-create plot output.

* The batch feature is available via the CLI by calling ``utopia batch``
* 📚
  :ref:`batch`,
  ``utopia batch --help``,
  :py:class:`~utopya.batch.BatchTaskManager` class,
  :ref:`utopya_default_batch_cfg`



|

|

|

----

.. _feature_testing_framework:

Model Testing Framework
-----------------------
Defining tests alongside a model improves the reliability and trust into the model implementation.
This can already be useful *during* the implementation of a model, e.g. when following a `test-driven development <https://en.wikipedia.org/wiki/Test-driven_development>`_ approach.

Utopia makes it easy to define tests by providing both a C++- and a Python-based testing framework.


C++: Boost-based Model Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Testing parts of a model implementation can be best done on C++ side, where you have access to the individual parts of the implementation. The Boost.Test framework offers a lot of support in defining tests for a model.
* To build and run only model-specific tests, use ``make test_model_<name>``. Consult the :doc:`README <../README>` for more information on available test targets.
* Model code coverage can also be evaluated; again, see :doc:`../README`.
* 📚
  :ref:`impl_unit_tests`,
  `Boost.Test documentation <https://www.boost.org/doc/libs/1_76_0/libs/test/doc/html/index.html>`_


Python-based Model Tests
^^^^^^^^^^^^^^^^^^^^^^^^
* Python-based tests are most useful for the *macroscopic* perspective, i.e.: given some configuration, testing that the model data is as expected.
* A test case can be as simple as this:

    .. literalinclude:: ../../python/model_tests/ForestFire/test_dynamics.py
        :language: python
        :start-after: # SPHINX-MARKER

  The tests make use of the `pytest <https://pytest.org/>`_ framework and some Utopia-specific helper classes which make running simulations and loading data easy.
  For example, test-specific configuration files can be passed to the :py:meth:`utopya.model.Model.create_run_load` method of the :py:class:`utopya.testtools.ModelTest` class... just as in the CLI.
* Tests are located on a per-model basis in the ``python/model_tests`` directory; have a look there for some more examples on how to define tests.
* The tests can be invoked using

    .. code-block:: bash

        python -m pytest -v python/model_tests/MyModel

  Consult the :doc:`../README` and the pytest documentation for more information on test invocation.
* 📚
  :ref:`impl_unit_tests` (for general remarks),
  :py:class:`~utopya.model.Model`,
  :py:class:`~utopya.testtools.ModelTest`,
  `pytest <https://pytest.org/>`_


|

|

|

----

Miscellaneous
-------------
These are not really features of Utopia itself, but of the way it is set up on the GitLab.
This environment provides some useful functionality you should know about.

Issue Board & Merge Request
^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Any questions, bug reports, feature suggestions... **write an issue** by visiting the `issue board <https://gitlab.com/utopia-project/utopia/issues>`_.
  Also seemingly minor questions have a place here!
* Want to contribute code to the framework repository? Open a `merge request <https://gitlab.com/utopia-project/utopia/merge_requests>`_.
  Looking forward to your contributions! :)
* When writing issues, MR descriptions, notes, or other content on the GitLab, take note of the many features of `GitLab MarkDown <https://docs.gitlab.com/ee/user/markdown.html>`_, e.g. for posting syntax-highlighted code, tables, simple diagrams, ... and much more.
* To add more involved diagrams like class diagrams or sequence diagrams, the GitLab also provides access to `PlantUML <http://plantuml.com>`_, simply by defining a code block with ``plantuml`` as syntax:

    .. code-block::

        ```plantuml
        Bob -> Alice : hello
        Alice -> Bob : hi
        ```


Pipelines – Automatic Builds & Test Execution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* When pushing to the Utopia project, an automatically triggered pipeline performs a number of tasks to assert Utopia's functionality:

    * All code is built with different compilers and different release types
    * All framework tests are run
    * All implemented model tests are run
    * The documentation is built and deployed to a test environment to view its current state

* Having these tasks being run automatically takes the burden off the developers' shoulders to assert that Utopia is still working as it should.
* Code changes can be merged into the master only when the pipeline succeeds and a code review has taken place.


Separate models repository
^^^^^^^^^^^^^^^^^^^^^^^^^^
* For advanced modelling projects, one typically wants to implement models in their own repository, with their own dependencies and separate version control.
  We maintain a `template project <https://gitlab.com/utopia-project/models_template>`_ which can be used to quickly generate such a models repository.
* 📚
  :ref:`set_up_models_repo`
