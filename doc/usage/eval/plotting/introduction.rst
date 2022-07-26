.. _eval_plotting:

Introduction
============

.. admonition:: Summary

    On this page, we give an introduction to

    * using **plot configurations** to organise your plots, **hierarchically** basing plots on each other, and **recursively** updating individual entries.
    * the `Data Transformation Framework <https://dantro.readthedocs.io/en/stable/data_io/transform.html>`_, which automates data analysis and transformation, allows you to control the data analysis right from the configuration, and keeps data processing separate from plotting.

The **Utopia Plotting Framework** is a powerful tool that handles everything from efficiently loading your data, transforming, slicing, and preparing it, to creating plots and customising their styles.
All this is **configuration-based**, meaning you can generate plots from the comfort of a :code:`.yml` file.
This has several advantages, one of the most important being that it allows you to **recreate plots later on**.
It can be incredibly frustrating trying to figure out which specific settings or code snippets produced some specific plot if you didn't note down the exact configuration.
The configuration-based plotting approach eliminates this difficulty, since all settings are always saved alongside the plot.

As always, Utopia embraces flexibility for developers, and you can of course fully customize the framework to your own needs, even turning it off and using your own plot functions entirely.
However, especially for large simulation runs with many parameters, you will soon find that it becomes tricky to keep track of all the dimensions, parameters, subspaces, and complicated operations you may wish to perform on the data, and this is exactly what Utopia does for you!

This section will give you a brief guide through Utopia's data transformation and data plotting capabilities.
We will begin by using several of the most :ref:`common and ubiquitous plots <line_plots>` to show you how to use it, before moving to more sophisticated, :ref:`high-dimensional operations <facet_grids>`, :ref:`animations <plot_animations>`, and :ref:`network <plot_networks>` plots.
We will be illustrating everything using the Utopia :ref:`SEIRD <model_SEIRD>`, :ref:`Opinionet <model_Opinionet>` and :ref:`ForestFire <model_ForestFire>` models as examples.



.. _plot_cfg_overview:

Plot Configurations
-------------------
As with simulation runs, Utopia uses a configuration-based approach with the aim of keeping as much information about how a plot should be created in the configuration, rather than in a specific implementation.
This makes plotting much simpler and facilitates automation.

The term *plot configuration* refers to the set of parameters required to create a *single* plot.
We will show you several examples of plot configurations in the following sections; however, the basic structure will often looks something like this:

.. code-block:: yaml

    my_plot:

      based_on:
        - my_base_plot
        - some_style

      select:
        parameter1: path/to/param1
        parameter2: path/to/param2

      style:
        text.usetex: True
        figure.figsize: [7, 4]
        grid.linewidth: 0.5
        font.fontsize: 10

      helpers:
        set_labels:
          x: iteration
        set_scales:
          x: log

What does this all mean?

* ``based_on``: Utopia uses `plot configuration inheritance <https://dantro.readthedocs.io/en/latest/plotting/plot_manager.html#plot-configuration-inheritance>`_, meaning that new plots can be based on existing ones.
  This is useful because often, you will want to create plots that are all similar, differing only in e.g. the parameters you're plotting or the axis labels.
  Plot configuration inheritance makes it very easy to do so without copy-pasting entire configurations: you simply specify the base plot, and only include those aspects that you want to change.
  In our example, the plot is based on :code:`my_base_plot`, and some default plot style (e.g. a color palette).
  Customising plot styles will be detailed in :ref:`plot_style`.

* ``select``: Here you can specify the data you want to plot.

* ``style``: Add some additional style parameters, for example use Latex, or set a specific figure size.
  This is particularly useful when preparing images for inclusing in LaTeX document, as you can set the figure and font sizes to match your document's settings.
  In :ref:`plot_style`, you will also see how to define such things *globally*, so that they are applied across all plots, and you only need to define figure sizes, colors, font sizes, etc., *once*.

* ``helpers``: The Utopia :py:class:`~utopya.eval.plothelper.PlotHelper` helps you further customize the plot, for example by adding axis
  labels.
  See :ref:`plot_helper` for more information.

The base configurations can be configurations for your own plots, but Utopia actually already implements a large number of commonly used plots (such as lineplots, errorbars, heatmaps, histograms, etc.), meaning you
do not need to implement them yourself.
For a list of all base plot configurations provided by utopya, see :ref:`the base plots reference <utopia_base_plots_ref>`.

The inheritance feature **recursively overwrites** settings from plots further down in the hierarchy.
Settings you specify in the top-level (``my_plot`` in the example above) always take precedent over entries in lower levels.

.. hint::

    The plot configuration inheritance feature is completely `implemented in dantro <https://dantro.readthedocs.io/en/latest/plotting/plot_manager.html#plot-configuration-inheritance>`_.


.. admonition:: Sorting plots into subdirectories

    Plot names may also include ``/`` to create subdirectories.
    For example, you may want to plot multiple different phase diagrams; a handy way to sort your plot outputs would then be:

    .. code-block:: yaml

        phase_diagrams/plot1:
          # ..

        phase_diagrams/plot2:
          # ..

    Two plots ``plot1`` and ``plot2`` will be saved in a subfolder called ``phase_diagrams`` in your output directory.
    This can be useful when you're creating a large number of plots.


.. _custom_base_config_pools:

Custom or additional base config pools
""""""""""""""""""""""""""""""""""""""
Which base config pools are used can be adjusted via the :py:class:`~utopya.multiverse.Multiverse` meta-configuration.

For Utopia, the following base configuration pools are made available:

* The `default base configuration pool <https://dantro.readthedocs.io/en/latest/plotting/base_plots.html>`_
* The ``{model_name}_base`` configuration pool for the currently selected model, if available.

This allows introducing additional configuration pools, thereby allowing a more versatile plot configuration inheritance.
For instance, an additional base config pool may be used to adjust a commonly used style, which can be very helpful when desiring to create publication-ready figures without redundantly defining plots.

By default, these four pools are available.
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


Configuration sets
""""""""""""""""""
Same as run configurations, plot configurations can also be included in :ref:`Configuration Sets <config_sets>`, simply by adding an ``eval.yml`` file to the configuration set directory.
This allows to define plot configurations for a specific simulation run, directly alongside it.

To avoid excessive duplication of plot configurations when adding config sets, make use of the plot configuration inheritance discussed above:

* Put shared definitions into the model's *base* plots configuration.
* In the config set's ``eval.yml``, only specify those options that *deviate* from the default or that should better be explicitly specified.



.. _pyplot_plot_creator_DAG_support:

The Data Transformation Framework
---------------------------------

.. hint::

    See the `dantro documentation of the DAG transformation framework <https://dantro.readthedocs.io/en/stable/data_io/transform.html>`_ for a complete guide on the data transformation framework.
    The dantro documentation also includes :ref:`a page about the integration of the DAG into the plotting framework <plot_creator_dag>`.

As part of :py:mod:`dantro`, a `data selection and transformation framework <https://dantro.readthedocs.io/en/latest/data_io/transform.html>`_ based on a directed, acyclic graph (*short*: DAG) of operations is available.
This is a powerful tool, especially when combined with the plotting framework.

The central idea is that plotting and data transformation should be separate:
Having a very long and intricate function that both slices and dices your data *and* plots it is inconvenient and error-prone.
With the DAG, the plot function focuses on the visualization of some data; everything else before (data selection, transformation, etc.) and after (adjusting plot aesthetics, saving the plot, etc.) is automated.

The DAG allows for arbitrary operations, making it a highly versatile and powerful framework.
It uses a configuration-based syntax that is optimized for specification via YAML.
Additionally, **it allows to cache results to a file**; this is very useful when the analysis of data takes much longer than the plotting itself.

Here is a simple example:
Let's say we performed a parameter sweep of our model over some parameter :code:`param`, each time measuring the response in the :code:`state` of our agents; now, we may want to plot the average :code:`state` as a function of :code:`param`.
A simple-enough operation: first, we **get the data**, i.e. the model output at the final time:

.. code-block:: yaml

    my_plot:
      based_on:
        - .creator.multiverse
        - .plot.facet_grid.errorbars

      select_and_combine:
        fields:
          state:
            path: path/to/data
            transform:
              - .isel: [!dag_prev , {time: -1}]

Taking it from the top: first, we are basing our plot on the pre-implemented ``multiverse.errorbars`` plot.
Then, we need to ``select_and_combine`` the data needed for this plot.
In this case, it is the sweep dimensions (the x-axis of our plot), given by the values of ``param1`` we are sweeping over.
Next, we get the final state of our agents, i.e. the ``state`` at ``time: -1``.

The ``!dag_prev`` flag is used by the DAG to point it to the previous node in the chain of operations it performs; we will see more examples of this later on.

Now that we have the data, **we need to transform it**. Simple:

.. code-block:: yaml

    my_plot:

      # all the previous entries ...

      transform:
        # Get the x-coordinate, in this case 'param'
        - .coords: [!dag_tag state , param]
          tag: x_axis

        # Calculate the mean
        - .mean: [!dag_tag state]
          tag: mean_state

        # Calculate the standard deviation
        - .std: [!dag_tag state]
          tag: variance_state

        # Bundle everything together and tag it
        - xr.Dataset:
            data_vars:
               x: !dag_tag x_axis
               y: !dag_tag mean_state
               dy: !dag_tag state_variance
          tag: data

And that's it!
We have created a dataset containing ``param1`` on the x-axis, and the mean and standard deviation of ``state`` on the y-axis, ready for plotting.
A full example of this is given in the section on :ref:`errorbars <errorbars>`.

Noticed the ``!dag_tag`` s?
Like ``!dag_prev`` , these are references to specific *labelled* nodes of the transformation graph, telling it which elements to use for which argument.
To create a labelled node, simply add the ``tag`` key to the transformation.

At the end of all your operations, you must have a transformation that is labelled with ``tag: data``; this is the data that the plot function expects.
Remember, the plot function won't be aware of any of these operations; its job is only to visualize the final output given the result of the transformation operations.

We will see more sophisticated uses of the DAG as we move through the tutorial.
The DAG supplies `many transformation operations <https://dantro.readthedocs.io/en/latest/data_io/data_ops_ref.html>`_; however, if you are missing an operation, you can always :ref:`add your own operation <custom_DAG_ops>`.
