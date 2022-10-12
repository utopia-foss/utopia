.. _histograms:

Histograms
==========

.. admonition:: Summary

    On this page, you will see how to

    * use ``.plot.facet_grid.hist`` to plot histograms
    * use ``.plot.multiplot`` to plot panels of histograms

Histograms are one of the most commonly used plots to visualise distributions.
Let us run the :ref:`SEIRD <model_SEIRD>` model with a fixed configuration multiple times over different seeds, and plot a histogram of the maximum peak height of the number of infected agents.

To plot a histogram, base your plot on ``.plot.facet_grid.hist``.
We can use the ``np.max`` operation in the data transformation process to select the maximum density for each run:

.. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
    :language: yaml
    :dedent: 0
    :start-after: ### Start --- histogram
    :end-before: ### End --- histogram

Any additional entries are keyword arguments that will be passed to the low-level plotting function, in this case `matplotlib.pyplot.hist <https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.hist.html>`_.
For example, we can specify the number of bins and color by adding

.. code-block:: yaml

    histogram:

      # all the previous entries ...

      color: mediumseagreen
      bins: 50

The output will look something like this:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/histogram.pdf
  :width: 800
  :alt: Single histogram

Plotting facetted histograms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For facetted histograms, use the :ref:`multiplot <multiplot>` functionality. Here, we are plotting the distribution of the
peak of infection in the top plot, and the minimum number of susceptible agents in the bottom plot:

.. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
    :language: yaml
    :dedent: 0
    :start-after: ### Start --- double_histogram
    :end-before: ### End --- double_histogram

This will produce a plot like this:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/double_histogram.pdf
  :width: 800
  :alt: Double histogram
