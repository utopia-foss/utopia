# Simple Evolutionary Games

_WIP_

This is a model of simple evolutionary games (on regular grids). It is in its core based on an extended and slightly modified version of Nowak & May 1992.

## Basics

In this model, the world is a (regular) grid of cells. Each cell represents one player<sup>1</sup> that possess two internal properties: a strategy and a payoff. The strategy - in the following denoted by $S_0$, $S_1$ determines how the player interacts with its neighbors. The interaction is pair-wise, meaning that a player interacts only with one other player in one game. However, the neighborhood of a player consists of multiple players, such that that per iteration step multiple games are played. The resulting gain from the individual games are summed up and stored as the player's payoff.

### Payoff From a Single Interaction

The gain from a single game is given by the interaction matrix:
$$
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
$$
$S_0$ and $S_1$ are the two different strategies.<sup>2</sup> The matrix entries $w_{ij}$ define the payoff a player with strategy $S_i$ receives from a game played against a player with strategy $S_j$.

### Neighborhood

The neighbors of a player are defined by the Moore neighborhood (9-neighborhood) of a cell on the grid. Thus, every player has $9$ neighboring players. Thus, every player plays against 9 other players per iteration step.
<!-- Adapt this, if we include the VanNeumann neighborhood at some point.--> 
<!-- Include a figure of and reference to the Moore Neighborhood here? --> 

### Evolution

Evolution is modelled by a very simplified imitation update scheme. After the games have been played, every player looks in her neighborhood for the player with the highest payoff, thus the player with the most successful strategy in her corresponding neighborhood. If this highest payoff is even higher than the player's payoff, she adapts her strategy to the more successful one. This means, in the next round, the player will participate in all the individual games with the strategy that yielded the best results in the previous round. 

If more than one player has the highest fitness in a neighborhood, one of these is selected randomly to pass on her strategy.

### Iteration step

An interaction step consists of two steps:
1. _Play the games_: Every player plays with all players in her neighborhood the game defined by the interaction matrix. The payoffs of the games are summed up and stored as a property of the players.
2. _Update the strategies_: Each player looks throughout its neighborhood for the most successful strategy (highest payoff) and adapts this strategy for the next iteration step.

These two steps are repeated for the desired number of iteration steps.

<sup>1</sup> In evolutionary game theory models, interacting entities are normally called players. This model is, at its current state, a pure evolutionary game model, therefore we will speak of players. However, if more complexity is introduced, e.g. by coupling this model with other model's the entities should be referred to as the more general agents.
<!-- Do we want to have this footnote? If yes, smaller font for footnotes? Perhaps link to agent definition in Kurt's Script? --> 
<sup>2</sup> In the case of a Prisoner's Dilemma these strategies correspond to cooperation ($S_0 = C$) and defection ($S_1 = D$). Here, we want to be able to model all kind of two-agent two-dimensional games. That's why we chose the more general nomenclature.

## Implementation Details

This section provides a few crucial implementation details.

### Interaction: Differences to Nowak & May 1992

In contrast to the implementation of Nowak & May 1992, this model excludes self-interactions. This means that, in this model, a player does not play against itself. 

This difference leads to diverging parameter regimes

### Interaction Matrix

The interaction matrix that defines the interaction between the players can be provided through three different ways:

1. _The benefaction `b` is set_: If we presuppose a simple Prisoner's Dilemma interaction, we can derive a complete interaction matrix just by one parameter $b (>1)$, which determines the benefit a defector gets from playing against a cooperator. The interaction matrix then has the following form:
$$
W = \bordermatrix{~ & S_0=C & S_1=D \cr
                  S_0=C & 1 & 0 \cr
                  S_1=D & b & 0 \cr}
$$
This simple parametrization is the one used in [Nowak1992].

2. _The benefit-cost pair (`bc-pair`) is set_: A Prisoner's Dilemma can also be defined by the two parameters $b$ and $c$ with $b>c$. $b$ encodes the benefit a player gets from a cooperative opponent and $c$ encodes the cost a cooperator pays for cooperating with the opponent. With this, the interaction matrix has the following form:
$$
W = \bordermatrix{~ & S_0=C & S_1=D \cr
                  S_0=C & b-c & -c \cr
                  S_1=D & b & 0 \cr}
$$

3. _The general interaction matrix (`ia_matrix`) if set_: The first two options only define different parameterizations of the Prisoner's Dilemma. However, it is possible to model all other possible two-player interactions with two different strategies by directly setting all matrix elements explicitly. With the right choice of parameters, one can for example model hawk-dove games, coordination games, or laiséz-faire games. For more details on these games see the theory section below. A general interaction matrix has the following form:
$$
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
$$

The algorithm is designed such that if an interaction matrix is provided in the configuration file the interaction matrix will have the form 3., even if `b` or `bc-pair` is also provided. If there is no interaction matrix, but a `bc-pair` provided, the interaction matrix will be derived as in case 2., even if `b` is set.

## Simulation Results

TODO

## Possible Future Extensions
This section collects ideas of generalizing and extending the model.
- _Generalize interactions to n-dimensional linear games_: At the moment, players can only choose between two different strategies. This could be generalized rather easily to n dimensions by generalizing the interaction matrix to n dimensions. With this, it would be possible to model for example rock-paper-scissors dynamics.

## More Theory

This section provides a deeper understanding of the underlying evolutionary game theory. It is useful to grasp the full capabilities of the _SimpleEG_ model, e.g. which storylines can potentially be modelled. The theory is mainly taken from [Friedman2016], if not otherwise indicated. The reader is referred to this book for a more detailed explanation of the theory.
<!-- How do we want to cite in general? We should adapt that style in the text, also for Nowak1992--> 

For a more basic and general introduction on evolutionary game theory, the reader is referred to [Nowak2006].

#### Two-Agent Games in 2D Strategy space: Game classification
Two-agent interactions with two possible outcomes can be classified into one of four different games according to Friedman & Barry 2016 depending on their interaction matrix. A general interaction matrix is given by:
$$
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
$$
It is useful to define the quantity $a_x$ (with $x \in \{ 0, 1\} in 2D), which describes the payoff advantage of $S0$ over $S1$ in a game against $S1$:
$$
w_x = w_{S_xS_x} - w_{S_x\bar{S_x}}
$$
where $\bar{S_x}$ is the complementary strategy of $S_x$.
From a more biological perspective, it can be interpreted as the fitness advantage of rare mutant strategy $S0$ if $S1$ is the common strategy.

_NOTE_: Its quite difficult to adapt the notation of Friedman&Barry2016 to our nomenclature - especially if one keeps in mind, that it would be nice to have it easily expandable for n-dimensions. Needs perhaps some discussion. 

##### Type 1: Hawk-Dove game (HD game)
$w_1, w_2 >0$

##### Type 2: Coordination game (CO game)
$w_1, w_2 <0$
also known as snow-drift game
##### Type 3: Dominant Strategy game (DS game)
$w_1, < 0 < w_2 $ or $w_2, < 0 < w_1 $

Two types:
###### Type 3a: Prisoner's Dilemma (PD Game)
mean fitness $\bar{w}$ decreases with increasing frequency of the dominant strategy. --> social dilemma

- explain parameters R,T,S,P and their relation in the classic version
###### Type 3b: Lasséz-Faire game (LF Game)
mean fitness $\bar{w}$ increases with increasing frequency of the dominant strategy. --> no social dilemma

## References
- Nowak, Martin A. & May, Robert M. _Evolutionary games and spatial chaos_, Nature, 1992
- Friedman, Daniel & Barry, Sinervo _Evolutionary games in natural, social, and virtual worlds_, Oxford University Press, 2016 
- Nowak, Martin A. _Evolutionary Dynamics_, Harward University Press, 2006