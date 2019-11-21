Plotting
========

The Utopia plotting framework is an extension of the dantro plotting framework which adds specializations for the data structures used by Utopia.
Furthermore, it extends the available plotting capabilities by providing a basic set of plotting functions.

.. note::

    While this page and the linked documents aim to give an overview over the plot framework in the context of Utopia, the dantro documentation provides the full API reference and more detailed information.
    To access it, visit the `project page <https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro>`_ or the `online dantro documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/>`_.

.. contents:: Overview
   :local:
   :depth: 2

.. toctree::
   :maxdepth: 2
   :caption: Further reading
   :glob:

   plotting/uni_and_mv_plots
   plotting/plot_helper
   plotting/animations
   plotting/*

.. toctree::
   :maxdepth: 1
   :caption: utopya plot functions
   :glob:

   plot_funcs/*

----

Nomenclature
------------
In the following, the basic vocabulary to understand the plotting framework is established.
Relevant conceps and structures are:

* The :ref:`plot configuration <plot_cfg_overview>` contains all the parameters required to make one or multiple plots.
* The :ref:`plot creator <external_plot_creator>` creates the actual plot. Given some plot configuration, it produces the plot as output.
* The :py:class:`~utopya.plotting.PlotManager` orchestrates the plotting procedure by feeding the relevant plot configuration to a specific plot creator.



.. _plot_cfg_overview:

Plot Configuration
------------------
The plotting framework uses a configuration-based approach the aim of which is to keep as much information about how a plot should be created in the configuration rather than in a specific implementation.
This makes plotting much simpler and facilitates automation.

In this documentation, plot configurations are usually shown using YAML as this is the usual way to define the configuration.
The structure is:

.. code-block:: yaml

    ---
    my_plot:
      creator: some_creator
      # ... plot configuration parameters here ...

    my_other_plot:
      creator: another_creator
      # ... plot configuration parameters for this plot ...

This leads to the :py:class:`~utopya.plotting.PlotManager` instantiating a plot creator ``some_creator`` which is instructed to create a plot called ``my_plot``.
The additional parameters are passed to the plot creator which can then do whatever it likes with it.
The same happens for the ``my_other_plot`` plot, which uses ``another_creator``.

For more information on the :py:class:`~utopya.plotting.PlotManager`, refer to `the dantro documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_manager.html>`_.

.. note::

    Usually, the term *plot configuration* refers to the set of parameters required to create a *single* plot.

    Many individual plot configurations can be stored in a YAML file.
    The top level of that file then denotes the *names* of the plots.
    In the example above that is ``my_plot`` and ``my_other_plot``.

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

    This can be useful when desiring to define YAML anchors that are used in the actual configuration entries.

.. hint::

    Plot configuration entries can also make use of parameter sweeps. Simply add the ``!pspace`` tag to the top-level entry:

    .. code-block:: yaml

        ---
        my_plot: !pspace
          some_param: !sweep
            default: foo
            values: [foo, bar, baz]

    This will automatically create a separate file for each plot and include the value of the parameter into the file or folder name.


.. _plot_cfg_inheritance:

Plot Configuration Inheritance
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
New plot configurations can be based on existing ones.
This makes it very easy to define various plot functions without copy-pasting the plot configurations.

To do so, add the ``based_on`` key to your plot configuration.
As arguments, you can provide either a string or a sequence of strings, where the strings have to refer to names of so-called "base plot configuration entries". These are configuration entries that utopya and the models already provide.

For example, the following lines suffice to generate a simple line plot based on the plot function configured as ``.basic_uni.lineplot``:

.. code-block:: yaml

  tree_density:
    based_on: .basic_uni.lineplot

    model_name: ForestFire
    path_to_data: tree_density

What happens there is that first the configuration for ``.basic_uni.lineplot`` is loaded and then recursively updated with those keys that are specified below the ``based_on``.
When providing a sequence, e.g. ``based_on: [foo, bar, baz]``, the first configuration is used as the base and is subsequently recursively updated with those that follow.

For a list of all base plot configurations provided by utopya, see :doc:`here <inc/base_plots_cfg>`.


.. _external_plot_creator:

The :py:class:`~utopya.plotting.ExternalPlotCreator`
----------------------------------------------------
In Utopia, the :py:class:`~utopya.plotting.ExternalPlotCreator` has a central role as it forms the basis of some further specialized plot creators.
The *"external"* refers to is abiliy to invoke some plot function that can be in some external module or file.

Such a plot function can basically do whatever it likes.
However, the :py:class:`~utopya.plotting.ExternalPlotCreator` has some specialized functionality for working with ``matplotlib`` which aims to make plotting more convenient: the ``style`` option and the :py:class:`~utopya.plotting.PlotHelper` framework.
Furthermore, it has access to dantro's :ref:`data transformation framework <external_plot_creator_DAG_support>`.

In practice, the :py:class:`~utopya.plotting.ExternalPlotCreator` *itself* is hardly used in Utopia, but it is the base class of the :py:class:`~utopya.plotting.UniversePlotCreator` and the :py:class:`~utopya.plotting.MultiversePlotCreator`.
Thus, the following information is valid for both these specialization and is important to understand before looking at the other creators.
More detail on the specializations themselves is given :ref:`later <uni_and_mv_plots>`.


Specifying which plotting function to use
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Let's assume we have a plotting function defined somewhere and want to communicate to the :py:class:`~utopya.plotting.ExternalPlotCreator` that this function should be used for some plot.

For the moment, the exact definition of the function is irrelevant.
You can read more about it in :ref:`below <plot_func_sig_recommended>`.

Importing a plotting function from a module
"""""""""""""""""""""""""""""""""""""""""""
To do this, the ``module`` and ``plot_func`` entries are required.
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
While there are those plot function implementations that are provided by utopya and those that are provided alongside models, you might also want to implement a plot function decoupled from all the existing code.

This can be achieved by specifying the ``module_file`` key instead of the ``module`` key in the plot configuration.
The python module is then loaded from file and the ``plot_func`` key is used to retrieve the plotting function:

.. code-block:: yaml

   ---
   my_plot:
     # Load the following file as a python module
     module_file: ~/path/to/my/python/script.py

     # Use the function with the following name from that module
     plot_func: my_plot_func

     # ... all other arguments (as usual)


.. _external_plot_creator_plot_style:

Adjusting a plot's style
^^^^^^^^^^^^^^^^^^^^^^^^
All matplotlib-based plots can profit from this feature.

Using the ``style`` keyword, matplotlib parameters can be configured fully via the plot configuration; no need to touch the code.
Basically, this sets the ``matplotlib.rcParams`` and makes the matplotlib stylesheets available.

The following example illustrates the usage:

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


For the ``base_style`` entry, choose the name of a `matplotlib stylesheet <https://matplotlib.org/3.1.1/gallery/style_sheets/style_sheets_reference.html>`_.
For valid RC parameters, see the `matplotlib customization documentation <https://matplotlib.org/3.1.1/tutorials/introductory/customizing.html>`_.


.. _external_plot_creator_plot_helper:

The :py:class:`~utopya.plotting.PlotHelper`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In short, the :py:class:`~utopya.plotting.PlotHelper` assists in the creation of the plot by providing an interface to matplotlib that is accessible via the plot configuration.

For example, the following plot configuration sets the title of the plot as well as the labels and limits of the axes:

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

To learn more about how this works, see :ref:`here <plot_helper>`.
Furthermore, notice how you can combine the capabilities of the plot helper framework with the ability to :ref:`set the plot style <external_plot_creator_plot_style>`.


.. _external_plot_creator_DAG_support:

The data transformation framework
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As part of dantro, a data selection and transformation framework based on a directed, acyclic graph (DAG) of operations is provided.
This is a powerful tool, especially when combined with the plotting framework.

The motivation of using this DAG framework for plotting is similar to the motivation of the plot helper:
Ideally, the plot function should focus on the visualization of some data; everything else before (data selection, transformation, etc.) and after (adjusting plot aesthetics, saving the plot, etc.) should be automated.

This is a very powerful framework, allowing arbitrary operations.
It uses a configuration-based syntax that is optimized for specification via YAML.
Additionally, it allows to cache results to a file; this is very useful when the analysis of data takes a large amount of time compared to the plotting itself.

To learn more about this, visit the `dantro documentation of the DAG transformation framework <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/data_io/transform.html>`_.

.. hint::

    If you are missing an operation, register it yourself:

    .. code-block:: python

        from dantro.utils import register_operation

        def my_operation(data, *, some_parameter):
            """Some operation on the given data"""
            # Do something with data and the parameter
            return new_data

        register_operation(name='my_operation', func=my_operation)

    Note that you are not allowed to override any existing operation.
    To avoid naming conflicts, it is advisable to use a unique name for the custom operation, e.g. if by prefixing the model name for some model-specific operation.



Implementing plot functions
---------------------------
Below, you will learn how to implement a plot function that can be used with the plot creator.

.. _is_plot_func_decorator:

The :py:func:`~utopya.plotting.is_plot_func` decorator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When defining a plot function, we recommend using this decorator.
It takes care of providing essential information to the :py:class:`~utopya.plotting.ExternalPlotCreator` and makes it easy to configure those parameters relevant for the plot function.

For example, to specify which creator should be used for the plot function, the ``creator_type`` can be given.
To control usage of the data transformation framework, the ``use_dag`` flag can be used and the ``required_dag_tags`` argument can specify which data tags the plot function expects.


.. _plot_func_sig_recommended:

Recommended plot function signature
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The recommended way of implementing a plot function makes use of both the :ref:`plot helper framework <external_plot_creator_plot_helper>` and the :ref:`data transformation framework <external_plot_creator_DAG_support>`.

When using the data transformation framework, the data selection is taken care of by that framework, moving the data selection procedure to the plot configuration.
In the plot function, one can specify which tags are required by the plot function; the framework will then make sure that these results are computed.
In this case, two tags called ``x`` and ``y`` are required which are then fed directly to the plot function.

Importantly, such a plot function can be **averse to any creator**, because it is compatible not only with the :py:class:`~utopya.plotting.ExternalPlotCreator` but also with all its specializations.
This makes it very flexible in its usage, serving solely as the bridge between data and visualization.

.. code-block:: python

    from utopya.plotting import is_plot_func, PlotHelper

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

        # Done! The plot helper saves the plot :tada:

Super simple, aye? :)

The corresponding plot configuration could look like this:

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

For more detail on the syntax, see :ref:`above <external_plot_creator_DAG_support>`.

While the plot function signature can remain as it is regardless of the chosen specialization of the :py:class:`~utopya.plotting.ExternalPlotCreator`, the plot configuration will differ for the specializations.
See :ref:`uni_and_mv_plots` for more information.

.. note::

    This is the recommended way to define a plot function because it outsources a lot of the typical tasks (data selection and plot aesthetics) to dantro, allowing you to focus on implementing the bridge from data to visualization of the data.

    Using these features not only reduces the amount of code required in a plot function but also makes the plot function future-proof.
    We **highly** recommend to use *this* interface.



Other possible plot function signatures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. warning::

    The examples below are for the :py:class:`~utopya.plotting.ExternalPlotCreator` and might need to be adapted for the specialized plot creators.

    Examples for those creators are given in the `dantro documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_data_selection.html>`_ and :ref:`here <uni_and_mv_plots>`.

Without DAG framework
"""""""""""""""""""""
To not use the data transformation framework, simply omit the ``use_dag`` flag or set it to ``False`` in the decorator.
When not using the transformation framework, the ``creator_type`` should be specified, thus making the plot function bound to one type of creator.

.. code-block:: python

    from utopya import DataManager
    from utopya.plotting import is_plot_func, PlotHelper, ExternalPlotCreator

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
If you desire to do everything by yourself, you can disable the plot helper framework by passing ``use_helper=False`` to the decorator.
Subsequently, the ``hlpr`` argument is **not** passed to the plot function.

There is an even more basic version to do this, leaving out the :py:func:`~utopya.plotting.is_plot_func` decorator:

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
