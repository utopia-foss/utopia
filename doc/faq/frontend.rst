Utopia Frontend FAQs
====================

Below, some frequently asked questions regarding the Utopia frontend are addressed.

.. contents::
   :local:
   :depth: 1

----

Can I customize my Utopia data output folder?
---------------------------------------------

Yes. By default, the output gets written to ``~/utopia_output``, which might not be desirable for you. To change it, you can adjust the user configuration file, i.e. a configuration that is local to your user and is applied to all invocations of Utopia you make from that user.

To do so, use the CLI to create the user configuration file for you:

.. code-block:: bash

  utopia config --deploy

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


What is this config file madness?
---------------------------------

The many different configuration files might be overwhelming at first. But be sure: They are all there for a reason and you can greatly benefit from them.

*WIP*


Can I work on Utopia output data *without* using the frontend?
--------------------------------------------------------------

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


Can I use a custom python script to generate a plot?
----------------------------------------------------

Yes. This can be done completely from within the plot configuration by adding the ``module_file`` key:

.. code-block:: yaml

   # Custom plot config file using an external python script
   ---
   my_plot:
     # Load the following file as a python module:
     module_file: ~/path/to/my/python/script.py

     # Use the function with the following name from that module
     plot_func: my_plot_func

     # ... all other arguments (as usual)

To use this plot configuration, just pass it to the command line interface using the ``--plot-cfg`` argument. Use the commands ``utopia --help`` and ``utopia run --help`` for more information on how to pass a custom plot configuration.

.. note::

  The function's signature needs to be such that it matches the call from the chosen ``PlotCreator``, selected by the ``creator`` key.



Which signature do plot functions have to have?
-----------------------------------------------

The following code framents show the required signature that plotting functions need to adhere to in order to be callable from the `universe` and `multiverse` plot creators.

.. code-block:: python

  import xarray as xr

  from utopya import DataManager, UniverseGroup

  def bare_basics(dm: DataManager, *,
                  out_path: str,
                  **additional_kwargs):
      """Bare-basics signature required by `external` plot creator.

      Args:
          dm: The DataManager object that contains all loaded data.
          out_path: The generated path at which this plot should be saved
          **additional_kwargs: Anything else that was defined in the plot
              configuration. Consider declaring the keywords explicitly
              instead of using the ** to gather all remaining arguments.
      """
      # Your code here ...

      # Save to the specified output path
      plt.savefig(out_path)


  def universe_plot(dm: DataManager, *,
                    out_path: str,
                    uni: UniverseGroup,
                    **additional_kwargs):
      """Signature required by the `universe` plot creator.

      Args:
          ...
          uni: Contains the data from a single selected universe
      """
      # Your code here ...

      # Save to the specified output path
      plt.savefig(out_path)


  def multiverse_plot(dm: DataManager, *,
                      out_path: str,
                      mv_data: xr.Dataset,
                      **additional_kwargs):
      """Signature required by the `multiverse` plot creator.

      Args:
          ...
          mv_data: The extracted multiverse data for the chosen universes.
      """
      # Your code here ...

      # Save to the specified output path
      plt.savefig(out_path)

.. note::

  It is highly recommended to use the ``out_path`` argument for saving the
  figure. This makes use of the existing interface and puts the output data
  in a directory relative to the simulation data.
  

What is the rationale behind the tree structure of the output data?
-------------------------------------------------------------------

*TODO*


Can I work interactively with the Utopia frontend?
--------------------------------------------------

Yes.

*TODO*
