Welcome to Utopia!
==================

We are happy that you found your way to the documentation.

Utopia is a comprehensive modelling framework for complex and evolving environmental systems.It aims to provide the tools to conveniently implement a model, configure and perform simulation runs, and evaluate the resulting data.
<!-- TODO Write more here ... -->

Utopia is free software and licensed under the `GNU Lesser General Public License Version 3 <https://www.gnu.org/licenses/lgpl-3.0.en.html>`_.
For the copyright notice and the list of copyright holders, please refer to the :doc:`COPYING`

.. image:: imgs/clusters.png
   :alt: Desc

How about some nice images here?

.. hint::

    **If you are new to Utopia**, you should start with the :doc:`Tutorial <getting_started/tutorial>`. You can then work your way through the other guides. :)

.. note::

    If you notice any errors in this documentation, even minor, or have suggestions for improvements, please inform `Benni <herdeanu@iup.uni-heidelberg.de>`_ and/or `Yunus <yunus.sevinchan@iup.uni-heidelberg.de>`_ of them. Thank you! :)
 

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
