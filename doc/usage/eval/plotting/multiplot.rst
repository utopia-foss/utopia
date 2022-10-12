.. _multiplot:

Multiplots
==========

.. admonition:: Summary

    On this page, you will see how to

    * use ``.plot.multiplot`` to plot different plot kinds onto individual axes of a single figure
    * use ``.plot.multiplot`` to plot different plot kinds onto a single axis
    * use the ``PlotHelper`` adjust the style of the figure and each individual axis

.. admonition:: Complete example: seaborn kde multiplot
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- multiplot
        :end-before: ### End --- multiplot

The :py:func:`~dantro.plot.funcs.multiplot.multiplot` is a highly versatile yet simple way of plotting different types of plots onto a single figure.
Essentially, the plot function allows to invoke arbitrary functions on the individual subplots of a figure, with data supplied via the plot configuration or the data transformation framework.

In the example below, we are plotting an errorbar plot and a scatter plot onto a single subplot. The errorbar shows the
density of susceptible agents with a standard deviation, and the scattered dots display the recovered agent density,
with the density of infected agents as the hue:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/multiplot.pdf
  :width: 800
  :alt: caption

Perhaps a little much for a single frame: but hopefully it illustrates the capability of multiplotting!
Such plots are simply achieved using the ``.plot.multiplot`` function, an overview of which is given in the following.
Refer to the `dantro multiplot documentation <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#dag-multiplot>`_
for further details.

Example: Double kdeplot
^^^^^^^^^^^^^^^^^^^^^^^

Let us go through an example in detail. We wish to plot two smoothed densities showing the maximum peaks of infection
for different values of the transmissivity in a single figure:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/double_kdeplot.pdf
  :width: 800
  :alt: Double kdeplot

Here, we are using a `seaborn function <https://seaborn.pydata.org/generated/seaborn.kdeplot.html>`_.
On the left, we see the distribution of infection peaks for a lower transmissivity (0.4), on the right, for a higher
transmissivity (0.6). Naturally, as the transmissivity increases, so does the height of peak infection.

To create this plot, first select the data and transform it accordingly:

.. code-block:: yaml

  double_kdeplot:

    # Base the plot on a creator (universe or multiverse) and .plot.multiplot:
    based_on:
      - .creator.multiverse
      - .plot.multiplot

    select_and_combine:
      fields:
        infected:
          path: densities
          transform:
            - .sel: [!dag_prev , {kind: infected}]

    # Calculate the maxima of the infection densities for two different
    # transmission rates
    transform:
      - .sel: [!dag_tag infected, {transmission rate: 0.4}]
      - np.max: [!dag_prev , 2]
      - .squeeze: !dag_prev
        tag: data_low

      - .sel: [!dag_tag infected, {transmission rate: 0.6}]
      - np.max: [!dag_prev , 2]
      - .squeeze: !dag_prev
        tag: data_high

Naturally, this requires a sweep to have been performed over a ``transmission rate`` variable.
The ``.squeeze`` transformation is required since we will be using the :py:func`seaborn.kdeplot` function,
which expects a flat array.

Next, distribute the plots onto the axes. This requires setting up the figure accordingly using the :ref:`PlotHelper <plot_helper>`:

.. code-block:: yaml

  double_kdeplot:

    # Everything as above ...

    # Distribute the plots
    to_plot:
      [0, 0]:                               # select the axis,
        - function: sns.kdeplot             # the plot function,
          data: !dag_result data_low        # the data,
          label: $p_\mathrm{transmit}=0.4$  # and add any kwargs the plot function accepts
          fill: true
      [1, 0]:
        - function: sns.kdeplot
          data: !dag_result data_high
          label: $p_\mathrm{transmit}=0.6$
          fill: true
    color: darkblue
    helper:
      setup_figure:
        ncols: 2

.. note::

  We are using ``!dag_result`` `placeholders <https://dantro.readthedocs.io/en/latest/plotting/plot_data_selection.html?highlight=!dag_result#using-data-transformation-results-in-the-plot-configuration>`_
  rather than ``!dag_tag`` to pass data to the plot functions.
  This will however raise a small warning saying that many of the calculated dag tags are being ignored. To suppress
  this, you can exclude tags that are not used in the transformation DAG by removing them from the ``compute_only``
  key in the plot configuration; for instance, above you can do

  .. code-block:: yaml

    compute_only: []

Multiple figures in a single axis
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can of course also plot multiple plots on a single axis; in this case, there is no need to setup the figure, and you
also do not need to specify the axis on which to plot (since there is only one):

.. code-block:: yaml

  double_kdeplot:
    # Everything as above ...

    # Distribute the plots
    to_plot:
      - function: sns.kdeplot
        data: !dag_result data_low
        label: $p_\mathrm{transmit}=0.4$
        linestyle: dashed
        color: red
      - function: sns.kdeplot
        data: !dag_result data_high
        label: $p_\mathrm{transmit}=0.6$
        color: yellow

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/single_kdeplot.pdf
  :width: 400
  :alt: Single kdeplot

Importing callables on the fly
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Many matplotlib and seaborn plots are available with a simple call, as above; see
`here <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#multiplot-plot-multiple-functions-on-one-axis>`_
for a list of all available functions. However, you can also *import* callables on the fly by passing a 2-tuple of
``(module, name)`` to ``function``:

.. code-block:: yaml

  plot:
    to_plot:
      [0, 0]:
        - function: [xarray.plot, scatter]
          # args, kwargs here ...


Modifying figure-level and axis-specific features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let us now go about modifying the plot appearance somewhat. To control the color, notice how in the
previous example we set

.. code-block:: yaml

  double_kdeplot:

    # Everything as above ...

    color: darkblue

This makes ``color`` a *shared* keyword argument, one that is passed to *all* plot function calls.
Specifying arguments within the ``to_plot`` entries can still overwrite the entries given on the shared level.

.. code-block:: yaml

  double_kdeplot:

    # Everything as above ...

    to_plot:
      - function: sns.kdeplot
        data: !dag_result data_low
        color: red
        linestyle: dashed
      - function: sns.kdeplot
        data: !dag_result data_low
        color: yellow

This gives you complete control over the appearance of each individual plot.

With the :ref:`plot helper framework <plot_helper>`, you can control such things as shared axes, the spacing between subplots, limits, and axis scales.
For instance, to modify horizontal or vertical distancing between plots and have figures share their axes, do

.. code-block:: yaml

  helper:
    # set the number of rows and columns with shared axes
    setup_figure:
      ncols: 2
      nrows: 3
      sharex: true
      sharey: true

    # adjust horizontal and vertical spacing between subplots
    subplots_adjust:
      hspace: 0.1
      wspace: 0.3

The ``subplots_adjust`` entries are passed to :py:meth:`matplotlib.figure.Figure.subplots_adjust`, whereas
``setup_figure`` is passed to :py:func:`matplotlib.pyplot.subplots`.

The :ref:`PlotHelper <plot_helper>` gives you a variety of options to adjust the plot appearance. You can choose to
apply these to the entire plot, or only individual axes. For instance,

.. code-block:: yaml

  multiplot:
    helper:
      set_limits:
        x: [0, 1]

will set the x limits on *all* axes to [0, 1]. You can use the ``axis_specific`` helper to only modify certain axes:

.. code-block:: yaml

  multiplot:
    helper:
      axis_specific:
        axis1:            # some arbitrary axis name
          axis: [0, 0]    # the x and y coordinates of the subplot in the plot
          set_limits:
            x: [0, 1]
        axis2:
          axis: [1, 0]
          set_limits:
            x: [-1, 0]

All helper functions are available under ``axis_specific``, giving you complete control over the appearance of the
individual axes. Furthermore, helpers specified on the top level apply to all axes.
