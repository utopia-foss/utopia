Analysis of the attractor
=========================

.. automodule:: utopya.plot_funcs.attractor

----

Bifurcation in on parameter dimension
-------------------------------------

For a bifurcation diagram in a single parameter dimension, there is the 
``attractor.bifurcation_diagram`` method. It takes a Sequence of analysis steps
of type Tuple(str, str): attractor_key, function name or str: key from default
analysis steps. The function performs an analysis of the data returning 
Tuple(bool, xr.DataArray): conclusive analysis, result. The attractor_key maps
this to the plotting of the attractor and resolves how to visualize this
dataset.

.. autofunction:: utopya.plot_funcs.attractor.bifurcation_diagram

The default analysis steps are listed here

.. literalinclude:: ../../../python/utopya/test/cfg/test_plotting__bifurcation_diagram__plots_cfg.yml
    :language: yaml


