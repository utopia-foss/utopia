# Contagious Disease Model

This is a simple model of a contagious disease on a 2D-grid. It is based
on the description in the script of the CCEES lecture of Prof. Roth.

## Fundamentals

We model a "forest" on a two dimensional square grid of cells. Each cell can be
in one of four different states: empty, tree, infected or herd.

#Update Rules

Each time step the cells update their respective states according to the
following rules:

1: Infected --> Empty
2: Empty    --> Tree
                -with a probability p_growth
3: Tree     --> Infected
                -with the probability p_infect for each infected
                or herd cell in the neighbourhood
                -with probability p_rd_infect for random-point infections

The model uses the Van Neumann neighborhood (NextNeighbor), but it can be
switched out with the Moore neighborhood (MooreNeighbor) fairly easy.

## Infection herds

Infection herds can be activated at the lower boundary of the grid. They spread
infection like normal infected trees, but don't revert back to the empty state.
(like Rule 1).

## Initialization

The default initialization is empty, so all cells are empty. In future randomly
distributed trees could be added as an initialization method, though it doesn't
impact the model outcomes.

## Implementation Details

The implementation is based on the CopyMe and the SimpleEG model.  

## Simulation Results – A Selection Process

The model outputs a plot for each time step of the states on a 2D-grid.

// TODO: Maybe add link to wiki?

## Theoretical Background

// TODO


## Possible Future Extensions

Based on this model, the forest fire model could be implemented.

## References

Kurt Roth: Chaotic, Complex, and Evolving Environmental Systems. Unpublished lecture notes, University of Heidelberg, 2018.
