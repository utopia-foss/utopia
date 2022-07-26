Implementing your own plot functions
====================================

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
The same happens for the ``my_other_plot`` plot, which uses ``another_creator``.
For more information on the :py:class:`~utopya.eval.plotmanager.PlotManager`, refer to `the dantro documentation <https://dantro.readthedocs.io/en/latest/plotting/plot_manager.html>`_.




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
