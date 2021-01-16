.. _run_config:

Configuring Simulation Runs
===========================

Utopia has one important and wide-ranging premise when it comes to the configuration of simulations: **everything should be configurable, but nothing need be.** In other words, you should be able to have full control over all the parameters that are used in a simulation, but there should be reasonable defaults for all of them such that you don't *have* to specify them. Ideally, you only specify those parameters you want to *change* and rely on the defaults for everything else.

This flexibility is realised using a set of different configuration levels. The many different ways to adjust the configuration might be overwhelming at first, but rest assured: these options are all there for a reason, and you can benefit greatly from them. In the following we will delve into the different options.


.. _config_hierarchy:

Hierarchy of Configuration Levels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To achieve the aforementioned goal, Utopia uses a hierarchy of configuration levels:

#. **Base configuration:** all the default values
#. **Model configurations:** model-specific defaults

    * Defined alongside the respective models
    * Provide defaults *only* for the model; can be imported where needed.

#. **User configuration:** user- or machine-specific *updates* to the defaults

    * Is used for all simulation runs, regardless of the model. This would be the place to specify model-*independent* parameters like the number of CPUs to work on.
    * Nonexistent by default. Deploy using ``utopia config user --deploy``; see ``utopia config --help`` for more info. The deployed version contains descriptions of all possible settings.

#. **Run configuration:** updates for a specific simulation run
#. **Temporary changes:** additional updates, defined via the CLI

    * If you call ``utopia run --help`` you can find a list of some useful ways to adjust some parameters.
    * For example, with ``--num-steps <NUMSTEPS>`` you can specify how many time steps the model should iterate.

Combining all these levels creates the so-called **meta configuration**, which contains *all* parameters needed for a simulation run.
It is assembled by starting from the lowest level, the base configuration, and recursively updating all entries in the configuration with the entries from the next level.

The individual files and the resulting meta configuration are also stored alongside your output data, such that all the parameters are in one place.
The stored meta configuration file can also be used as the run configuration for a new simulation run, simply by passing it to ``utopia run``.

This can be a little bit confusing at first, but don't worry: the section below gives a more detailed description of the different use cases.


Where to specify changes
^^^^^^^^^^^^^^^^^^^^^^^^
Short Answer
""""""""""""
If in doubt, use the run configuration; you can specify everything there.

Longer Answer
"""""""""""""
Changes to the defaults *can* be specified in the user configuration, the run configuration, and via the CLI.

To decide where to specify your changes, think about the frequency with which you change the parameter and whether the change relates to a model-specific parameter or one that configures the framework.

Going through the following questions might be helpful:

* Is the change temporary, e.g. for a single simulation run?

    * **Yes:** Ideally, specify it via the CLI. If there are too many temporary changes, use the run configuration.
    * **No:** Continue below.

* Is the change independent of a model, e.g. the number of CPUs to use?

    * **Yes:** Use the user-configuration.
    * **No:** The parameter is model-specific; use the run configuration.


.. warning::

    The base and model configurations provide *default* values; these configuration files are **not meant to be changed**, but should reflect a certain set of persistent defaults.

    Of course, during model *development*, you as a model developer will change the default model configuration, e.g. when adding additional dynamics that require a new parameter.

.. hint::

    Have a look at the :ref:`corresponding FAQ section <faq_config>` for more information.
