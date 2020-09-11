Welcome to Utopia!
==================

Utopia is a comprehensive modelling framework for complex and evolving systems.
It is tailored to build models based on cellular automata (CA), agent-based models (ABMs), and network models.
Utopia provides tools for implementing a model, configuring and performing simulation runs, and evaluating the resulting data.
Additionally, several readily implemented models are shipped with Utopia.

Utopia is free software and licensed under the `GNU Lesser General Public License Version 3 <https://www.gnu.org/licenses/lgpl-3.0.en.html>`_.
For more information and a list of copyright holders, please refer to the :doc:`COPYING`.

.. TODO Nice image (hosted outside of repo)
.. image:: imgs/clusters.png
   :alt: Desc

If you want to know more about the Utopia project, have a look at ...

If you want to start using Utopia, head right into the :ref:`Tutorial <tutorial>`!

.. note::

    If you notice any errors in this documentation, even minor, or have suggestions for improvements, please inform us about them by `opening an issue <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/-/issues/new>`_ or `emailing us <mailto:utopia-dev@iup.uni-heidelberg.de>`_.
    Thank you!


.. ----------------------------------------------------------------------------
.. Invisible TOCtree – the sidebar should be used for navigation

.. toctree::
   :hidden:

   about/utopia_project
   about/main_features
   about/should_i_use
   about/features


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
   :caption: Usage
   :maxdepth: 1
   :hidden:

   usage/philosophy_workflow
   usage/step_by_step
   usage/handling_data
   usage/plotting
   usage/parameter-sweeps
   usage/base_configs
   usage/stop_conds
   usage/interactive
   usage/config_validation
   usage/cpp_doc
   usage/unit-tests


.. toctree::
   :caption: Frequently Asked Questions
   :maxdepth: 1
   :glob:
   :hidden:

   faq/*


.. toctree::
   :caption: API References
   :maxdepth: 1
   :glob:
   :hidden:

   api/*

.. toctree::
   :caption: For Developers
   :maxdepth: 1
   :hidden:

   for_devs/coding-guidelines
   for_devs/model-requirements

.. toctree::
   :caption: Miscellaneous
   :maxdepth: 1
   :hidden:

   COPYING.md
   CODE_OF_CONDUCT.md
   CONTRIBUTING.md
   examples


.. * :ref:`Page Index <genindex>`
.. * :ref:`Python Module Index <modindex>`
