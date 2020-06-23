Generic Plots
=============

You can directly use generic plot functions provided by dantro.
For a detailed description visit `the plot function section in the dantro documentation <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#plot-functions>`_.

.. contents::
   :local:
   :depth: 2

----

Errorbar and Errorbands
-----------------------
For errorbar plots, two options are available: the errorbar plot uses bars to represent a symmetric confidence interval; the errorband plot uses a shaded area.
The corresponding base plot configurations are called ``.dag.generic.errorbar`` and ``.dag.generic.errorbands``.

For more information, refer to the docstrings below and to `the dantro documentation <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#errorbar-and-errorbands-visualizing-confidence-intervals>`_.

.. autofunction:: dantro.plot_creators.ext_funcs.generic.errorbar

.. autofunction:: dantro.plot_creators.ext_funcs.generic.errorbands

----

Facet Grid
----------
Faceting plotting functions are able to represent high-dimensional data using subplots and animations.
Utopia makes use of the ``facet_grid`` plotting function implemented in dantro.
It is accessible via the base configuration key ``.dag.generic.facet_grid``.

For more information, refer to the docstrings below and to `the dantro documentation <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#facet-grid-a-declarative-generic-plot-function>`_.

.. autofunction:: dantro.plot_creators.ext_funcs.generic.facet_grid
