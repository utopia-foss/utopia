.. _plot_subspaces:

Plotting subspaces
==================

.. admonition:: Summary \

  On this page, you will see how to

  * use the ``subspace`` key to select only certain parts of the parameter space for plotting
  * use ``col_wrap: auto`` when plotting ``facet_grid`` panels to automatically square a plot
    with many columns and few rows.

For large parameter sweeps across many dimensions, you may often wish to only select a subspace of the whole
parameter space for plotting. This is easily achieved using the :code:`subspace` key. Subspace selection will also
improve performance speeds, since less data is being loaded into memory.

Consider the :ref:`facet grid example <facet_grid_panels>` we considered in a previous section.
Here, we performed a parameter sweep across three dimensions (``seed``, ``transmission rate``,
``immunity rate``) and plotted the resulting curves of the susceptible, infected, and recovered populations:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/panel_all.pdf
  :width: 800
  :alt: A facet grid example

If we want to restrict ourselves to the subspace :code:`transmission rate = 0.2`
and :code:`immunity rate < 0.3`, do the following:

.. code-block:: yaml

  infection_curves_averaged:

    # Same as before
    based_on:
      - .creator.multiverse
      - .plot.facet_grid.errorbands

    select_and_combine:
      fields:
        infected:
          path: densities
          transform:
            - .sel: [ !dag_prev , { kind: [ 'infected' ] }]

      # Add the following entry:
      subspace:
        transmission rate: [0.2]
        immunity rate: [0, 0.1, 0.2]

    transform:
    # same as before ...

    x: time
    y: infected density
    yerr: yerr
    col: immunity rate
    hue: kind

Note that we are no longer assigning the :code:`row` key, as there is only a single row to plot!

The output then looks like this:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/panel_subspace.pdf
  :width: 800
  :alt: A facet grid example with subspace selection

.. note::

    Even when selecting a single value, the subspace entry needs to an iterable, i.e. an array.

.. hint::

    If you have lots of columns and few rows, use ``col_wrap: auto`` to create a more square plot.