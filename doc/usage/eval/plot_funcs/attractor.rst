Analysis of the attractor
=========================

.. automodule:: utopya.plot_funcs.attractor
    :noindex:

.. contents::
    :local:
    :depth: 1

----

Bifurcation in one parameter dimension
--------------------------------------

For a bifurcation diagram in a single parameter dimension, there is the
``attractor.bifurcation_diagram`` method. It takes a Sequence of analysis steps
of type Tuple(str, str): attractor_key, function name or str: key from default
analysis steps. The function performs an analysis of the data returning
Tuple(bool, xr.DataArray): conclusive analysis, result. The attractor_key maps
this to the plotting of the attractor and resolves how to visualize this
dataset.

.. autofunction:: utopya.plot_funcs.attractor.bifurcation_diagram
    :noindex:

The default analysis steps are listed here

.. literalinclude:: ../../../../python/utopya/test/cfg/plots/bifurcation_diagram/plots.yml
    :language: yaml


Bifurcation in two parameter dimensions
---------------------------------------

For a bifurcation diagram in two parameter dimensions, there is the ``attractor.bifurcation_diagram_2d`` method.
As the 1d equivalent it takes a Sequence of analysis steps of type Tuple(str, str): attractor_key, function name or str: key from default analysis steps.
The function performs an analysis of the data returning Tuple(bool, xr.DataArray): conclusive analysis, result.
The attractor_key maps this to the plotting of the attractor and resolves how to visualize this dataset.
Other than in 1d the attractor itself cannot be visualized by its state, hence the previously optional ``visualisation_kwargs`` become obligatory and are passed to ``matplotlib.patches.Rectangle`` with auto detected or passed width and height.

.. note::
    The visualisation_kwarg to 'fixpoint' can have a ``to_plot`` entry that defines kwargs proper to every used data field.
    In that case the field with the heighest fixpoint value defines the coloring.

.. autofunction:: utopya.plot_funcs.attractor.bifurcation_diagram
    :noindex:
