.. _run_config:

Configuring Simulation Runs
===========================

Utopia has one important and wide-ranging premise when it comes to the configuration of simulations: **everything should be configurable, but nothing need be.** In other words, you should be able to have full control over all the parameters that are used in a simulation, but there should be reasonable defaults for all of them such that you don't *have* to specify them. Ideally, you only specify those parameters you want to *change* and rely on the defaults for everything else.

This flexibility is realised using a set of different configuration levels. The many different ways to adjust the configuration might be overwhelming at first, but rest assured: these options are all there for a reason, and you can benefit greatly from them. In the following we will delve into the different options.

.. contents::
    :local:
    :depth: 2


.. _config_hierarchy:

Hierarchy of Configuration Levels
---------------------------------
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



.. _config_sets:

Configuration sets
------------------
Typically, a model's default configuration is only *one* of many scenarios one wants to investigate.
*Configuration sets* allow to define additional run configuration files alongside with their plots configuration and make them conveniently accessible via the CLI.

The following command will invoke a ``ForestFire`` model simulation with the run and plots configuration specified in that config set:

.. code-block:: bash

    utopia run ForestFire --cfg-set multiverse_example

A config set is simply a directory containing a ``run.yml`` file and/or an ``eval.yml`` file.
In the above example, the directory's name is ``multiverse_example`` and it contains both of these files.
If one of the files is missing, the respective defaults will be used.

.. hint::

    The CLI still accepts a run configuration or plots configuration path, which will have precedence over the files defined via the config set.

Under the hood, the ``Model`` class searches for these config sets, making it accessible for :ref:`interactive use <utopya_interactive>` via :py:meth:`~utopya.model.Model.get_config_set` and :py:meth:`~utopya.model.Model.get_config_sets`.

Here, however, we will focus on use via the CLI.


Search procedure
^^^^^^^^^^^^^^^^
When specifying a config set with some name, say ``my_config_set``, the :py:meth:`~utopya.model.Model.get_config_set` method is invoked.
It first looks for all available configuration sets and then selects the one with the specified name.

The following directories will be searched for subdirectories with name ``my_config_set``:

    * If the given name can be resolved to the path of an existing directory, that directory
    * Any additionally specified directories in the utopya configuration
    * The ``cfgs`` directory in the model's source directory, in this case: ``.../src/utopia/models/ForestFire/cfgs``

The search takes place in *that* order and stops once a matching config set is found.

.. note::

    If using :py:meth:`~utopya.model.Model.get_config_sets` to retrieve *all* available configuration sets, there will appear warnings if config sets with the same name are found in different search locations.

    Config sets with the same name are *not* merged.



The ``cfgs`` directory
""""""""""""""""""""""
This directory is typically created by a model developer, e.g. to provide example configurations.
If you are a model developer, simply create a directory called ``cfgs`` and add your config set directories to it.
(In this case that would be ``cfgs/my_config_set/run.yml`` and ``…/eval.yml``.)


User-specified search directories
"""""""""""""""""""""""""""""""""
There can also be user-specified config set search directories, which is useful if you do not have access to or don't want to modify the model source directory.

These search directories are specified in ``~/.config/utopia/utopya_cfg.yml``.
If such a file does not exist on your machine, create it and add a ``config_set_search_dirs`` key, which lists the directories you want to additionally search.
This may look as follows:

.. code-block:: yaml

    ~/.config/utopia/utopya_cfg.yml
    ---
    config_set_search_dirs:
      - ~/utopia_cfgs/{model_name:}
      - /more/config_sets/{model_name:}

Notice the (optional) ``{model_name:}`` placeholder, which is automatically resolved to the current model's name.

.. note::

    The directories will be searched in the order given there.
    If config sets with the same name appear in multiple search directories, the *former* ones will have precedence.


Local search directories
""""""""""""""""""""""""
Additionally, a local path may also be specified during search, for example:

.. code-block:: bash

    utopia run MyModel --cfg-set my/extra/cfg_sets/foo

This will add ``my/extra/cfg_sets`` as the last search directory and specify ``foo`` as the desired name, thus yielding the local config set.

The argument may also be absolute or include ``~``.

.. note::

    Under the hood, *every* argument to ``--cfg-set`` will be checked for whether it is a path to an *existing* directory.
    Only if that is the case, the parent directory will be searched.



See available config sets
^^^^^^^^^^^^^^^^^^^^^^^^^
To see the names of available config sets for each model, use:

.. code-block::

    utopia models info ForestFire
