
``Amee`` - A Model for Envo-Evolutionary dynamics
==================================================

Very short introduction
-----------------------

Envo-evolutionary dynamics emphasizes the coupling between evolutionary systems, i.e. population of reproducing and mutating entities on the one hand and its environment, biotic or abiotic, on the other hand. In the literature, the terms *eco-evolutionary dyanimcs* is often found. 
Of special importance for evolution is the term *niche construction*, which refers to the modification of the environment by organisms such that its own or other's selection pressures are changed.

Design
------

Amee is not a single model, but rather a collection of different models which work through a common interface.
It can be extended and modified by building new or different ``modeltraits``, ``agent_policies`` or ``AmeeImpl`` classes, or by inheriting from ``Amee.hh`` and overriding functions therein.
However, consider to set up a new model when doing the latter. All the mentioned changes can be accomplished via inheritance. 

Furthermore, it provides a number of modifications to Utopia itself, namely a custom cell class and respective manager factories, and some infrastructure in the form of random number generators and a statistics library which provides one pass algorithms for the most common statistical tasks.

For a more thorough introduction, refer to the Amee beginner's guide inside the Amee code folder ``src/models/Amee/doc``.
