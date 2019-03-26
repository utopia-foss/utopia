``Geomorphology`` Model
=======================

This is a very simple implementation of a geomorphology model, combining erosion due to rainfall and tectonic uplift. It is implemented as a stochastic cellular automaton on a grid, where the state of each cell :math:`i` consists of the height :math:`h_{t,i}` (double) and the watercontent :math:`w_{t,i}` (double). In the beginning, the heights of the cells represent a (disretized) inclined plane. Rainfall, implemented as a gauss-distributed random number drawn for each cell, erodes sediments over time. Due to the stochastic nature of the rainfall, a network of rivers emerges. 

Model parameters
----------------

* mean rainfall :math:`\langle r \rangle`
* rainfall standard deviation :math:`\sigma_r`
* initial height offset :math:`\Delta h`
* initial slope :math:`s`
* erodibility :math:`e`
* uplift :math:`u`

Initial Configuration
^^^^^^^^^^^^^^^^^^^^^

In order to observe the formation of a river network, the cell heights are initialized as an inclined plane. Denoting the coordinates of a cell :math:`i` as :math:`x_i, y_i`, its initial height is given by

.. math::
    h_{0,i} = \Delta h + s \cdot y_i.

The model parameter :math:`\Delta h` is introduced in order to avoid dealing with negative heights and best chosen large (with respect to the other parameters).

The water contents :math:`w_{0,i}` are :math:`0` everywhere in the beginning.

Erosion
^^^^^^^

The erosion process is implemented by synchronously updating all the cells (in one time step :math:`t`) according to the following steps:

1. Rainfall: For each cell  :math:`i`, a random number :math:`r_{t,i} \sim \mathcal{N}(\langle r \rangle, \sigma_r)` (where :math:`\mathcal{N}` is the normal (Gaussian) distribution) is drawn and added to the watercontent :math:`w_{t,i}` of that cell.

2. Sediment Flow: For each cell :math:`i` that has a positive watercontent :math:`w_{t,i}` and has at least one neighbouring cell :math:`j` with a lower height :math:`h_{t,j} < h_{t,i}`, the sediment flow from cell :math:`i` to its lowest neighbor is determined as

    .. math::
        \Delta s_{i,j} = e \cdot (h_{t,i} - h_{t,j}) \cdot \sqrt{w_{t,i}}.

    The sediment flow is taken into account for the state of cell :math:`i` at time :math:`t+1` by updating

    :math:`h_{t+1, i} -= \Delta s_{i,j}.`
    
    In case the new height of cell :math:`i` (after considering all inflows from/outflows to all neighbours) is negative, it is cut off at :math:`0`.
    
    Special case: The height of the cells on the lower boundary (the boundary, where the cell heights are lowest in the initial configuration) is always decreased by :math:`e \cdot \sqrt{w_{t,i}}`.
    This constant outflow (constant meaning independent of the height difference to the lowest neighbour) prevents lakes from forming. We want to avoid this here, for now, since consistently modelling lake formation is a non-trivial problem (see e.g. the bachelor thesis of Julian Weninger and the master thesis of Hendrik Leusmann)
    
3. Water Flow: After the new heights for all cells have been determined (but not yet set), the water of all cells is moved to their lowest neighbour (if the cell is not on the boundary and has a lowest neighbour).

4. Uplift: Finally, the height of each cell is incremented by the constant :math:`u` that represents uplift.

Default parameters
^^^^^^^^^^^^^^^^^^

Below are the default configuration parameters for the ``Geomorphology`` model.

.. literalinclude:: ../../src/models/Geomorphology/Geomorphology_cfg.yml
   :language: yaml
   :start-after: ---
