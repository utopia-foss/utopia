.. _eval_plotting:

Plotting
========

The Utopia plotting framework is an extension of the dantro plotting framework which adds specializations for the data structures used by Utopia.
Furthermore, it extends the available plotting capabilities by providing a basic set of plotting functions.

.. note::

    While this page and the linked documents aim to give an overview over the plot framework in the context of Utopia, the dantro documentation provides the full API reference and more detailed information.
    To access it, visit the `project page <https://gitlab.com/utopia-project/dantro>`_ or the `online dantro documentation <https://dantro.readthedocs.io/en/stable/index.html>`_.

.. contents:: Overview
   :local:
   :depth: 2

----

Nomenclature
------------
In the following, the basic vocabulary to understand the plotting framework is established.
Relevant conceps and structures are:

* The :ref:`plot configuration <plot_cfg_overview>`, which contains all the parameters required to make one or multiple plots.
* The :ref:`plot creator <pyplot_plot_creator>`, which creates the actual plot. Given some plot configuration, it outputs the plot.
* The :py:class:`~utopya.eval.plotmanager.PlotManager`, which orchestrates the plotting procedure by feeding the relevant plot configuration to a specific plot creator.



.. _plot_cfg_overview:

Plot Configuration
------------------
The plotting framework uses a configuration-based approach with the aim of keeping as much information about how a plot should be created in the configuration, rather than in a specific implementation.
This makes plotting much simpler and facilitates automation.

In this documentation, plot configurations are usually given in YAML, as this is how configurations are tpyically defined. The standard plot configuration structure is:

.. code-block:: yaml

    ---
    my_plot:
      creator: some_creator
      # ... plot configuration parameters here ...

    my_other_plot:
      creator: another_creator
      # ... plot configuration parameters for this plot ...

This leads to the :py:class:`~utopya.eval.plotmanager.PlotManager` instantiating a plot creator ``some_creator``, which is instructed to create a plot called ``my_plot``.
The additional parameters are passed to the plot creator, which then uses these for its own purposes.
The same happens for the ``my_other_plot`` plot, which uses ``another_creator``. For more information on the :py:class:`~utopya.eval.plotmanager.PlotManager`, refer to `the dantro documentation <https://dantro.readthedocs.io/en/stable/plotting/plot_manager.html>`_.

Plot configuration entries can also make use of parameter sweeps. Simply add the ``!pspace`` tag to the top-level entry:

.. code-block:: yaml

    ---
    my_plot: !pspace
      some_param: !sweep
        default: foo
        values: [foo, bar, baz]

This will automatically create a separate file for each plot and include the value of the parameter into the file or folder name.


.. note::

    Usually, the term *plot configuration* refers to the set of parameters required to create a *single* plot.

    Many individual plot configurations can be stored in a YAML file.
    The top level of that file then denotes the *names* of the plots.
    In the above example, these would be ``my_plot`` and ``my_other_plot``.

.. hint::

    Plot configuration entries starting with an underscore are ignored by the plot manager:

    .. code-block:: yaml

        ---
        _foobar:        # This entry is ignored
          # ...

        my_plot:        # -> creates my_plot
          # ...

        my_other_plot:  # -> creates my_other_plot
          # ...

    This can be useful when defining YAML anchors that are used in the actual configuration entries.




.. _plot_cfg_inheritance:

Plot Configuration Inheritance
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
New plot configurations can be based on existing ones.
This makes it very easy to define various plot functions without copy-pasting the plot configurations.
To do so, add the ``based_on`` key to your plot configuration.
As arguments, you can provide either a string or a sequence of strings, where the strings have to refer to names of so-called "base plot configuration entries".
These are configuration entries that utopya and the models already provide.

For example, the following lines suffice to generate a simple line plot based on the plot function configured as ``.basic_uni.lineplot``:

.. code-block:: yaml

  tree_density:
    based_on: .basic_uni.lineplot

    model_name: ForestFire
    path_to_data: tree_density

Here, the configuration for ``.basic_uni.lineplot`` is first loaded and then recursively updated with those keys that are specified below the ``based_on``.
When providing a sequence, e.g. ``based_on: [foo, bar, baz]``, the first configuration is used as the base and is subsequently recursively updated with those that follow.

For a list of all base plot configurations provided by utopya, see :ref:`utopya_base_cfg`.

This feature is completely implemented in dantro; see the `plot configuration inheritance <https://dantro.readthedocs.io/en/latest/plotting/plot_manager.html#plot-configuration-inheritance>`_ entry in the linked documentation for details.
For Utopia, the following base configuration pools are made available:

* The ``utopya`` base configuration pool, see :ref:`utopya_base_cfg`
* The ``{model_name}_base`` configuration pool for the currently selected model, if available

Custom or additional base config pools
""""""""""""""""""""""""""""""""""""""
Which base config pools are used can be adjusted via the meta configuration.
This allows to introduce additional config pools, thereby allowing a more versatile plot configuration inheritance.
For instance, an additional base config pool may be used to adjust a commonly used style, which can be very helpful when desiring to create publication-ready figures without redundantly defining plots.

By default, only the two aforementioned pools are available.
In the meta config, this looks like this, using a shortcut syntax:

.. code-block:: yaml

    plot_manager:
      base_cfg_pools:
        - utopya_base
        - framework_base
        - project_base
        - model_base

Additional entries in that list are expected to be 2-tuples of the form ``(name, path)``, where each string can be a format string.
For example, the list may be amended to include two additional pools:

.. code-block:: yaml

    plot_manager:
      base_cfg_pools:
        - utopya_base
        - model_base
        - ["{model_name}_extd", "{paths[source_dir]}/{model_name}_extd.yml"]
        - ["{model_name}_custom", "~/some/path/{model_name}/custom_base.yml"]

The example shows how there is access to the model name and the model-specific paths dict (only in the second entry of the tuple).
In the first case, a file within the model source directory (alongside the other config files) is added as an additional base pool; in the second example, an arbitrary directory is used.

.. note::

    If no file exists at the specified pool path, a warning will be emitted in the log and an empty pool will be used (having no effect on any plot).
    This allows for more flexibility if only some models have additional plot config pools defined.

Now, how can this be used during plot config inheritance?
Consider the case where a plot ``my_plot`` for model ``MyModel`` is meant to have a certain style which is optimized for publication e.g. by using TeX labels or a larger font size.
This can be achieved by adding an additional config pool and, importantly, *without touching the default plot configuration or the plot definition itself*.

.. code-block:: yaml

    # (0) -- utopya_base
    .default_style_and_helpers:
      # ...

    # (1) -- MyModel_base_plots.yml
    # Define model-specific default style
    .default_style:
      based_on: .default_style_and_helpers  # inherit from (0)

      style:
        base_style: default
        font.size: 8.0

    # (2) -- MyModel_extd.yml
    .default_style:
      based_on: .default_style  # inherit from (1)

      # Overwrite some parts
      style:
        base_style: seaborn-paper

        font.family: serif
        font.size: 10.0
        text.usetex: true
        # ...

    # (3) -- MyModel/custom_base.yml
    # no relevant changes here

    # (4) -- MyModel_plots.yml
    my_plot:
      based_on:
        - my_plot_definition  # defined somewhere
        - .default_style      # inherit from (2)

As you can see above, this builds a multi-stage inheritance chain.
The important part is that the custom-added base pool (2) inherits the default plot style from (1) and extends it.
The lookup of ``based_on`` in (4) now inherits not from (1) but from (2), thus using the adjusted style.

And with that, the style (or other properties) of the default plots defined in (4) can be adjusted without touching that configuration or the model base plots config.
Yay.

.. hint::

    While this is a powerful tool, be aware that this may quickly escalate and may become hard to maintain.
    Use responsibly.



Configuration sets
^^^^^^^^^^^^^^^^^^
Same as run configurations, plot configurations can also be included in :ref:`Configuration Sets <config_sets>`, simply by adding an ``eval.yml`` file to the configuration set directory.
This allows to define plot configurations for a specific simulation run, directly alongside it.

.. hint::

    To avoid excessive duplication of plot configurations when adding config sets, :ref:`plot_cfg_inheritance` can be helpful:

        * put shared definitions into the base configuration
        * in the config set, only specify those options that deviate from the default or that should better be explicitly specified



.. _pyplot_plot_creator:

The :py:class:`~utopya.eval.plotcreators.PyPlotCreator`
-------------------------------------------------------
In Utopia, the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` has a central role, as it forms the basis of several, more specialized plot creators.
The *"external"* refers to is abiliy to invoke some plot function from an external module or file. Such a plot function can essentially be arbitrary. However, the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` has some specialized functionality for working with ``matplotlib`` which aims to make plotting more convenient: the ``style`` option and the :py:class:`~utopya.eval.plothelper.PlotHelper` framework.
Furthermore, it has access to dantro's :ref:`data transformation framework <pyplot_plot_creator_DAG_support>`.

In practice, the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` *itself* is hardly used in Utopia, but it is the base class of the :py:class:`~utopya.eval.plotcreators.UniversePlotCreator` and the :py:class:`~utopya.eval.plotcreators.MultiversePlotCreator`.
Thus, the following information is valid for both these specializations and is important to understand before looking at the other creators.
More detail on the specializations themselves is given :ref:`later <uni_and_mv_plots>`.


Specifying which plotting function to use
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Let's assume we have a plotting function defined somewhere and want to communicate to the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` that this function should be used for some plot. For the moment, the exact definition of the function is irrelevant. You can read more about it in :ref:`below <plot_func_sig_recommended>`.

Importing a plotting function from a module
"""""""""""""""""""""""""""""""""""""""""""
To import a plot function, the ``module`` and ``plot_func`` entries are required.
The following example shows a plot that uses a plot function from ``utopya.plot_funcs`` and another plot that uses some (importable) package from which the module and the plot function are imported:

.. code-block:: yaml

   ---
   my_plot:
     # Import some module from utopya.plot_funcs (note the leading dot)
     module: .distribution

     # Use the function with the following name from that module
     plot_func: my_plot_func

     # ... all other arguments

   my_other_plot:
     # Import a module from any installed package
     module: my_installed_plotting_package.some_module
     plot_func: my_plot_func

     # ... all other arguments


.. _external_plot_funcs:

Importing a plotting function from a file
"""""""""""""""""""""""""""""""""""""""""
There are plenty of plot function implementations provided both by utopya and the various Utopia models. However, you might also want to implement a plot function of your own design. This can be achieved by specifying the ``module_file`` key instead of the ``module`` key in the plot configuration. The python module is then loaded from file and the ``plot_func`` key is used to retrieve the plotting function:

.. code-block:: yaml

   ---
   my_plot:
     # Load the following file as a python module
     module_file: ~/path/to/my/python/script.py

     # Use the function with the following name from that module
     plot_func: my_plot_func

     # ... all other arguments (as usual)


.. _pyplot_plot_creator_plot_style:

Adjusting a plot's style
^^^^^^^^^^^^^^^^^^^^^^^^
All matplotlib-based plots can profit from this feature. Using the ``style`` keyword, matplotlib parameters can be configured fully via the plot configuration; no need to touch the code. Basically, this sets the ``matplotlib.rcParams`` and makes the matplotlib stylesheets available. The following example illustrates the usage:

.. code-block:: yaml

    ---
    my_plot:
      # ...

      # Configure the plot style
      style:
        base_style: ~        # optional, name of a matplotlib style to use
        rc_file: ~           # optional, path to YAML file to load params from
        # ... all further parameters are interpreted directly as rcParams

In the following example, the ``ggplot`` style is used and subsequently adjusted by setting the linewidth, marker size and label sizes.

.. code-block:: yaml

    ---
    my_ggplot:
      # ...

      style:
        base_style: ggplot
        lines.linewidth : 3
        lines.markersize : 10
        xtick.labelsize : 16
        ytick.labelsize : 16


For the ``base_style`` entry, choose the name of a `matplotlib stylesheet <https://matplotlib.org/stable/gallery/style_sheets/style_sheets_reference.html>`_.
For valid RC parameters, see the `matplotlib customization documentation <https://matplotlib.org/stable/tutorials/introductory/customizing.html>`_.


.. _pyplot_plot_creator_plot_helper:

The :py:class:`~utopya.eval.plothelper.PlotHelper`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The aim of the :py:class:`~utopya.eval.plothelper.PlotHelper` framework is to let the plot functions focus on what cannot easily be automated: being the bridge between some selected data and its visualization. The plot function should not have to concern itself with plot aesthetics, as these can be easily automated. The :py:class:`~utopya.eval.plothelper.PlotHelper` framework can make your life significantly easier, as it already takes care of most of the plot aesthetics by making large parts of the matplotlib interface accessible via the plot configuration. That way, you don't need to touch Python code for trivial tasks like changing the plot limits. It also takes care of setting up a figure and storing it in the appropriate location. Most importantly, it will make your plots future-proof and let them profit from upcoming features. For available plot helpers, have a look at the :py:class:`~utopya.eval.plothelper.PlotHelper` API reference.

As an example, the following plot configuration sets the title of the plot as well as the labels and limits of the axes:

.. code-block:: yaml

  my_plot:
    # ...

    # Configure the plot helpers
    helpers:
      set_title:
        title: This is My Fancy Plot
      set_labels:
        x: $A$
        y: Counts $N_A$
      set_limits:
        x: [0, max]
        y: [1.0, ~]

Furthermore, notice how you can combine the capabilities of the plot helper framework with the ability to :ref:`set the plot style <pyplot_plot_creator_plot_style>`.


.. _pyplot_plot_creator_DAG_support:

The data transformation framework
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As part of dantro, a data selection and transformation framework based on a directed, acyclic graph (DAG) of operations is provided.
This is a powerful tool, especially when combined with the plotting framework.

What motivates using this DAG framework for plotting is similar what motivates the plot helper:
ideally, the plot function should focus on the visualization of some data; everything else before (data selection, transformation, etc.) and after (adjusting plot aesthetics, saving the plot, etc.) should be automated. The DAG allows for arbitrary operations, making it a highly versatile and powerful framework. It uses a configuration-based syntax that is optimized for specification via YAML. Additionally, it allows to cache results to a file; this is very useful when the analysis of data takes much longer than the plotting itself.

To learn more, visit the `dantro documentation of the DAG transformation framework <https://dantro.readthedocs.io/en/stable/data_io/transform.html>`_.

.. hint::

    If you are missing an operation, you can register it yourself using :py:func:`~utopya.eval.transform.register_operation`.
    Add something like the following to your model-specific plot module:

    .. code-block:: python

        """model_plots/MyModel/__init__.py"""

        # Your regular imports here

        # --- Register custom operations ...
        from utopya.eval import register_operation

        # ... from some imported module
        import numpy as np
        register_operation(name='np.mean', func=np.mean)

        # ... from a lambda
        register_operation(name='MyModel', func=lambda d: d**2)

        # ... from some custom callable
        def my_operation(data, *, some_parameter):
            """Some operation on the given data"""
            # Do something with data and the parameter
            return new_data

        register_operation(name='MyModel.my_operation', func=my_operation)

    Of course, custom operations can also be defined somewhere else within your plot modules, e.g. an ``operations.py`` file, and imported into ``__init__.py`` using ``from .operations import my_operation``.

    Note that you are not allowed to override any existing operation.
    To avoid naming conflicts, it is advisable to **use a unique name for custom operations**, e.g. by prefixing the model name for some model-specific operation.

    **Important:** Your model-specific custom operations should be defined in the model-specific plot module, i.e.: accessible after importing ``model_plots/<your_model_name>/__init__.py``.
    Prior to plotting, the :py:class:`~utopya.eval.plotmanager.PlotManager` pre-loads that module, such that the ``register_operation`` calls are actually invoked.


Implementing plot functions
---------------------------
Below, you will learn how to implement a plot function that can be used with the plot creator.

.. _is_plot_func_decorator:

The :py:func:`~dantro.plot.utils.plot_func.is_plot_func` decorator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When defining a plot function, we recommend using this decorator.
It takes care of providing essential information to the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` and makes it easy to configure those parameters relevant for the plot function. For example, to specify which creator should be used for the plot function, the ``creator_type`` can be given.
To control usage of the data transformation framework, the ``use_dag`` flag can be used and the ``required_dag_tags`` argument can specify which data tags the plot function expects.


.. _plot_func_sig_recommended:

Recommended plot function signature
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The recommended way of implementing a plot function makes use of both the :ref:`plot helper framework <pyplot_plot_creator_plot_helper>` and the :ref:`data transformation framework <pyplot_plot_creator_DAG_support>`.

When using the data transformation framework, the data selection is taken care of by that framework, moving the data selection procedure to the plot configuration.
In the plot function, one can specify which tags are required by the plot function; the framework will then make sure that these results are computed.
In the following example, two tags called ``x`` and ``y`` are required, which are then fed directly to the plot function.

Importantly, such a plot function can be **averse to any creator**, because it is compatible not only with the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` but also with all its specializations.
This makes it very flexible in its usage, serving solely as the bridge between data and visualization.

.. code-block:: python

    from utopya.eval import is_plot_func, PlotHelper

    @is_plot_func(use_dag=True, required_dag_tags=('x', 'y'))
    def my_plot(*, data: dict, hlpr: PlotHelper, **plot_kwargs):
        """A creator-averse plot function using the data transformation
        framework and the plot helper framework.

        Args:
            data: The selected and transformed data, containing specified tags.
            hlpr: The associated plot helper.
            **plot_kwargs: Passed on to matplotlib.pyplot.plot
        """
        # Create a lineplot on the currently selected axis
        hlpr.ax.plot(data['x'], data['y'], **plot_kwargs)

        # Done! The plot helper saves the plot.

Simple, right? The corresponding plot configuration could look like this:

.. code-block:: yaml

    my_plot:
      creator: external

      # Select the plot function
      # ...

      # Select data
      select:
        x: data/MyModel/some/path/foo
        y:
          path: data/MyModel/some/path/bar
          transform:
            - mean: [!dag_prev ]
            - increment: [!dag_prev ]

      # Perform some transformation on the data
      transform: []

      # ... further arguments

For more detail on the syntax, see :ref:`above <pyplot_plot_creator_DAG_support>`.

While the plot function signature can remain as it is regardless of the chosen specialization of the :py:class:`~utopya.eval.plotcreators.PyPlotCreator`, the plot configuration will differ for the various specializations.
See :ref:`uni_and_mv_plots` for more information.

.. note::

    This is the recommended way to define a plot function, because it outsources a lot of the typical tasks (data selection and plot aesthetics) to dantro, allowing you to focus on implementing the bridge from data to visualization of the data.

    Using these features not only reduces the amount of code required in a plot function, but also makes the plot function future-proof.
    We **highly** recommend using *this* interface.



Other possible plot function signatures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. warning::

    The examples below are for the :py:class:`~utopya.eval.plotcreators.PyPlotCreator` and might need to be adapted for the specialized plot creators.

    Examples for those creators are given in the `dantro documentation <https://dantro.readthedocs.io/en/stable/plotting/plot_data_selection.html>`_ and :ref:`here <uni_and_mv_plots>`.

Without DAG framework
"""""""""""""""""""""
If you wish not to use the data transformation framework, simply omit the ``use_dag`` flag or set it to ``False`` in the decorator.
When not using the transformation framework, the ``creator_type`` should be specified, thus binding the plot function to one type of creator.

.. code-block:: python

    from utopya import DataManager
    from utopya.eval import is_plot_func, PlotHelper, ExternalPlotCreator

    @is_plot_func(creator_type=ExternalPlotCreator)
    def my_plot(dm: DataManager, *, hlpr: PlotHelper, **additional_kwargs):
        """A plot function using the plot helper framework.

        Args:
            dm: The DataManager object that contains all loaded data.
            hlpr: The associated plot helper
            **additional_kwargs: Anything else from the plot config.
        """
        # Select some data ...
        data = dm['foo/bar']

        # Create a lineplot on the currently selected axis
        hlpr.ax.plot(data)

        # When exiting here, the plot helper saves the plot.

.. note::

    The ``dm`` argument is only provided when *not* using the DAG framework.


Bare basics
"""""""""""
If you really want to do everything by yourself, you can also disable the plot helper framework by passing ``use_helper=False`` to the decorator. The ``hlpr`` argument is then **not** passed to the plot function.

There is an even more basic version of doing this, leaving out the :py:func:`~dantro.plot.utils.plot_func.is_plot_func` decorator:

.. code-block:: python

    from utopya import DataManager

    def my_bare_basics_plot(dm: DataManager, *, out_path: str,
                            **additional_kwargs):
        """Bare-basics signature required by the ExternalPlotCreator.

        Args:
            dm: The DataManager object that contains all loaded data.
            out_path: The generated path at which this plot should be saved
            **additional_kwargs: Anything else from the plot config.
        """
        # Your code here ...

        # Save to the specified output path
        plt.savefig(out_path)

.. note::

    When using the bare basics version, you need to set the ``creator`` argument in the plot configuration in order for the plot manager to find the desired creator.
