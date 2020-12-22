Welcome to Utopia's documentation!
==================================

Utopia is a comprehensive modelling framework for complex and evolving systems.
It is tailored to build models based on cellular automata (CA), agent-based models (ABMs), and network models.
Utopia provides tools for implementing a model, configuring and performing simulation runs, and evaluating the resulting data.
Additionally, several readily implemented models are shipped with Utopia.

Utopia is free software and licensed under the
`GNU Lesser General Public License Version 3 <https://www.gnu.org/licenses/lgpl-3.0.en.html>`_
or any later version.
For details, please refer to the :doc:`COPYING`.


Welcome to Utopia!
------------------

We are happy that you found your way to the documentation.
If you want to know more about Utopia, have a look at the :ref:`models included <index_models>`, or scroll through the extensive :doc:`features`.
For a complete overview, refer to the :doc:`ReadMe <README>`.

If you want to start using Utopia, head right into the :doc:`Tutorial <guides/tutorial>`!
You can then work your way through the other :ref:`guides <index_guides>`.

.. admonition:: Wondering if you should start using Utopia?

   You might ask yourself: "Why not use `NetLogo <https://ccl.northwestern.edu/netlogo/index.shtml>`_?"
   Here's why:

   - Utopia is written in modern C++ and Python, and allows developers to exploit the full potential of both programming languages.
   - Utopia offers tools to perform parameter sweeps of models and analyze their high-dimensional data efficiently.
   - Utopia includes a multithreading interface and can execute simulations concurrently on distributed cluster nodes.
   - Utopia supports and encourages software engineering workflows, including version control, unit testing, and more.

   On the other hand, you should *not* use Utopia, if

   - Programming and using the command line is not your thing; *Utopia embraces flexibility for programmers.*
   - You want a library with a simple and concise feature set; *Utopia is feature-rich and extensive.*
   - You want a *very* quick solution; *Utopia has a steep learning curve.*

.. hint::

   If you notice any errors in this documentation, even minor, or have suggestions for improvements, please inform `Benni <herdeanu@iup.uni-heidelberg.de>`_ and/or `Yunus <yunus.sevinchan@iup.uni-heidelberg.de>`_ of them. Thank you! :)


Contents of this Documentation
------------------------------

.. toctree::
   :maxdepth: 1
   :glob:

   The README <README>
   C++ Documentation <cpp_doc>
   Utopia Feature List <features>

.. _index_guides:

.. toctree::
   :caption: Guides
   :maxdepth: 1
   :glob:

   guides/tutorial
   guides/greenhorn-guide
   guides/how-to-build-a-model
   guides/*

.. toctree::
   :caption: Frontend & utopya
   :glob:
   :maxdepth: 2

   frontend/inc/utopya_base_cfg
   frontend/data
   frontend/plotting
   frontend/batch
   frontend/inc/base_plots_cfg
   frontend/inc/batch_cfg
   utopya API Reference <api_utopya/utopya>
   frontend/interactive
   frontend/*
   frontend/inc/api_paramspace

.. _index_models:

.. toctree::
   :caption: Models
   :maxdepth: 1
   :glob:

   models/*

.. toctree::
   :caption: Frequently Asked Questions (FAQ)
   :maxdepth: 2
   :glob:

   faq/*

.. toctree::
   :caption: Miscellaneous
   :maxdepth: 1
   :glob:

   Copyright Notice <COPYING>
   cite
   Contribution Guide <CONTRIBUTING>
   Code of Conduct <CODE_OF_CONDUCT>
   Documentation Examples <examples>

* :ref:`Page Index <genindex>`
* :ref:`Python Module Index <modindex>`
