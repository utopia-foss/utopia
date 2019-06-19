``Geomorphology`` Model
=======================

This is a basic implementation of a geomorphology model, combining erosion due to rainfall and toppling with tectonic uplift. It is implemented as a stochastic cellular automaton ton a grid, where the stae of each cell :math:`i` consists of the topological height :math:`h_{t,i}` (double), the drainage area to a cell, and the watercolumn :math:`w_{t,i}` (double). In the beginning, the heights of the cells represent a (disretized) inclined plane. Rainfall is encapsulated in the drainage network representing a river system. Drainage is always passed from one cell to its lowest grid-neighbor; sinks are filled with water (watercolumn) and the overflow receives the drainage upstream from the lake. The higher the drainage area to a cell, the more water flows over this cell. Stream power erodes sediments over time. Additionally, a stochastic process of toppling is considered.


Initial Configuration
^^^^^^^^^^^^^^^^^^^^^

In order to observe the formation of a river network, the cell heights are initialized as an inclined plane. Denoting the coordinates of a cell :math:`i` as :math:`x_i, y_i`, its initial height is given by

.. math::
    h_{0,i} = \Delta h + s \cdot y_i.

Negative topological heights are forbidden here and set to 0, i.e. the normal distribution is cut at 0.

The drainage network is a fictional construct, integrating continuous rainfall over time. It translates to the size and stream power of a river at the cell.

Algorithm
^^^^^^^^^

The erosion process is implemented by asynchronously updating all the cells (in one time step :math:`t`) according to the following steps:

1. Uplift

    Finally, the height of each cell is incremented by the normally distributed :math:`u` that represents uplift.

2. Set drainage network
    
    1. Map all cells to their lowest neighbor, if none is lower than the cell itself, to itself.

    2. Fill sinks (no lower neighbor) with water, such that a lake forms and one of the lake cells has a lower neighbor or is an outflow boundary. All lake cells point to this cell.

    3. Set drainage area. For every cell, pass the cells assigned drainage area (default 1., cummulated with that receved from other already called cells) downstream through the already initialized network (adding the drainage area to every cell on the way) and dump it on any cell passed by, that was not yet called or is an outflow boundary.

3. Stream power erosion
    
    .. math::
        \frac{dz}{dt} = c * \Delta z * \sqrt{A}

    with the rock heigth z, the stream power constant c, and the drainage area A.

4. Toppling

    With a frequency f per cell evaluate the failure probability for slope:

    .. math::
        p = s / h_c

    with the critial height :math:`h_c`. If toppling occurs the slope is reduced to 1/3. of its initial value.

Default parameters
^^^^^^^^^^^^^^^^^^

Below are the default configuration parameters for the ``Geomorphology`` model.

.. literalinclude:: ../../src/utopia/models/Geomorphology/Geomorphology_cfg.yml
   :language: yaml
   :start-after: ---


Further reading
---------------

The model is analyzed in depth in the bachelor thesis of Julian Weninger (2016).

* 
  Weninger, J., 2016. Development of Mountain Ranges and their Rivers - A Cellular Automaton Simulation. Bachelor’s Thesis, IUP (TS&CCEES) at Universität Heidelberg.
