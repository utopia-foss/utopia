.. _plot_sweep_cfgs:

Sweeps in Plot Configs
======================

.. admonition:: Summary

  On this page, you will see how to

  * add the ``!pspace`` tag to plots in order to plot the same figure multiple times with different settings
  * write your plots to a common subfolder

Sometimes you may want to plot a bunch of very similar plots that differ only in a few configuration entries.
Plot configuration entries can also make use of parameter sweeps. Simply add the ``!pspace`` tag to the top-level entry:

.. code-block:: yaml

    my_plot: !pspace
      some_param: !sweep
        default: foo
        values: [foo, bar, baz]

This will automatically create a separate file for each plot and include the value of the parameter into the file or folder name.
You can also give the plots more meaningful names by adding a ``name`` entry:

.. code-block:: yaml

    my_plot: !pspace
      some_param: !sweep
        default: foo
        values: [foo, bar, baz]
        name: parameter_name

You may also find it useful to write plots of a similar kind to a separate folder, in order to have a better overview of your output.
For example, you may want to plot three different phase diagrams; a handy way of achieving is doing

.. code-block:: yaml

    phase_diagrams/plot1:
      # ..

    phase_diagrams/plot2:
      # ..

Two plots ``plot1`` and ``plot2`` will be saved in a subfolder called ``phase_diagrams`` in your output directory.
This can be useful when you're creating a large number of plots.
