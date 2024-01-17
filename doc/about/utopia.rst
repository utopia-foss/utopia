.. _about_utopia:

About Utopia
============


.. contents::
    :local:
    :depth: 2

----


Goals & Philosophy
------------------
Computer models are an important part of scientific research in a wide range of fields.

First and foremost, Utopia aims to be a *comprehensive* modelling framework.
We believe that by tightly integrating frequently-used tools, we can increase productivity in the :ref:`modelling workflow <usage_workflow>` and reliability of the scientific results.

The goals of Utopia can be summarized as follows:

* Attempt to cover all "everyday aspects" of studying computer models.
* Adhere to software engineering best practices in order to produce reliable and reproducible results.
* Foster collaboration, e.g. by developing and using the framework throughout a research group; that way synergies can develop more rapidly and friction can be reduced.

Furthermore, Utopia was built with the following design philosophy in mind:

* Everything should be configurable — but with good defaults.
* Provide a wide set of tools, but don't force them on the framework user.
* Prevent lock-in by using open data formats.
* Be computationally efficient, yet customizable enough.


.. hint::

    We have written a research article about these goals and observations we made throughout building Utopia.
    If you are interested, take a look at :ref:`the publications list <cite>`.


Capabilities
------------
Utopia's core capability is that it can bring together many (if not all) tools that you are using to study computer models and integrate them in a way that makes your workflow more efficient.

If you have a modelling workflow similar to :ref:`this <usage_workflow>`, Utopia might be just the right tool for you ...

.. admonition:: Should *you* use Utopia?

    Find out :ref:`here <should_i_use>`.

    For a more extensive overview of specific features, have a look at the :ref:`utopia_features`.


History
-------
Below is a brief history of how the Utopia project came about.

2016
^^^^
The idea of the Utopia framework emerged in 2016 among members of the `TS-CCEES research group <http://ts.iup.uni-heidelberg.de/>`_ at the `Institute of Environmental Physics <https://www.iup.uni-heidelberg.de/en>`_.
In the words of Prof. Kurt Roth at that time:

.. pull-quote::

    We embark on creating a C++ framework for simulating the coupled dynamics of geomorphology, diverse vegetation, and interacting, developing, and evolving populations of different species.
    The focus is on fundamental studies of such coupled complex and evolving systems.

    The framework is to consist of coupled CA and ABMs (cellular automata and agent-based models) that operate on flexible discretizations in order to optimally represent the various process chains.
    Design goals are (i) versatility and expandability of the framework, (ii) computational efficiency, and (iii) usability by non-experts, in this order.
    It is first developed and demonstrated for a minimally complete set of species and processes.

At the same time, Lukas Riedel created ``citcat``, a generic C++ template library for the efficient simulation of cellular automata, based on `DUNE <https://www.dune-project.org>`_.
Later on, ``citcat`` became the basis for the Utopia framework.


2017
^^^^
Development on ``citcat`` continued and first experiences were gained in collaboratively working on software projects.

During this time, many of the first Utopia contributors built their own domain-specific simulation frameworks as part of their MSc or BSc projects.
The experience gained and ideas developed during that time had a big influence on the philosophy and goals of Utopia.
For instance, it was observed that re-implementing models over and over again was not only inefficient, but also error-prone.
On the other hand, by *collaboratively* working on a framework, these difficulties could be circumvented and  synergies could develop.


2018
^^^^
The structure and scope of Utopia was planned in more detail: it was to consist of a C++ backend (for computationally efficient model implementations, based on ``citcat``) and a Python frontend (for model configuration, simulation management, and evaluation).

In a group effort, the foundations of Utopia were laid in a week-long hackathon.
Similar events were organized a few more times to boost development of the framework.

Henceforth, new projects in the research group preferentially used Utopia for model implementations and evaluations.
Alongside these developments, the feature set of Utopia evolved further.


2019
^^^^
To gain flexibility and control, the DUNE framework was removed as a dependency of Utopia.
This entailed a restructuring of the build system and a custom implementation for cellular automata.

In the summer of 2019, Utopia was first used in postgraduate teaching as part of the *Chaotic, Complex, and Evolving Environmental Systems* lecture by Prof. Kurt Roth.
In the accompanying exercises, students used Utopia to run simulations of different models and understand the effect of the chosen parameters on the system dynamics.
Furthermore, Utopia was used in a postgraduate physics seminar, where groups of students implemented models using Utopia and investigated their behavior.

In August 2019, Utopia went public under the `LGPLv3+ open-source license <https://www.gnu.org/licenses/lgpl-3.0.html>`_.


2020
^^^^
Three :ref:`research articles <cite>` about Utopia, its frontend, and collaboratively developing and working with this modelling framework were published in 2020.

Building on the experience from previous teaching events, Utopia was used in two further postgraduate courses: the next iteration of the aforementioned lecture, as well as another seminar on complex and evolving systems.

Furthermore, 2020 was the year in which the number of total projects carried out using Utopia surpassed 25.
As part of these projects, more than 45 models have been implemented so far (mostly in private repositories).


2021
^^^^
With the sunset of the TS-CCEES research group, the Utopia framework has to find a new home ...

The `Utopia Project webpage <https://utopia-project.org>`_ is published.
Code repositories are migrated to a `GitLab.com group <https://gitlab.com/utopia-project>`_.

Benjamin Herdeanu and Yunus Sevinchan defend their doctoral theses; both have been using Utopia extensively.


2022
^^^^
The Utopia project has migrated all its repositories from the TS-CCEES group's servers to `gitlab.com/utopia-project <https://gitlab.com/utopia-project>`_.

The `outsourcing of the Utopia frontend <https://gitlab.com/utopia-project/utopia/-/merge_requests/277>`_ brings about a larger architectural change.
This makes the features of `utopya <https://gitlab.com/utopia-project/utopya>`_ available for use with other simulation backends, adds a bunch of new features, expands the documentation, and aims to reduce future maintenance load.


2023
^^^^
Harald Mack and Julian Weninger defend their doctoral theses, using Utopia as the foundation for their computer models.

Utopia is used in `several scientific publications <https://utopia-project.org/publications/>`_.
With utopya as a standalone package, first Python-based models or analysis frameworks are implemented.
