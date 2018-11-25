# A simple geomorphology model

This is a very simple implementation of a geomorphology model, combining erosion due to rainfall and tectonic uplift. It is implemented as a stochastic cellular automaton on a grid, where the state of each cell consists of the height (double) and the watercontent (double). In the beginning, the heights of the cells represent a (disretized) inclined plane. Rainfall, implemented as a gauss-distributed random number drawn for each cell, erodes sediments over time. Due to the stochastic nature of the rainfall, a network of rivers emerges. 

## Model parameters

* mean rainfall $`\langle r \rangle`$
* rainfall standard deviation $`\sigma_r`$
* initial height offset $`\Delta h`$
* initial slope $`s`$
* erodibility $`e`$
* uplift $`u`$

## Erosion

The erosion process is implemented by synchronously updating all the cells (in one time step $`t`$) according to the following steps:
1. Rainfall: For each cell  $`i`$, a random number $`r_{t,i}) \sim \mathcal{N}(\langle r \rangle, \sigma_r)`$ is drawn and added to the watercontent of that cell.
2. Sediment Flow: For each cell $`i`$ that has a positive watercontent $`w_{t,i}`$ and has at least one neighbouring cell $`j`$ with a lower height $`h_{t,j} < h_{t,i}`$, the sediment flow from cell $`i`$ to its lowest neighbor is determined as

$`\Delta s_{i,j} = e \cdot (h_{t,i} - h_{t,j}) \cdot w_{t,i}.`$

The sediment flow is taken into account for the state of cell $`i`$ at time $`t+1`$ by updating

$`s_{t+1, i} -= \Delta s_{i,j}.`$

## Behaviour for the default parameters

The current default parameters (as defined in the file `geomorpholofy_cfg.yml`) are:

* $`\langle r \rangle = .1`$
* $`\sigma_r = .02`$
* $`h_0 = 1000`$
* $`s = 0.4`$
* $`u = 0.1`$
* $`e = 0.01`$

For these parameters and a grid size of $`200 \times 200`$ 
