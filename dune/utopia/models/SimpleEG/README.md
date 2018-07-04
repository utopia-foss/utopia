# Simple Evolutionary Games

This is a model of simple evolutionary games (on regular grids). It is based on an extended and slightly modified version of Nowak & May (1992).

## Fundamentals

The world is a regular square grid of cells. Each cell represents one player^1 that possesses two internal properties: a strategy and a payoff. The strategy -- in the following denoted by $`S_i`$, where $`i`$ denotes a specific strategy from the set of all possible strategies -- determines how the player interacts with its neighbors. Thus, the basic interaction is pair-wise. However, the neighborhood of a player, which is defined through the grid, consists of multiple players, such that that per iteration step multiple games are played. The resulting gain from the individual games are summed up and stored as the player's payoff.


^1 In evolutionary game theory models, interacting entities are normally called players. This model is, at its current state, a pure evolutionary game model, therefore we will speak of players. However, if more complexity is introduced, e.g. by coupling this model with other models the entities should be referred to as the more general agents.
<!-- Do we want to have this footnote? If yes, smaller font for footnotes? Perhaps link to agent definition in Kurt's Script? --> 


### The Game – The Gain

The payoff, a player receives from a single game, is encoded in the interaction matrix:
<!--
```math
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
```-->
<!--Would be nice, if this would work, but stupid markdown does not cope with the complicated standard latex command 'bordermatrix'-->

```math
\begin{bmatrix}w_{S_0S_0} & w_{S_0S_1} \cr w_{S_1S_0} & w_{S_1S_1} \end{bmatrix}
```

$`S_0`$ and $`S_1`$ denote the two different possible strategies.^2 The matrix entries $`w_{S_iS_j}`$ define the payoff a player with strategy $`S_i`$ receives from a game played against a player with strategy $`S_j`$.


^2 In the case of a Prisoner's Dilemma these strategies correspond to cooperation ($`S_0 = C`$) and defection ($`S_1 = D`$). Here, we want to be able to model all kind of two-dimensional two-player games. That's why we chose the more general nomenclature.

### The Neighborhood – Just Look Around

The neighbors of a player placed on a regular square grid are determined by the Moore neighborhood (9-neighborhood) of the corresponding cell. Thus, every player has $`9`$ neighbours against which she plays in a single iteration step.

<!-- Adapt this, if we include the VanNeumann neighborhood at some point.--> 
<!-- Include a figure of and reference to the Moore Neighborhood here? --> 

### Evolution – Simply the Best

Evolution is modelled by a very simple imitation update scheme. After all games are played, every player looks in her neighborhood for the player with the highest payoff, thus, the player with the most successful strategy in her corresponding neighborhood. If this highest payoff is higher than the player's payoff, she adapts her strategy to the more successful neighbor's one. This means, in the next round, the player will participate in all the individual games with the strategy that locally yielded the best result in the previous round. 

If more than one player has the highest fitness in a neighborhood, one of these is selected randomly to pass on her strategy.

### Iteration Step – Time in Steps 

An interaction step consists of two steps:

1. _Play the games_: Every player plays with all players in her neighborhood the game defined by the interaction matrix. The payoffs of the games are summed up and stored as a property of the players.

2. _Update the strategies_: Each player looks within its neighborhood for the most successful strategy (highest payoff) and adapts this strategy for the next iteration step.

These two steps are repeated for the desired number of iteration steps.


## Implementation Details

This section provides a few important implementation details.

### Interaction: Differences to Nowak & May (1992)

In contrast to the implementation of Nowak & May (1992), this model excludes self-interactions. This means that a player does not play against itself in this model.

This leads to difficulties in comparing the parameter regimes of Nowak & May (1992) with the parameter regimes obtained in this model. However, qualitatively the same system regimes can be observed for slightly different parameters.
<!-- Better check this again! --> 

### Interaction Matrix

The interaction matrix that defines the interaction between the players can be provided in three different ways:

1. _Setting the benefit `b`_: If we presuppose a simple Prisoner's Dilemma interaction, we can derive a complete interaction matrix just with one parameter $`b (>1)`$, which determines the benefit a defector gets from playing against a cooperator. This simple parametrization is the one used in Nowak & May (1992).

2. _Setting the benefit-cost pair `bc-pair`_: A Prisoner's Dilemma can also be defined by the two parameters $`b`$ and $`c`$ with $`b>c`$. $`b`$ encodes the benefit a player gets from a cooperative opponent and $`c`$ encodes the cost a cooperator pays for cooperating with the opponent. 

3. _Setting a general interaction matrix `ia_matrix`_: The first two options only define different parameterizations of the Prisoner's Dilemma. However, it is possible to model all other possible linear two-strategy two-player interactions by setting all matrix elements explicitly. With the right choice of parameters, one can for example model a Hawk-Dove games, a Coordination Games, or a Laiséz-Faire game. For more details on these games and their definitions see the theory section below. 

The interaction matrices of the individual options are collected here:
	
| Setting the benefit `b` | Setting the benefit-cost-pair `bc-pair`  | Setting a general interaction matrix `ia_matrix` |
| -------- | -------- | -------- |
| 	$`\begin{bmatrix} 1 & 0 \\ b & 0 \end{bmatrix}`$ | $`\begin{bmatrix} b-c & -c \cr b & 0 \end{bmatrix}`$ | $` \begin{bmatrix}w_{S_0S_0} & w_{S_0S_1} \cr w_{S_1S_0} & w_{S_1S_1} \end{bmatrix}`$  |

The algorithm is designed such that if an interaction matrix is provided in the configuration file the interaction matrix will define the game, even if `b` or `bc-pair` are also provided. If there is no interaction matrix, but a `bc-pair` provided, the interaction matrix will be derived from it, even if `b` is set. `b` is used only if none of the other options is provided.

## Simulation Results -- A Selection Process

TODO

## More Conceptual and Theoretical Background

This section provides a deeper understanding of the underlying evolutionary game theory. It is useful to grasp the full capabilities of the _SimpleEG_ model, e.g. which storylines can potentially be modelled. The theory is mainly taken from Friedmann & Barry (2016), if not otherwise indicated. The reader is referred to this book for a more detailed explanation of the theory.

For a more basic and general introduction on evolutionary game theory, the reader is referred to Nowak (2006).

#### Linear Two-Player Games in 2D Strategy Space: Game classification
Linear two-player, two-strategy interactions can be classified into one of four different games according to Friedman & Barry (2016) determined by their interaction matrix. A general interaction matrix has the form:

```math
W = \begin{bmatrix}
w_{S_0S_0} & w_{S_0S_1} \cr
w_{S_1S_0} & w_{S_1S_1}
\end{bmatrix}
```

It is useful to define the quantity $`a_x`$ with $`x \in \{ 0, 1\}`$, which describes the payoff advantage of $`S0`$ over $`S1`$ in a game against $S1$ if the strategy space is two dimensional:

```math
a_x = w_{S_xS_x} - w_{S_x\bar{S_x}}
```

where $`\bar{S_x}`$ is the complementary strategy of $`S_x`$.
From a more biological perspective, it can be interpreted as the fitness advantage of rare mutant strategy $`S_0`$ if $`S_1`$ is the common strategy.

##### Type 1: Hawk-Dove game (HD game): $`a_1, a_2 >0`$

In this parameter regime, the game has the characteristics of a _Hawk-Dove game_. It is also known as the _game of chicken_ of _snowdrift game_. 
We describe its logic using the Hawk-Dove picture, although the same situation can be adapted to a multitude of situations that can be found in a multitude of areas such as biology or even international politics. This game was also used in the foundation of evolutionary game theory in the paper of May & Price (1973).

In a Hawk-Dove game two players compete for a resource that cannot be split up. They can choose between two strategies: thread displays (Dove) or attack (Hawk). If both choose the Hawk strategy they attack each other and fight until one of them is injured and the other wins. If one player behaves as a Hawk and the other one as a Dove the Hawk defeats the Dove. If both players choose Dove, they both get a small benefit. However, the payoff is smaller than for a Hawk playing against a Dove.

If the parameters are chosen such that the relation $`a_1, a_2 >0`$ is met, the game is a version of a Hawk-Dove game. Varying the parameters varies the relation of the outcomes from the individual strategies in the games, their qualitative outcome, however, does not change.

##### Type 2: Coordination game (CO game): $`a_1, a_2 <0`$

This game is perhaps best explained using Jean-Jacques Rousseau's _Discourse on the Origin and Basis of Inequality Among Men_ (1754):
> If it was a matter of hunting a deer, everyone well realized that he must remain faithful to his post; but if a hare happened to pass within reach of  one  of  them,  we  cannot  doubt  that  he  would  have  gone off in pursuit of it without scruple.
>

Extracting the underlying nature of the interaction: All players profit most, if they work for a common aim, but single players can get distracted by easier achievable goals with smaller returns. There is a steady state – all cooperating – which is however unstable to some kind.

Such situations can be modelled if the parameters of the interaction matrix obbey the relation $`a_1, a_2 <0`$.

##### Type 3: Dominant Strategy game (DS game): $`a_1, < 0 < a_2 `$ or $` a_1, > 0 > a_2 `$

In dominant strategy games, one strategy can be easily invaded by the other one, whereas the other cannot be invaded. Assuming replicator dynamics, this means that one strategy has a fixation, thus a population will evolve towards it.

There are two qualitatively different types of dominant strategy games:
###### Type 3a: Prisoner's Dilemma (PD Game): $`a_1, < 0 < a_2 `$

The Prisoner's Dilemma is arguably the most famous social dilemma. The standard story to explain the nature of the dilemma uses two prisoner's from the same criminal gang. Let us call them A and B. They are confined separately and cannot communicate with each other. Their sentenzing is dependent on the desicion of the other prisoner: If A and B betray each other, each serves 2 years in prison. If A betrays B and B remains silent, then A will be set free and B will need to serve 3 years in prison. If they both remain silent, they will get a diminished punishment of 1 year. Therefore, the decision of one prisoner is dependent on the decision of the other one. Both face a social dilemma situation.

To put it in the standard parameter set, a two dimensional interaction matrix of the form 

```math
W = \begin{bmatrix}
R & S \\
T & P 
\end{bmatrix}
```

defines a Prisoner's Dilemma if the parameters have the following relation: T (temptation) > R (reward) > P (punishment) > S (sucker's payoff). This formulation is equivalent to the condition: $`a_1, < 0 < a_2 `$. 

In a Prisoner's Dilemma the mean fitness $`\bar{w}`$ decreases with increasing frequency of the dominant strategy. This means that the more players defect the lower the mean fitness gets.

###### Type 3b: Lasséz-Faire game (LF Game): $`a_2, < 0 < a_1 `$

In the Lasséz-Faire game regime – defined by the relation $`a_2, < 0 < a_1 `$ – there is no social dilemma any more. One strategy is dominant and also results in the highest mean payoff. The mean fitness $`\bar{w}`$ increases with increasing frequency of the dominant strategy. 

## Possible Future Extensions
This section collects ideas of generalizing and extending the model:

- _Generalize interactions to n-dimensional linear games_: At the moment, players can only choose between two different strategies. This could be generalized rather easily to n dimensions by generalizing the interaction matrix to n dimensions. With this, it would be possible to model for example rock-paper-scissors dynamics ($`n=3`$).


## References
- Friedman, Daniel & Barry, Sinervo (2016). _Evolutionary Games in Natural, Social, and Virtual Worlds_. Oxford University Press 
- Nowak, Martin A. & May, Robert M. (1992). _Evolutionary Games and Spatial Chaos_. Nature
- Nowak, Martin A. (2006). _Evolutionary Dynamics_. Harward University Press
- Rousseau, Jean-Jaque (1754). _Discourse on the Origin and Basis of Inequality Among Men_. 
- Smith, Maynard J. & Price, G.R. (1973). _The logic of animal conflict_. Nature. doi:10.1038/246015a0.