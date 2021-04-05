# Opinionet

*Opinionet* is an agent-based opinion dynamics model. It simulates consensus-seeking agents - so-called *users* - that interact with their social environment, which is represented by the underlying network structure. Each network node accommodates a single agent. Each agent holds an opinion value in the continuous opinion space (missing mat)

As an extension, a second network of economically driven *media* is implemented, which both influences and adapts to the user network.

This model is based on Deffuant and Weisbruch's [Deffuant 2000] selective exposure model and Quattrociocchi's [Quattrociocchi 2014] model of media influence.

## Fundamentals

The model algorithm is based on the following underlying concepts:

* **asymmetric interactions**: In the current age of online communication and social media, the links along which opinions and beliefs spread, need not coincide with strong, mutual social ties. Hence, a directed network is used to represent the interactions among users.
* **bounded confidence**: A user can only be influenced by a neighboring user if the opinion difference between the two is smaller than a given threshold epsilon;. epsilon; is referred to as confidence bound or tolerance.
* **selective exposure**: The bounded confidence approach describes the way users process the input by their social surroundings. With selective exposure, they also modify this input. Users preferentially interact with others that share similar beliefs. They change their interaction preferences by continuously adjusting the weights of their outgoing links, which represent the probabilities of interactions along the respective links. The weight adjustment is done in a way that links to conforming neighbors are reinforced.
* **network adaptivity**: Social networks naturally co-develop along with the beliefs and opinions of individuals. A rewiring mechanism allows users to cut unwanted links and establish new links, preferentially to a neighbor's neighbor.
* **economically driven media**: The media are part of a static, undirected network with the links representing ideological competition. Given this prescribed competition, they aim at maximizing their user number by adjusting their opinion depending on that of the most successful neighbor.

## Implementation Details

### The user network

Each user holds two properties: An **opinion** sigma; and the **used medium**. The interaction network is implemented as a directed graph with each node containing a single user and each edge holding a weight representing the interaction probability along that edge. An edge from user i to user j means that agent i seeks agreement with agent j. This means that the direction of influence, or that of the information flow, is opposite to the edge direction.

### The media network

Each medium holds two properties: An **opinion** sigma; and the **user number**. The media live on a topologically static, undirected graph with fixed weights. The weights represent (mutual) ideological competition and can be negative or positive.

### Model algorithm

Each iteration step consists of three *revisions*. Each of the revisions updates a single user or medium.

1. **user revision**: Interaction among users
2. **media revision**: Interaction among media
3. **information revision**: Interaction between users and media

Note that any parameters that are not a vertex property are fixed globally, i.e., they are the same for all agents and interactions.

#### User revision

First, a random user i is picked. Then, three steps follow:

1. **opinion update**:
    The interaction partner j is chosen based on the probability distribution given by the weights of the outgoing links. If (missing code)
    Otherwise the opinion remains unchanged. Hereby, (missing math) describes the strength of the attraction.

2. **weight update**:
    (missing math) denotes the weight of the edge from i to j. The weights of user i's outgoing links are updated depending on the relative opinion distances to the respective neighbors. The weights are first reduced proportional to the opinion distance

    (missing math)

    and are then again normalized over the neighborhood. The `weighting` kappa determines the strength of the weight change.

3. **rewiring**: 
    Each of the outgoing edges is rewired with the `rewiring` probability if the respective opinion distance is larger than the tolerance. The links are preferentially rewired to a neighbor's neighbor. If no suitable user is found at first try, a random user is selected for rewiring. The rewired edge initially has the weight 1/out-degree. Note that the degree is preserved by the rewiring process.

#### Media revision

First, a random medium is selected. As an interaction partner, the neighbor with the highest user number that is still within the tolerance range epsilon m is selected. The opinion is updated as follows:

(missing math)

Here, the weights are the prescribed weights that can be both negative or positive. If there is no medium with a higher user number in the neighborhood, the opinion remains unchanged.

#### Information revision

First, a user and a medium are selected randomly. Whether the user's opinion is influenced and whether the user switches to the new medium depends on the *user characteristic*. The basic type is the bounded confidence type, where the user is influenced and switches to the new medium if the medium's opinion lies within the tolerance range epsilon m. The opinion is then updated analogous to the user opinion update.

## Simulation results

![opinion](https://ts-gitlab.iup.uni-heidelberg.de/uploads/-/system/personal_snippet/32/32fcd004beccf656a749e4e258eb933e/opinion.png)

The `opinion_time_series` plot shows the temporal development of the user opinion density as well as a few representative trajectories. On the right, the final opinion distribution is displayed.

In the above case, the media network was turned off. One can observe stable opinion groups with opinions closer than epsilon=0.162. Due to the strong *weighting* kappa;=0.95, the weights of the links between those groups dropped to zero.

A useful quantity for characterizing the final state is the *localization* L which is approximately the inverse of the number of distinct final macroscopic opinion groups (e.g. L~0.5 for two separated opinion groups). The plot below shows the final localization over epsilon; for different values of kappa;. Stronger weighting impedes global consensus.

![transition_shift_weighting-1](https://ts-gitlab.iup.uni-heidelberg.de/uploads/-/system/user/102/4cbad5459176bd592b6cd317ce174d0f/transition_shift_weighting-1.png)

## Literature

- Traub, Jeremias (2019). Bachelor’s thesis: Modelling Opinion Dynamics - Selective Exposure in Adaptive Social Networks. Ruprecht-Karls Universität Heidelberg.
- Deffuant G. et al: _Mixing beliefs among interating agents._ Adv Complex Syst. (2000) 3:87-98. DOI: 10.1142/S0219525900000078
- Quattrociocchi W. et al: _Opinion dynamics on interacting networks: media competition and social influence._ Sci. Rep. (2014) 4, 4938. DOI: 10.1038/srep04938
- Castellano, C. et al. (2009). “Statistical physics of social dynamics”. Reviews of Modern Physics 81:2, pp. 591–646. DOI : 10.1103/RevModPhys.81.591 .
- Sîrbu, A. et al. (2017). “Opinion dynamics: models, extensions and external effects”.
arXiv:1605.06326 [physics], pp. 363–401. DOI : 10.1007/978-3-319-25658-0_17 .
