# A simple geomorphology model

This is a very simple implementation of a geomorphology model, combining erosion due to rainfall and tectonic uplift. It is implemented as a stochastic cellular automaton on a grid, where the state of each cell $`i`$ consists of the height $`h_{t,i}`$ (double) and the watercontent $`w_{t,i}`$ (double). In the beginning, the heights of the cells represent a (disretized) inclined plane. Rainfall, implemented as a gauss-distributed random number drawn for each cell, erodes sediments over time. Due to the stochastic nature of the rainfall, a network of rivers emerges. 

## Model parameters

* mean rainfall $`\langle r \rangle`$
* rainfall standard deviation $`\sigma_r`$
* initial height offset $`\Delta h`$
* initial slope $`s`$
* erodibility $`e`$
* uplift $`u`$

## Initial Configuration

In order to observe the formation of a river network, the cell heights are initialized as an inclined plane. Denoting the coordinates of a cell $`i`$ as $`x_i, y_i`$, its initial height is given by

$`h_{0,i} = \Delta h + s \cdot y_i.`$

The model parameter $`\Delta h`$ is introduced in order to avoid dealing with negative heights and best chosen large (with respect to the other parameters).

The water contents $`w_{0,i}`$ are $`0`$ everywhere in the beginning.

## Erosion

The erosion process is implemented by synchronously updating all the cells (in one time step $`t`$) according to the following steps:
1. Rainfall: For each cell  $`i`$, a random number $`r_{t,i} \sim \mathcal{N}(\langle r \rangle, \sigma_r)`$ (where $`\mathcal{N}`$ is the normal (Gaussian) distribution) is drawn and added to the watercontent $`w_{t,i}`$ of that cell.
2. Sediment Flow: For each cell $`i`$ that has a positive watercontent $`w_{t,i}`$ and has at least one neighbouring cell $`j`$ with a lower height $`h_{t,j} < h_{t,i}`$, the sediment flow from cell $`i`$ to its lowest neighbor is determined as

    $`\Delta s_{i,j} = e \cdot (h_{t,i} - h_{t,j}) \cdot \sqrt{w_{t,i}}.`$

    The sediment flow is taken into account for the state of cell $`i`$ at time $`t+1`$ by updating

    $`h_{t+1, i} -= \Delta s_{i,j}.`$
    
    In case the new height of cell $`i`$ (after considering all inflows from/outflows to all neighbours) is negative, it is cut off at $`0`$.
    
    Special case: The height of the cells on the lower boundary (the boundary, where the cell heights are lowest in the initial configuration) is always decreased by $`e \cdot \sqrt{w_{t,i}}`$.
    This constant outflow (constant meaning independent of the height difference to the lowest neighbour) prevents lakes from forming. We want to avoid this here, for now, since consistently modelling lake formation is a non-trivial problem (see e.g. the bachelor thesis of Julian Weninger and the master thesis of Hendrik Leusmann)
    
3. Water Flow: After the new heights for all cells have been determined (but not yet set), the water of all cells is moved to their lowest neighbour (if the cell is not on the boundary and has a lowest neighbour).
4. Uplift: Finally, the height of each cell is incremented by the constant $`u`$ that represents uplift.

## Default parameters

The current default parameters (as defined in the file `geomorpholofy_cfg.yml`) are:

* $`\langle r \rangle = .1`$
* $`\sigma_r = .02`$
* $`h_0 = 1000`$
* $`s = 0.4`$
* $`u = 0.1`$
* $`e = 0.01`$
