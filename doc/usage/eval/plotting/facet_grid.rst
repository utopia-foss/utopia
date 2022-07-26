.. _facet_grids:

Stacked plots and Facet grids
=============================

.. admonition:: Summary

  On this page, you will see how to

  * use ``.plot.facet_grid.line`` and  ``.plot.facet_grid.errorbands`` to stack multiple
    lines in a single plot, using ``hue`` to differentiate them
  * plot multiple panels in a single image, using the ``hue``, ``row``, and ``col`` keys.
  * use ``.plot.facet_grid.with_auto_encoding`` to automatically distribute variables onto
    available plot dimensions
  * use ``col_wrap: auto`` to automatically make the plot squarer

.. admonition:: Complete example: Stacked Errorbands
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- stacked_errorbands
        :end-before: ### End --- stacked_errorbands

.. admonition:: Complete example: Facet grid
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- facet_grid
        :end-before: ### End --- facet_grid

In the previous section we plotted a single line; now let's see how to plot multiple lines with legends, all in
the same plot. Using our SEIRD model, let's look at how the susceptible, infected, and recovered populations evolve
together:

.. code-block:: yaml

    densities:

      based_on:
        - .creator.universe
        - .plot.facet_grid.line

      select:
        data:
          path: data/SEIRD/densities
          transform:
            - .sel: [ !dag_prev , { kind: ['susceptible', 'infected', 'recovered'] }]

      x: time

Now that we've added more than one y-value, we need to tell the line-plot what to put on the x-axis (:code:`x: time`).
Instead of specifying the x-value, we could also tell it what the color represents – the two are equivalent.
To do this, simply replace the following line:

.. code-block:: yaml

   # x: time
   hue: kind

In both cases, we get something like this:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/stacked_kinds.pdf
  :width: 800
  :alt: caption

We used latex and some pretty colours to spruce everything up – see :ref:`plot_style` for more details.

Stacked line plot with one sweep dimension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let's compare the infection curves for three different values of the transmission rate :code:`p_transmit` of the virus.

.. code-block:: yaml

  infection_curves:

    based_on:
      - .creator.multiverse
      - .plot.facet_grid.line

    select_and_combine:
      fields:
        data:
          path: data/SEIRD/densities
          transform:
            - .sel: [ !dag_prev , { kind: [ 'infected' ] }]

    x: time

Since this is a multiverse plot, we must use the corresponding :code:`creator`, and use the :code:`select_and_combine`
key to gather the data. In this example, :code:`transform` block only adds a :code:`data` tag to the data, without
performing any actual transformation operations.

.. note::

    For ``facet_grid`` plots, the ``data`` tag must always be defined,
    even when not applying any sort of transformation. Here, we are
    defining the ``data`` tag in the ``select`` step.
    Other plot functions may have different requirements.

This produces the following output:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/stacked_transmission.pdf
  :width: 800
  :alt: Stacked density plot

Unsurprisingly, we see the peak of infection increasing as the virus becomes more transmissible.

Stacked line plot with one sweep dimension and statistics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let's do the same thing, but with each infection curve representing an average over a few simulation runs with different
initial seeds. This assumes that we have performed a two-dimensional multiverse run, sweeping over both the :code:`seed`
and the transmission rate :code:`p_transmit`. The only thing we need to change from the previous entry is the
:code:`transform` block:

.. code-block:: yaml

   transform:
     - operation: .mean
       args: [ !dag_tag infected ]
       kwargs:
         dim: seed
       tag: data

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/stacked_transmission_averaged.pdf
  :width: 800
  :alt: Stacked density plot

This would be much more meaningful if we could add errorbands to each of the curves, so let's do that:

.. code-block:: yaml

  infection_curves_averaged:

    # Use the errorbands function!
    based_on:
      - .creator.multiverse
      - .plot.facet_grid.errorbands

    # This is the same as above
    select_and_combine:
      fields:
        infected:
          path: densities
          transform:
            - .sel: [ !dag_prev , { kind: [ 'infected' ] }]

    # Calculate mean and standard deviation along the 'seed' dimension
    transform:
      - operation: .mean
        args: [ !dag_tag infected ]
        kwargs:
          dim: seed
        tag: mean
      - operation: .std
        args: [ !dag_tag infected ]
        kwargs:
          dim: seed
        tag: std
      - operation: xr.Dataset
        kwargs:
          data_vars:
            infected density: !dag_tag mean
            err: !dag_tag std
        tag: data

    x: time
    y: infected density
    yerr: err
    hue: transmission rate

Because the data is two-dimensional, we need to tell the plot function what to put on the x-axis, and what to stack:
that's why need both the :code:`hue` and :code:`x` keys. Make sure to adjust the :code:`hue` key to whatever you
named your sweep dimension!

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/stacked_errorbands.pdf
  :width: 800
  :alt: Stacked density plot

.. _facet_grid_panels:

Facet grids
^^^^^^^^^^^

The stacked line plots we have just discussed are examples of **facet grids**.
Facet grids are a simple way of visualising the results of parameter sweeps
in a single image, either by showing several plots in a single frame, or by
combining several frames into single image. Showing several panels in a single image can be
useful when there are simply too many variables for a single plot, or when plotting everything on a single
would clutter the plot. In such situations, you may wish to produce something like this:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/panel_errorbands.pdf
  :width: 800
  :alt: A facet grid example

Here, we are showing the output of a 3-dimensional parameter sweep: we are sweeping over the :code:`transmission rate`,
over the :code:`immunity rate`, and over the initial seed. Each panel shows the density of infected agents over time
(x-axis), with the transmission rate on the rows, and the immunity rate on the columns. Within each panel,
we are averaging over the :code:`seed` and producing an errorband plot, using the :code:`.plots.facet_grid.errorbands`
function.

Generating this plot is a simple modification away from our previous configuration; all we need to do is to
divide up our variables amongst the rows and columns, using the :code:`row` and :code:`col` keys:

.. code-block:: yaml

  infection_curves_averaged:

    # Same as above
    based_on:
      - .creator.multiverse
      - .plot.facet_grid.errorbands

    select_and_combine:
    # also same as above...

    transform:
    # same as above ...

    x: time
    y: infected density
    yerr: err
    row: transmission rate
    col: immunity rate

    color: crimson
    helpers:
      set_limits:
         y: [0, 0.2]

The transformation framework takes care of everything else. Notice that we have set the y-limits to all be equal,
so that we can compare the curves at a single glance.

The :code:`facet_grid` allows us to simultaneously plot parameters onto rows, columns, the y-axis, and also make
use of the hue; let's additionally include the susceptible and recovered agents in our plots:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/panel_all.pdf
  :width: 800
  :alt: A facet grid example

A little crowded perhaps, but we get the idea. All this requires is to include the other two :code:`kinds`
in our selection, and to set the :code:`hue` key:

.. code-block:: yaml

  infection_curves_averaged:

    based_on:
    # Same as above

    # Also select the other kinds:
    select_and_combine:
      fields:
        infected:
          path: densities
          transform:
          - .sel: [ !dag_prev , { kind: [ 'susceptible', 'infected', 'recovered' ] }]

    transform:
    # same as above ...

    x: time
    y: infected density
    yerr: err
    row: transmission rate
    col: immunity rate
    hue: kind

    helpers:
      set_limits:
         y: [0, 0.6]

Note that the legend and row and column titles are automatically plotted.

.. hint::

    You may sometimes not want to plot *all* values of a parameter; for example, for the plot above, we may just be
    interested in :code:`immunity rate = 0, 0.1, 0.2`, and :code:`transmission rate = 0.2, 0.4`. This is easily
    achieved using :ref:`subspace selection <plot_subspaces>`.

.. hint::

    If you have lots of columns and few rows, use ``col_wrap: auto`` to create a more square plot.

Auto-encoding
^^^^^^^^^^^^^
If you don't care which variables go where, you can include the ``.plot.facet_grid.with_auto_encoding`` modifier into your plot:

.. code-block:: yaml

    based_on:
      # ...
      - .plot.facet_grid.with_auto_encoding
      # ...

This will automatically distribute the variables onto any available dimensions.
Variables will be distributed in a certain order, see the :py:func:`~dantro.plot.funcs.generic.determine_encoding` dantro function.

.. hint::

    Automatically determining the encoding can be useful if you want to implement more generic plots that do not depend so much on the dimensionality of your multiverse simulation run.
    They are best suited for getting an overview of your simulation results.

    However, if you want to be sure that a specific variable is represented in a certain way, it's best to specify the encoding (``x``, ``col``, ...) explicitly.
    For publication-ready figures, this explicit definition is more suited.

To further control in which order dimensions are populated, you can pass a dict to the ``auto_encoding`` argument (instead of a boolean):

.. code-block:: yaml

    based_on:
      # ...
      - .plot.facet_grid.line
      - .plot.facet_grid.with_auto_encoding
      # ...

    # change the order in which encodings are populated
    auto_encoding:
      line: [x, col, hue, frames, row]  # default: [x, hue, col, row, frames]

.. hint::

    The ``.plot.facet_grid.with_auto_encoding`` base config also sets the ``col_wrap: auto`` argument, which aims to make facet grid plots with many subplots more square by wrapping after ``sqrt(num_cols)``.
    This is ignored if the ``row`` encoding is specified.
