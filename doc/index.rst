Welcome to Utopia!
==================

We are happy that you found your way to the documentation.

Utopia is a comprehensive modelling framework for complex and evolving systems.
It is tailored to build models based on cellular automata (CA), agent-based models (ABMs), and network models.
Utopia provides tools for implementing a model, configuring and performing simulation runs, and evaluating the resulting data.
Additionally, several readily implemented models are shipped with Utopia.

Utopia is free software and licensed under the `GNU Lesser General Public License Version 3 <https://www.gnu.org/licenses/lgpl-3.0.en.html>`_.
For the copyright notice and the list of copyright holders, please refer to the :doc:`COPYING`

.. image:: imgs/clusters.png
   :alt: Desc

How about some nice images here?


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

    **If you are new to Utopia**, you should start with the :doc:`Tutorial <getting_started/tutorial>`. You can then work your way through the other guides. :)

.. note::

    If you notice any errors in this documentation, even minor, or have suggestions for improvements, please inform us about them by `opening an issue <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/-/issues/new>`_ or `emailing us <mailto:utopia-dev@iup.uni-heidelberg.de>`_.
    Thank you!


Contents of This Documentation
------------------------------

.. toctree::
   :hidden:

   About the Utopia project<about>
   Features<features_showcase>


.. toctree::
   :caption: Getting started
   :maxdepth: 1
   :hidden:

   Installation <README.md>
   getting_started/tutorial


.. toctree::
   :caption: Models
   :maxdepth: 1
   :glob:
   :hidden:

   models/*


.. toctree::
   :caption: Building your own model
   :maxdepth: 1
   :hidden:

   building_your_own_model/philosophy_workflow
   building_your_own_model/step_by_step
   building_your_own_model/handling_data
   building_your_own_model/plotting
   building_your_own_model/parameter-sweeps
   building_your_own_model/base_configs
   building_your_own_model/stop_conds
   building_your_own_model/interactive
   building_your_own_model/config_validation
   building_your_own_model/features
   building_your_own_model/cpp_doc
   building_your_own_model/api_refs


.. toctree::
   :caption: For Developers
   :maxdepth: 1
   :hidden:

   for_developers/coding-guidelines
   for_developers/model-requirements
   for_developers/unit-tests


.. toctree::
   :caption: Frequently Asked Questions (FAQ)
   :maxdepth: 1
   :glob:
   :hidden:

   faq/*


.. toctree::
   :caption: Miscellaneous
   :maxdepth: 1
   :hidden:

   COPYING.md
   CODE_OF_CONDUCT.md
   CONTRIBUTING.md
   examples


* :ref:`Page Index <genindex>`
* :ref:`Python Module Index <modindex>`
