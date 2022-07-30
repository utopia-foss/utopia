.. _plot_sweep_cfgs:

Sweeps in Plot Configs
======================

.. admonition:: Summary

    On this page, you will see how to

    * add the ``!pspace`` tag to plots in order to plot the same figure multiple times with different settings

Sometimes you may want to plot a bunch of very similar plots that differ only in a few configuration entries.
Plot configuration entries can also make use of parameter sweeps.
Simply add the ``!pspace`` tag to the top-level entry:

.. code-block:: yaml

    my_plot: !pspace
      some_param: !sweep
        default: foo
        values: [foo, bar, baz]

This will automatically create a separate file for each plot and include the value of the parameter into the file or folder name.
You can also give the plots more meaningful names by adding a ``name`` entry to a sweep dimension:

.. code-block:: yaml

    my_plot: !pspace
      some_param: !sweep
        default: foo
        values: [foo, bar, baz]
        name: parameter_name

This feature is built on top of the :py:class:`~paramspace.paramspace.ParamSpace` class, which is instantiated via the ``!pspace`` tag.
The ``!sweep`` tag corresponds to :py:class:`~paramspace.paramdim.ParamDim`, ``!coupled-sweep`` to :py:class:`~paramspace.paramdim.CoupledParamDim`.
