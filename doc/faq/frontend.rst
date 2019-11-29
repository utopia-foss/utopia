Utopia Frontend
===============

Below, some frequently asked questions regarding the Utopia frontend are addressed.

.. contents::
   :local:
   :depth: 2

----


Custom output folder
--------------------
Can I customize my Utopia data output folder?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. By default, the output gets written to ``~/utopia_output``, which might not be desirable for you. To change it, you can adjust the user configuration file, i.e. a configuration that is local to your user and is applied to all invocations of Utopia you make from that user.

To do so, use the CLI to create the user configuration file for you:

.. code-block:: bash

  utopia config user --deploy

Follow the instructions given there. A file will then be created at ``~/.config/utopia/user_cfg.yml``. Open that file and find the ``paths`` entry. Uncomment the ``paths`` entry and the nested ``out_dir`` entry such that it looks like this:

.. code-block:: yaml

  # ...

  # Output paths
  # These are passed to Multiverse._create_run_dir
  paths:
    # base output directory
    out_dir: ~/my/custom/utopia_output

    # ...

Save and close the file. From now on, all your simulation output will be stored in ``~/my/custom/utopia_output``.

.. warning::

  Do not add any model-specific information into the user configuration.

  Also, only uncomment and change those parts of the user configuration that you *really* want to change. With updates to Utopia, the default values of parts of the frontend configuration may change; if you fixed those values in the user configuration, you might not only miss out on the updates, but the frontend might not work as desired.


Work interactively 
------------------
Can I work interactively with the Utopia frontend?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:doc:`Yes. </frontend/interactive>`


Work without frontend
---------------------
Can I work on Utopia output data *without* using the frontend?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. Utopia stores all data in easily readable file formats that are supported on many platforms.

* The output from models is in the binary `HDF5 <https://en.wikipedia.org/wiki/Hierarchical_Data_Format#HDF5>`_ format.
* All the configuration that played a role in creating that data is stored in human-readable `YAML <https://en.wikipedia.org/wiki/YAML>`_ files.

For Python, the `h5py <http://www.h5py.org>`_ library provides a very convenient interface to access the output data. It often makes sense to also read in the configuration file for a single simulation, which can be found in the ``data/uni*/config.yml`` file.

.. code-block:: python

   import h5py as h5
   import yaml

   with open('config.yml', 'r') as f:
       cfg = yaml.load(f)

   # Get the file name and open the HDF5 file
   filename = cfg['output_path']
   data = h5.File(filename, 'r')

   # Using dict-like access, the data tree can be traversed ...
   model_output = data["MyFancyModel"]

.. note::

  You can use the data tree that is printed out before plotting to find out the tree representation within the file. **Note,** however, that only the part below the ``data`` key is located inside the HDF5 file; all the rest is loaded into the data tree from separate sources.

.. note::

  While working with Utopia's output data directly is possible, be aware that the frontend takes care of a great deal of things, which are not available in such a case: It loads many HDF5 files into a uniform data tree, makes the configuration accessible, allows to collect data from different parts of the tree for plotting, reshapes data to be in the expected shape ...


.. _faq_config:

The versatile ways of configuring your simulations
--------------------------------------------------
Utopia has one important and wide-ranging premise when it comes to the configuration of simulations:
**Everything should be configurable, but nothing need be.**

In other words, you should be able to have full control over all the parameters that are used in a simulation, but there should be reasonable defaults for all of them such that you don't *have* to specify them.
Ideally, you only specify those parameters you want to *change* and rely on the defaults for everything else.

This flexibility is realised using a set of different configuration levels.
The many different ways to adjust the configuration might be overwhelming at first, but be sure: These options are all there for a reason and you can greatly benefit from them.


Which possibilities are there to configure a simulation?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To achieve the goal stated above, Utopia uses a hierarchy of configuration levels:

#. **Base configuration:** all the default values
#. **Model configurations:** model-specific defaults

    * Defined alongside the respective models
    * Provide defaults *only* for the model; can be imported where needed.

#. **User configuration:** user- or machine-specific *updates* to the defaults

    * Is used for all simulation runs, regardless of the model. This would be the place to specify model-*independent* parameters like the number of CPUs to work on.
    * Empty by default. Deploy using ``utopia config user --deploy`` and follow the steps given there.

#. **Run configuration:** updates for a specific simulation run
#. **Temporary changes:** additional updates, defined via the CLI

    * If you call ``utopia run --help`` you can find a list of some useful ways to adjust some parameters.
    * For example, with ``--num-steps <NUMSTEPS>`` you can specify how many time steps the model should iterate.

Combining all these levels creates the so-called **meta configuration**, which contains *all* parameters needed for a simulation run.
The combination happens by starting from the lowest level, the base configuration, and recursively updating all entries in the configuration with the entries from the next level.

The individual files and the resulting meta configuration are also stored alongside your output data, such that all the parameters are in one place.
The stored meta configuration file can also be used as the run configuration for a new simulation run.

This can be a little bit confusing at first, but no worries: The section below gives a more detailed description of the different use cases.


Where do I specify my changes?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Short Answer
""""""""""""
If in doubt, use the run configuration; you can specify everything there.

Longer Answer
"""""""""""""
Changes to the defaults *can* be specified in the user configuration, the run configuration, and via the CLI.

To decide where to specify your changes, think about the frequency you change the parameter with and whether the change relates to a model-specific parameter or one that configures the framework.

Going through the following questions might be helpful:

* Is the change temporary, e.g. for a single simulation run?

    * **Yes:** Ideally, specify it via the CLI. If there are too many temporary changes, use the run configuration.
    * **No:** Continue below.

* Is the change independent of a model, e.g. the number of CPUs to use?

    * **Yes:** Use the user-configuration.
    * **No:** The parameter is model-specific; use the run configuration.


.. warning::

    The base and model configurations provide *default* values; these configuration files are **not meant to be changed** but should reflect a certain set of persistent defaults.

    Of course, during model *development*, you as a model developer will change the default model configuration, e.g. when adding additional dynamics that require a new parameter.


How do I find the available parameters?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The base configuration documents a lot of parameters directly in the configuration file; see :ref:`here <utopya_base_cfg>`.

For the model configuration, the model documentation usually includes the default configuration; for example: :doc:`../models/ForestFire`.



Data tree structure
-------------------
What is the rationale behind the tree structure of the output data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. todo::

  Write this.

