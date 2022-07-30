
.. _faq_sim_control:

Simulation Control
------------------
Can I customize my Utopia data output folder?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. By default, the output gets written to ``~/utopia_output``, which may or may not be ideal for you. To change this, you can adjust the :ref:`user configuration file <config_hierarchy>`, i.e. the configuration that is local to your user and is applied to all invocations of Utopia you make from that user. To do so, use the CLI to create the user configuration file for you:

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

  Also, only uncomment and change those parts of the user configuration that you *really* want to change. With updates to Utopia, the default values of parts of the frontend configuration may change; fixing those values in the user configuration will mean that you not only miss out on the updates, but also that the frontend might not work as desired.


Can I work interactively with the Utopia frontend?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes: see :ref:`here <utopya_interactive>`.


Can I work with Utopia output data *without* using the Utopia frontend?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes. Utopia stores all data in easily readable file formats that are supported on many platforms.

* The output from models is in the binary `HDF5 <https://en.wikipedia.org/wiki/Hierarchical_Data_Format#HDF5>`_ format.
* All the configuration files that were relevant to creating that data is stored in human-readable `YAML <https://en.wikipedia.org/wiki/YAML>`_ files.

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

  While working with Utopia's output data directly is possible, be aware that the frontend takes care of a great deal of things, which are not available in such a case: it loads many HDF5 files into a uniform data tree, makes the configuration accessible, allows collecting data from different parts of the tree for plotting, reshapes data into the expected shape, and more.


How does the Cluster Mode work?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In cluster mode, the :py:class:`~utopya.multiverse.Multiverse` splits up a parameter sweep run in a way that each compute node takes up a specific fraction of the available universe simulations.
The workload is balanced depending on the number of universes to be computed and the available nodes.
The corresponding information is extracted from environment variables and can be configured via the meta configuration.
