.. _welcome:

Welcome to Utopia!
==================

Utopia is a comprehensive modelling framework for complex and evolving systems.
It is tailored to build models based on cellular automata (CA), agent-based models (ABMs), and network models.
Utopia provides tools for implementing a model, configuring and performing simulation runs, and evaluating the resulting data.
Additionally, several readily implemented models are shipped with Utopia.

Utopia is free software and licensed under the `GNU Lesser General Public License Version 3 <https://www.gnu.org/licenses/lgpl-3.0.en.html>`_.
For more information and a list of copyright holders, please refer to the :doc:`COPYING`.

.. TODO Nice image (hosted OUTSIDE of this repo)


Where to next?
^^^^^^^^^^^^^^

* To learn more about the Utopia project, have a look at :ref:`about_utopia` and :ref:`utopia_features`.
* If you want to start using Utopia right away, head for the :ref:`tutorial`.
* To explore other parts of the documentation, use the navigation sidebar.
* You can also take a look at the `project website <https://utopia-project.org>`_ to get to know us and explore some of Utopia's models and features!

.. note::

    If you notice any errors in this documentation, even minor, or have suggestions for improvements, please inform us about them by `opening an issue <https://gitlab.com/utopia-project/utopia/-/issues/new>`_ or `emailing us <mailto:utopia-dev@iup.uni-heidelberg.de>`_.
    Thank you!


.. ----------------------------------------------------------------------------
.. Hidden TOCtree – the sidebar should be used for navigation

.. toctree::
   :hidden:

   Project website <https://utopia-project.org>
   about/utopia
   about/should_i_use
   about/features


.. toctree::
   :caption: Getting started
   :maxdepth: 1
   :hidden:

   Installation <README.md>
   getting_started/tutorial


.. toctree::
   :caption: Usage
   :maxdepth: 1
   :hidden:

   usage/workflow
   usage/implement/index
   usage/run/index
   usage/eval/index


.. toctree::
   :caption: Models
   :maxdepth: 1
   :hidden:

   models/ContDisease
   models/ForestFire
   models/GameOfLife
   models/Opinionet
   models/PredatorPrey
   models/SandPile
   models/SEIRD
   models/SimpleEG
   models/SimpleFlocking
   ... and more <models/index>

.. NOTE If adding models above, also adjust models/index


.. toctree::
   :caption: Frequently Asked Questions
   :maxdepth: 1
   :hidden:

   faq/frontend/index
   faq/core/index


.. toctree::
   :caption: References
   :maxdepth: 1
   :hidden:

   C++ Documentation <../doxygen/html/index.html#http://>
   ref/cpp_doc
   ref/utopya_base_cfg
   ref/base_plots_cfg
   ref/batch_cfg
   utopya package <https://utopya.readthedocs.io/>
   dantro package <https://dantro.readthedocs.io/>
   paramspace package <https://paramspace.readthedocs.io/>

.. api/utopya/utopya         .. FIXME Remove
.. api/paramspace/paramspace .. FIXME Remove

.. toctree::
   :caption: Utopia Development
   :maxdepth: 1
   :hidden:

   dev/workflow
   dev/coding-guidelines
   dev/model-requirements
   dev/writing-docs

.. toctree::
   :caption: Miscellaneous
   :maxdepth: 1
   :hidden:

   Copyright Notice <COPYING>
   cite
   Contribution Guide <CONTRIBUTING>
   Code of Conduct <CODE_OF_CONDUCT>
   index_pages
