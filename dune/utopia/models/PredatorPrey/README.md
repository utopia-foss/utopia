# Predator-Prey Model
 
This model is a version of the Predator-prey model from the CCEES lecture
script (v3, chapter 9.3.1). It is implemented as a cellular automaton (CA)
with the cells arranged on a two-dimensional grid and represents a simple
case of population dynamics on a grid.

## Scenario 

Two species are present in this model: the prey species which has a steady
inflow of resources (e.g. by eating plants whose population dynamics are 
not represented in this model) and the predator species that feeds on the 
prey. Both species reproduce in a non-sexual manner. Their interactions 
consists of the predator moving on the grid and looking for prey in its 
neighborhood and eventually eating it upon making contact. The prey flees 
with a certain probability. A general cost is applied to both species to 
uphold their internal structure.

## Implementation

This is modeled using a CA. Each cell of the CA has 4 possible states:

- empty
- prey
- predator
- predator and prey

There cannot be two individuals of the same species on the same cell at the 
same time. Consequently, each cell contains a variable for each species in 
which the resource level of the respective individual is stored. The 
interaction is calculated for each time step and consists of four 
consecutively applied rules:

1. Cost: Resources of each individual are reduced by the cost of living. 
Individuals with negative or zero resources are removed.
2. Movement: Predators move to a cell populated by prey in their 
neighborhood or to an empty cell if there is no prey. Prey that are on a 
cell together with a predator flee to an empty cell in their neighborhood 
with a certain probability. If there are several cells in the neigborhood 
that meet the above condition, one is chosen randomly.
3. Eat: Prey take up resources and predators eat prey if they are on the 
same cell.
4. Reproduction: If an individual's resources exceed a certain value and if 
there is a cell in its neighborhood that is not already populated by an 
individual of the same species, it reproduces and an individual of the same 
species is set on the empty cell. 2 resource units are transferred to the 
descendant.

All cell updates are performed asynchronously to all cells. The order for 
the cell update is random for rules 2 and 4. 

## Initialization
Cells are initialized in either random or fraction mode. For mode random a 
random field is used and the cells are initialized as prey, predator, 
predator and prey or empty according to probabilites set by the user. For 
the mode fraction, fractions of cells to be initialized in the respective 
states are specified.

## Plotting
The output of a simulation is set in the plot config file and as of now 
consists of a plot that contains the frequency of each species over time 
and a 2d animation of all cells with the cell state colour coded.

## References
Kurt Roth: Chaotic, Complex, and Evolving Environmental Systems. 
Unpublished lecture notes, University of Heidelberg, 2018.
     
 