Utopia Tutorial
===============

.. warning::

  This guide is *work in progress* and partly incomplete. If you notice any errors, even minor, or have suggestions for improvements, please inform Benni and/or Yunus of them. Thank you! :)

In this tutorial, you will learn how to configure, run, and evaluate the models already implemented in *Utopia*.
After having worked through it, you can apply the learned concepts and methods to all available models; the content and available parameters may change but the structure remains the same.
You can also use this tutorial as a reference guide, in which you can look up how to run a model simulation or configure plots.

This requires that you already installed *Utopia* as described in the :doc:`README <../readme>`. If you have not done so already, please do so now.

Also note, that this guide does not go into how building your own model in *Utopia* works; for that please refer to :doc:`how-to-build-a-model`.
Before you start building your model, however, you should have familiarized yourself with the core concepts of *Utopia*, as covered by this tutorial.

So far for an introduction. Let's dive in! 🤓

.. contents::
   :local:
   :depth: 2

.. note::

  These boxes will be used below to give additional remarks. They are not strictly required to follow the tutorial and you can skip them, if you want.

.. warning::

  These boxes, on the other hand, contain important information that you should have a look at.

----

.. _activate_venv:

Getting started
---------------

*Utopia* can be controlled fully from its command line interface (CLI). And that's what we're going to start with.

First thing is to make the CLI accessible. For that, go to the ``utopia`` directory and run the following commands:

.. code-block:: bash

   cd build             # ... to go to the utopia build directory
   source ./activate    # ... to enter the virtual environment

*Utopia* operates inside a so-called virtual environment. With the ``source ./activate`` command, you have entered it and should now see ``(utopia-env)`` appearing on the left-hand side of your shell.
When working with *Utopia*, make sure to always be in this virtual environment.

.. note::

    **Virtual environment:** Simplified, you can see a virtual environment as some kind of separate operating system within your operating system.
    That means, it is possible to install software in this virtual environment without installing it on your operating system. 
    Also, if something goes really wrong such that it would break your operating system, just the virtual environment is broken and your actual system is fine. :)
    Like this, it is much simpler to assure that everything works without problems on every maschine.
    The marker ``(utopia-env)`` tells you that you are within the Utopia virtual environment in your terminal session.

Let us now look at how to run a model using the CLI. Run on the command line:

.. code-block:: bash

   utopia run --help

You should now get a wall of text. Don't be scared, it is your friend. 
This command shows you all the possible parameters that you might need to run your model. 
Run it whenever you forgot what options you have. 

At the very top of the printed out text, you will see the ``usage`` specified, with optional parameters in square brackets. As you see there, the only *non*-optional parameter is the name of the model you want to run.
For testing purposes, a ``dummy`` model is available. Let's try it out:

.. code-block:: bash

   utopia run dummy

This should give you some output and, ideally, end with the following line:

.. ::

  INFO   utopia       All done.

If that is the case: Congratulations! You just ran your first (dummy) Utopia simulation. :)

If not, you probably got the following error message:

..  ::

  FileNotFoundError: Could not find command to execute! Did you build your binary? 

Alright, so let's build the ``dummy`` binary: Make sure you are in the ``build`` directory and then call ``make dummy``. After that command succeeds, you will be able to run the dummy model.


.. note::

    The CLI you interacted with so far is part of the so-called *Utopia* **Frontend**. It is a Python framework that manages the simulation and evaluation of a model.
    It not only supplies the CLI, but also reads in a configuration, manages multi-core simulations, provides a plotting infrastructure and more.
    As mentioned, the frontend operates in a virtual environment, in which all necessary software is installed in the required version.

Warm-up Examples
^^^^^^^^^^^^^^^^

Let us go through a couple of examples to show how flexible and interactive *Utopia* can be just from the command line.

* ``utopia run dummy --no-plot`` will run the model without creating any plots. It can be useful if you are only interested in the data created or the terminal output.
* ``utopia run dummy --set-params dummy.foo=1.23 dummy.bar=42`` allows to set model specific parameters (here: ``foo`` and ``bar`` of the ``dummy`` model) directly from the command line.
* ``utopia eval dummy`` loads the data of the previous simulation of the named model and performs the default evaluation on it
* ``utopia eval dummy --plot-only the_plot_I_am_currently_working_on`` only creates the plot with the specified name

Notice that ``utopia eval`` uses the ``eval`` subcommand. You can run ``utopia -h`` to see what other subcommands are available.

Now you should be reasonably warmed-up with the CLI. Let's get to running an actual simulation.


Running a Simulation
--------------------

Diving deeper into *Utopia* is best done alongside an actual model implementation; here, let's go with the ``SandPile`` model.
Due to its simplicity, this model is the perfect place to start, allowing you to focus on how *Utopia* works.

The ``SandPile`` model
^^^^^^^^^^^^^^^^^^^^^^

The ``SandPile`` model is a simple cellular automata model first described in the seminal work by `Bak et al. <https://doi.org/10.1103/PhysRevLett.59.381>`_ in 1987. 
It models heaps of sand and how their slope differ from a critical value. For more information on the model see the CCEES lecture notes, chapter 7.2.

You can also check out the corresponding :doc:`model documentation <../models/SandPile>`.


Run the model and see what happens
""""""""""""""""""""""""""""""""""

Let us run the model:

.. code-block:: bash

   utopia run SandPile

You see how easy it is to run a model? 🙂
But where are the simulation results?

Navigate to your home folder. You should find a folder named ``utopia_output``.
Follow the path ``~/utopia_output/SandPile/YYMMDD-hhmmss/``, where ``YYMMDD-hhmmss`` is the timestamp of the simulation, i.e., the date and time the model has been run. (More on this `below <#directory-structure>`_.)

You should see three different folders:

* ``config``: Here, all the model configuration files are stored. You already learned how to set parameters in the terminal through the command line interface. But from the number of files inside the folder you can probably already guess that there are more options to set parameters. You will explore the possibilities below.
* ``data``: Here, the simulation data is stored. 
* ``eval``: Here, the results of the data evaluation are stored. All saved plots are inside this folder.

This directly structure already hints at the three basic steps that are executed during a model run:

1. Combine different configurations, prepare the simulation run(s) and start them.
2. Store the data
3. Read in the data and evaluate it through automatically called plotting functions.

.. note::

  The ``utopia`` CLI commands always attempt to run through completely and only stop if there were major problems.
  So, always check the terminal output for example if you are missing plotting results! All errors will be printed out. To increase verbosity, you can add the ``--debug`` flag to your commands.

So, to get an idea of how the simulation went, let us have a look at the ``SandPile`` model plots. These are plots implemented alongside the model that show the relevant model behaviour. `Below <#plotting>`_, you will learn how to adjust these plots; for now, let us use these only to understand the behaviour of changes in the model parameters.

Navigate to the ``eval/YYMMDD-hhmmss/`` folder and open ``state_mean.pdf``. 
Inside of the eval folder there is again a time-stamped folder.
Every time you evaluate a simulation, a new folder is created. 
Like this, no evaluation result is ever overwritten.

The ``slope.pdf`` file contains the plot of the mean slope over time. 
You can see that only four time steps are shown. 
That is because by default *Utopia* runs 3 iteration steps producing four data points taking into account the initial state. 
You can run 

.. code-block:: bash

   utopia run SandPile --num-steps 10000

and open the new plot (remember to go down the new data tree). It should show a more interesting plot now. You can also look at the plot for the area distribution in the ``compl_cum_prob_dist.pdf`` file.


Directory structure
^^^^^^^^^^^^^^^^^^^

Let's take a brief detour and have a look at the directory structure of the *Utopia* repository, the output folder and where you can place the configuration files you will need in the rest of this tutorial.

Assuming that you installed *Utopia* inside your home directory, the directory structure should look similar to the following (only most relevant directories listed here):

::

  ~                          # Your home directory (or another base directory)
  ├─┬ Utopia                 # All the Utopia code
    ├── ...                  # Can have other Utopia-related code here
    └─┬ utopia               # Utopia repository
      ├── build              # Build results
      ├─┬ include            # The Utopia backend C++ library
        └─┬ utopia
          ├── core           # Utopia core structures
          └── data_io        # Data input and output library
      ├─┬ src
        └─┬ models           # The model implementations
          ├── ...
          └── SandPile
      ├─┬ python             # All python code
        ├─┬ model_plots      # Model-specific plots
          ├── ...
          └── SandPile
        ├─┬ model_tests      # Model-specific (Python) tests
          ├── ...
          └── SandPile
        └── utopya           # The Utopia frontend
      └── ...

This might be a bit overwhelming, but you will soon know your way around this.

You are already familiar with the ``build`` directory, needed for the build commands and to enter the virtual environment. Other important ones will be the model implementations and the model plots; you can ignore the others for now.

The *Utopia* frontend also took care of creating an ``utopia_output`` directory, which by default is inside your home directory. The output is ordered by the name of the model you ran and the timestamp of the simulation:

::

  ~                          # Your home directory (or another base directory)
  ├── Utopia                 # All the Utopia code
  ├─┬ utopia_output          # The Utopia output folder
    ├── ...                  # Other model names
    └─┬ SandPile             
      ├─┬ YYMMDD-hhmmss      # Timestamp of a simulation run
        ├── config           # Config files used in the simulation run
        ├── data             # Raw output data
        ├─┬ eval             # Plots
          ├─ YYMMDD-hhmmss   # ... created at one time
          ├─ YYMMDD-hhmmss   # ... created at another time
          ├─ ...             # ... even more plots
      ├── ...
      └── YYMMDD-hhmmss      # Timestamp of another simulation run

As *Utopia* makes frequent use of configuration files, let's take care that they don't become scattered all over the place.
It makes sense to build up another folder hierarchy for each model, which helps you organize the different *Utopia* run and evaluation settings for different models:

::

  ~                          # Your home directory (or another base directory)
  ├── Utopia                 # All the Utopia code
  ├── utopia_output          # The Utopia output folder
  └─┬ utopia_cfgs            # Custom config files (needs to be created manually)
    ├── ...                  
    └─┬ SandPile             
      └─┬ test               # Configuration files for a test run ...
        ├─ run.yml           # ... specifying one run
        └─ plots.yml         # ... specifying the plots for this run

In this example, the ``test`` directory holds the configuration files for the test runs of the ``SandPile`` model, i.e.: this tutorial.

.. note::

  The above is the directory structure this tutorial will follow. You are free to do it in another way, just take care to adapt the paths given in this tutorial accordingly.

    - Utopia need not be installed in the home directory; it can be where it suits you.
    - The configuration file directory can also be anywhere, but it makes sense that it's somewhere easily accessible from the command line.
    - For changing the output directory, have a look at the corresponding question in the :doc:`FAQ <../faq/frontend>` to see, how this is done.

  In fact, the more natural place, for the ``utopia_cfgs`` would be within the
  top-level ``Utopia`` directory, right *beside* the ``utopia`` repository.
  But you are free to choose all that. :)


Change parameters
^^^^^^^^^^^^^^^^^

Alright, back to the model now.

What is this business with the model files and how can you actually change the model parameters? Enter: Your first configuration file:
  - If you have not done so already, create the ``~/utopia_cfgs/SandPile`` directory
  - In it, to keep things sorted, create another directory named ``test``
  - Inside of the ``~/utopia_cfgs/SandPile/test/`` folder create an empty ``run.yml`` file

Now, copy the following lines into it:

.. code-block:: yaml

  ---
  # The run.yml configuration file for a test simulation of the SandPile model.
  parameter_space:
    # Number of simulation steps
    num_steps: 2000

The syntax you see here is called `YAML <https://en.wikipedia.org/wiki/YAML>`_, a human-readable markup language. We (and many other projects) use it for configuration purposes, exactly because it is so easy to write and read.
Just to give you an idea: A key-value pair can be specified simply with the ``key: value`` string. And to bundle multiple keys under a parent key, lines can be indented (here: using two spaces), as you see above.

.. note::

  In Utopia, all files with a ``.yml`` endings are configuration files. 
  To learn more about YAML, you can have a look at `learnXinYminutes tutorial <https://learnxinyminutes.com/docs/yaml/>`_ or search for others on the internet.

As you can see, the parameters are all bundled under the ``parameter_space`` key. With the above configuration, you set the number of iteration steps to ``2000``, overwriting the default value of ``3``.

Remember that every parameter you provide here will overwrite the default parameters. However, this is only the case if you put them in the correct location – in other words: the correct indentation level is important!

Now, you can run the model with the new parameters by passing the configuration file to the CLI:

.. code-block:: bash

   utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml

The path to the run configuration is placed directly behind the model name.
The model should then run for 2000 iteration steps. So, let us go and check the resulting plot.
If everything went correctly, the ``slope.pdf`` should show a plot with 2001 data points.

If you recall, you have already encountered a possibility to change parameters using the CLI and adding the parameters directly after the ``utopia run`` command.
So, let us suppose that we have the run configuration from above and add something to the CLI, like this:

.. code-block:: bash

  utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml --num-steps 1000

How many time steps will the model run?

The answer is: 1000 steps. Parameters provided in the CLI overwrite parameters from configuration files!
This gives you more flexibility for trying out parameters quickly.
You can also see that in the log messages, where it will say something like:

.. ::

  $ utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml --num-steps 1000
  INFO     utopia         Parsing additional command line arguments ...
  INFO     utopia         Updates to meta configuration:

  parameter_space: {num_steps: 1000}

  INFO     multiverse     Initializing Multiverse for 'SandPile' model ...
  INFO     multiverse     Loaded meta configuration.
  ...


Of course, often you want to change more parameters, especially model specific ones. At the same time, you might want to leave some of the default parameters as they are.
To that end, *Utopia* follows an approach where you can import the default parameters and then overwrite them. To do so, expand your ``run.yml`` file such that it looks like this:

.. code-block:: yaml

  # The run.yml configuration file for a test simulation of the SandPile model.
  ---
  parameter_space:
    # Number of simulation steps
    num_steps: 2000

    # Write out step size
    write_every: 1

    # The random number generator seed
    seed: 42

    # Below, you can update SandPile model specific parameters that will overwrite the _default_ ones.
    SandPile: !model
      model_name: SandPile
      # The above two lines import the model's _default_ configuration.
      # Below, you can make updates to these values. Only add the values you
      # want to _change_ from the defaults.
      # ...

Notice, that there now is a whole ``SandPile:`` key. This is the part of the configuration that is available to the ``SandPile`` model. The model will have access only to parameters below this key.

You will also notice the ``!model`` behind the key; that is a so-called YAML tag. It is used to denote that the defaults for the ``model_name: SandPile`` are to be loaded into this level of the configuration. This way, you only have to specify the keys you would like to *update*.
Do not forget to provide the ``!model`` tag and the ``model_name`` key, otherwise the default model parameters will not be loaded and you might be missing crucial parameters.

So far, so good. But what are the model's default parameters? Each models
default configuration is included in its documentation. Make sure you have
built the documentation as described in the README and then open it. In the
documentation for the ``SandPile`` model you will find a section with the
default parameters: It looks something like this:

.. code-block:: yaml

  # --- Space parameters
  # The physical space this model is embedded in
  space:
    periodic: false
  
  # --- CellManager
  cell_manager:
    grid:
      structure: square
      resolution: 16      # in cells per unit length of physical space
      # NOTE A large number of cells can make the initialization take a while...
  
    neighborhood:
      mode: vonNeumann
  
  # --- Dynamics
  # The initial slope range.
  initial_slope: [5, 6]
  # Cells are randomly initialized using a uniform distribution in the given
  # closed range. The first value is the lower limit and the second one the
  # upper limit of the slope.
  
  # The critical slope; beyond this value, sand topples
  critical_slope: 4

.. note::

  You can also locate the default model configuration of the ``SandPile``
  model at ``src/models/SandPile/SandPile_cfg.yml``. This file really is only
  for *looking*; to change parameters, there is the ``run.yml`` file.

So, let's change the grid resolution to a more interesting value. In your
``run.yml``, add the following entry:

.. code-block:: yaml

  cell_manager:
    grid:
      resolution: 32

Make sure, it is at the correct indentation level (inside the ``SandPile``
model). As is clear from the configuration keys, this changes the grid
resolution in the so-called cell manager to :math:`32` cells per unit length
of the physical space.

Run the model again and look at the resulting plots. What happened?

By the way: What you learned here, applies also to all other models.
You just need to know the model specific parameters, which you can always find in the model configuration located at ``utopia/src/models/<model_name>/<model_name>_cfg.yml``.
So, just check out another model and change parameters if you like. 😎

.. note:: 

  **Changing the model configurations:** Technically, it is possible to change the model parameters in the file where the defaults are specified.
  However, this is **not** advisable at all! As the name says, these files are to carry the *default* parameters and are not expected to change. 
  Instead write your own run configuration files as described in this section.
  This ensures inter alia that all models always work with their default configuration and that tests are guaranteed to run quickly and pass.
  Basically, you prevent the universe from collapsing.

.. warning::

  **Configuration files:** In Utopia, nearly every option can be set through a configuration parameter.
  With these, it is important to take care of the correct indentation level.
  If you place a parameter at the wrong location, it will often be ignored, sometimes even without warning! A common mistake at the beginning is to place model specific parameters outside of the ``!model`` scope (see text).

.. warning::
  Take care to choose model parameters wisely:

  1. Parameters such as ``grid_size`` can lead to a dramatically increased computation time,
  2. Some parameters have requirements which can also depend on other parameters. If this is the case, you normally find a comment above the corresponding parameters.

.. note::

  **User configuration:** It is possible to create a so-called *user configuration file*. This file contains all settings that are user- or machine-specific such as on how many cores to run a simulation or where to store the output data.
  See how to create a user configuration by typing ``utopia config --help`` in your terminal (be sure to be in the virtual environment).
  For more information, have a look at the :doc:`FAQ <../faq/frontend>`.


Plotting
--------

*Utopia* aims to make it easy to couple the simulation of a model with its evaluation. To that end, the *Utopia* frontend provides a plotting framework, that loads the generated simulation data and can provide it to plotting functions, which then take care of the evaluation of the data.

There are multiple ways in which plots can be generated:

* Each model can implement model-specific plot functions
* General plotting functions are available (to avoid recreating code over and over)
* External Python plotting scripts can be specified

Like many other parts of *Utopia*, this relies on a *YAML*-based configuration interface in which the plotting function to be used is specified and the parameters can be passed.

First, let's look at how a custom configuration can be used to adjust the behavior of existing model plots. Let's assume that – using the above steps – you have arrived at a run configuration you are happy with and you now want to run a simulation and afterwards create some plots from it.


Creating the simulation data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To not re-run simulations all the time (you would and could not do that after a very long simulation), let us first create some simulation data and then focus only on evaluating it:

.. code-block:: bash

  utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml --no-plot

The ``--no-plot`` leads to the run being stopped after the simulation finished. You can now invoke the evaluation separately:

.. code-block:: bash

  utopia eval SandPile

This will load the data of the *most recent* simulation run and perform the default plots.
You will see that a new folder has been created in the ``eval`` folder of the most recently run ``SandPile`` simulation. The evaluation results are placed in a new subfolder with the timestamp of the ``utopia eval`` invocation.

.. note::

  If you want to do the same with some other simulation output (that is not the most recent), you have to specify either a path to the run directory (can be absolute or relative) or its timestamp; ``utopia eval`` will do its best to find the desired directory.
  Check the log output if the correct directory was identified and, as always, see ``utopia eval --help`` for... well: help.

Customizing default plots
^^^^^^^^^^^^^^^^^^^^^^^^^

Let us now customize the default plots.

Above, you used the ``~/utopia_cfgs/SandPile/test/run.yml`` file to configure the *simulation*. For *plots*, you can do just the same: Create a new file named ``plots.yml`` right beside the already existing configuration file.

Now, what will have to go into that file in order to manipulate the existing plot configuration?

Plots go by a name. To find out the names of the configured plots, let's first have a look at the terminal output: In the log messages you will see output from the ``plot_mngr`` module informing you about which plots were performed:

.. ::

  ...
  INFO     utopia         Plotting...
  INFO     plot_mngr      Performing plots from 2 entries ...
  INFO     plot_mngr      Performing 'slope' plot ...
  INFO     plot_mngr      Finished 'slope' plot.
  INFO     plot_mngr      Performing 'compl_cum_prob_dist' plot ...
  INFO     plot_mngr      Finished 'compl_cum_prob_dist' plot.
  INFO     plot_mngr      Successfully performed plots for 2 configuration(s).                                                                          
  INFO     utopia         Plotting finished.

As you see there, two plots are configured under the names ``slope`` and ``compl_cum_prob_dist``. To find out more, locate the corresponding plot configuration in the model directory: ``utopia/src/models/SandPile/SandPile_plots.yml``.

There, you will find the same names as extracted from the log as keys on the root level of the configuration file. It looks something like this:

.. code-block:: yaml

  # Plot the slope (mean - critical_slope)
  slope:
    creator: universe
    universes: all
    
    # Use the SandPile-specific plot functions
    module: model_plots.SandPile
    plot_func: slope

    # Arguments passed to plt.plot
    linestyle: 'None'
    marker: '.'

  # Plot the complementary cumulative probability distribution
  compl_cum_prob_dist:
    creator: universe
    universes: all
    
    # Use the SandPile-specific plot functions
    module: model_plots.SandPile
    plot_func: compl_cum_prob_dist

    # Arguments passed to plt.plot
    linestyle: 'None'
    marker: '.'

Let's adjust the ``slope`` plot function. To that end, copy the corresponding configuration into your ``plots.yml`` file. Make sure it works by calling:

.. code-block:: shell

  utopia eval SandPile --plots-cfg ~/utopia_cfgs/SandPile/test/plots.yml

Confirm in the logs that only the ``slope`` plot was created. Now check out the run directory, where a new directory inside ``eval`` (with the current timestamp) will hold the plot output.

Feel free to customize the plot configuration by changing parameters in the ``plots.yml`` file. Does it have any effect to change the name of the plot? What happens when you add more arguments below ``marker``?

.. note::

  You can run the CLI in debug mode, which will produce tracebacks and help you understand what's going on: ``utopia eval <model_name> --debug --plots-cfg <path/to/plots.yml>``.
  This is very useful when you run into errors in the plot functions, as the program then stops and gives you more information on what went wrong.

.. warning::

  As with the default model configuration, the default plot configuration is best left untouched. **To modify it, you should always pass a new plot configuration.**
  Note that, currently, the configuration you are passing to the CLI is not updating the existing default plots.

As you see, you can change *some* of the parameters of the plots; but only the ones the person who implemented the plot function chose to expose. Further along this cook book, you will see how you can define your own plotting functions.


Animations
^^^^^^^^^^

At one point you might be interested in making cool animations of the state variables, but you might be deterred as it is generally hard to do. Not with *Utopia*\ !
In fact, it is part of the default plotting system. You might have noticed the corresponding plot configuration already. It looks something like this:

.. code-block:: yaml

  # Plot an animation of the CA state and save as individual frames
  slope_anim: &slope_anim
    enabled: false

    creator: universe
    universes: all

    module: .ca
    plot_func: state_anim

    # Select the model name; determines where to read the data from
    model_name: SandPile

    # Select the properties to plot
    to_plot:
      # The name of the property to plot with its options
      slope:
        title: Slope
        limits: [1, 4]
        cmap: copper

    writer: frames  # can be: frames, ffmpeg (if installed), ...
    
    # ...

Quite a few more parameters here. Let's try and understand the most important ones:

* ``enabled: false`` is used here to disable the plot by default
* ``module: .ca`` now refers to an internal (denoted by the leading dot) plotting module for cellular automata
* ``model_name: SandPile`` tells the plot function to use the data of that model
* ``to_plot`` allows specifying which properties to plot. You can also add more properties here and it will access the data depending on the name of the property.

To play around with this, again: copy the configuration over from the ``SandPile_plots.yml`` into your ``plots.yml``.
First thing to change would be to remove the ``enabled: false`` entry.
Run ``utopia eval`` with this plot configuration now and see what happens.

You can try the following things to get to know the capabilities of the ``state_anim`` plotting function:

* Change the ``cmap``
* Change the ``limits`` argument
* If you have ``ffmpeg`` installed, change the ``writer`` argument
* Try to add another property. To know which name to use, check out the printed data tree in the terminal log. (Be careful with indentation levels)

Now that your animation is configured, you might want to run a simulation with a larger grid and more time steps. Go for it! :)

.. warning::

  Before you launch some cool million-step simulation on a ``1024 x 1024`` grid, remember that it all needs to be stored somewhere and this might either flood your RAM or your hard drive / SSD ... or both.

.. note::

  If you *just* want to enable a disabled default plot and not change anything in the plot configuration, the CLI is here to help:
  ``utopia eval <model_name> --plot-only <plot_name1> <plot_name2> ...``.
  As always, check out ``utopia eval --help`` for more info.

Using your own plot functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now you might want to have a bit more flexibility with what you plot. For example, you might be interested in plotting the standard deviation for the level of the cells for each time step.
There is no default plot function that does that, but you can use the plot configuration to specify which python script to use to do the plot.

In the above configurations, the ``module`` key was used to specify which module to use. To load a file as a module, use the ``module_file`` key and then insert the absolute path for your function file. You can use the ``~`` character to resolve your home directory.

.. code-block:: yaml

   state_std:
     # Load the following file as a python module:
     module_file: ~/path/to/my/python/script.py

     # Use the function with this name from that module:
     plot_func: plot_state_std

     # All other arguments (as usual) ...
     # Select a creator (which fits the function signature)
     creator: universe
     universes: all

     # ... arguments passed on to the plot_state_std function

Now we need to write a plot function that accepts the loaded data. For the ``plot_state_std`` function, it makes sense to use the ``universe`` plot creator, which allows making a plot for each universe.

The function that is being implemented thus needs to have the following form:

.. code-block:: python

  from utopya import DataManager, UniverseGroup

  def universe_plot(dm: DataManager, *,
                    out_path: str,
                    uni: UniverseGroup,
                    **additional_kwargs):
      """Signature required by the `universe` plot creator.

      Args:
          dm: The DataManager object that contains all loaded data.
          out_path: The generated path at which this plot should be saved
          uni: Contains the data from a single selected universe
          **additional_kwargs: Anything else that was defined in the plot
              configuration. Consider declaring the keywords explicitly
              instead of using the ** to gather all remaining arguments.
      """
      # ... your code here ...

      # Save to the specified output path
      plt.savefig(out_path)

Let's fill that in with the code that calculates the standard deviation for the state. The complete python code then is:

.. code-block:: python

  import numpy as np
  import matplotlib.pyplot as plt

  from utopya import DataManager, UniverseGroup

  def plot_state_std(dm: DataManager, *,
                     out_path: str,
                     uni: UniverseGroup):
      """Signature required by the `universe` plot creator.

      Args:
          dm:       The DataManager object that contains all loaded data.
          out_path: The generated path at which this plot should be saved
          uni:      Contains the data from the single selected universe
      """

      # Get the slope data and calculate the standard deviation
      slope = uni['data/SandPile/slope']
      slope_std = np.std(slope)

      # Get the corresponding x-values, i.e.: the time steps
      times = uni.get_times_array()

      # Call the plot function
      plt.plot(times, slope_std)

      # Set the aesthetics
      plt.xlabel("Time [steps]")
      plt.ylabel("Sand Slope Std. Dev.")

      # Save the figure to the specified output path
      plt.savefig(out_path)

For more information on possible plot signatures, consult the :doc:`FAQs <../faq/frontend>`.



Parameter Sweeps and the ``Multiverse``
---------------------------------------

Alright. With the above, you already came in touch with a lot of the features of *Utopia*.
In this section, you will learn how to perform multiple simulations for different sets of parameters, and how you can handle them in plotting.

Furthermore, this section will use a different model for the examples. This is to show that what you learned above can be applied to other models in *Utopia*, not only the ``SandPile`` model.


The ``ForestFire`` Model
^^^^^^^^^^^^^^^^^^^^^^^^

This is a cellular automaton model, where a cell can either have the state ``empty`` or can be a ``tree``.
Each tree can ignite with a certain probability, which will lead to whole tree cluster (i.e., all connected trees) burning down. You can find out more in the :doc:`model documentation <../models/ForestFire>`.

Let's dive right in and have a short test run of this model:

.. code-block:: shell

  utopia run ForestFire

Just as for the ``SandPile`` model, it will create and run a simulation with 4 time steps.

.. note::

  Make sure you have built the ``ForestFire`` binary before trying to run it. Follow the steps in the `getting started section <#getting-started>`_, if you are stuck here.


Parameters
""""""""""

For getting to know the parameters available to the ``ForestFire`` model, let's have a look at that model's default configuration by opening its description in the Utopia documentation.
It looks something like this:

.. code-block:: yaml

  # --- Space
  space:
    periodic: true
  
  # --- CellManager and cell initialization
  cell_manager:
    grid:
      structure: square
      resolution: 64      # cells per unit length of space's extent
  
    neighborhood:
      mode: Moore         # can be: empty, vonNeumann, Moore
  
    # Initialization parameters for each cell
    cell_params:
      # Initial tree density, value in [0, 1]
      # With this probability, a cell is initialized as tree (instead of empty)
      initial_density: 0.2
  
  # --- Model Dynamics
  # Probability per site and time step to transition from state empty to tree
  growth_rate: 7.5e-3
  
  # Probability per site and time step to transition to burning state, burning
  # down the whole cluster
  lightning_frequency: 1.0e-5 
  
  # If set to true, the bottom boundary is constantly ignited. This _requires_
  # the space to be set to non-periodic.
  light_bottom_row: false
  
  # Probability (per neighbor) to _not_ catch fire from a neighbor
  resistance: 0.


To *change* these parameters, you again need to create a run configuration file, e.g. ``~/utopia_cfgs/ForestFire/test/run.yml``. In it, let's change the initial density of trees to zero:

.. code-block:: yaml

  # A test configuration for the ForestFire model
  ---
  # Frontend configuration parameters
  # ...

  # What is passed to the C++ side (_after_ the frontend prepared it)
  parameter_space:
    num_steps: 1000
    seed: 42

    ForestFire: !model
      model_name: ForestFire
      # The above two lines import the model's _default_ configuration
      # Below, you can make updates to these values. Only add the values you
      # want to _change_ from the defaults.

      # Set the initial tree density, value in [0, 1]
      cell_manager:
        cell_params:
          initial_density: 0.0

You will surely see similarities to the run configuration used in the ``SandPile`` model. Again, the model-independent parameters are on the top level inside the ``parameter_space``: ``num_steps`` and ``seed`` (and others that we are not overwriting here).
As above, the model-specific default parameters are imported using the ``!model`` tag, where ``model_name`` specifies the parameters to import.

Now, pass the configuration to the CLI:

.. code-block:: bash

  utopia run ForestFire ~/utopia_cfgs/ForestFire/test/run.yml

Compare the output with that with non-zero initial density. What happens when you turn on percolation mode? Feel free to play around. :)


Parameter sweeps
^^^^^^^^^^^^^^^^

Often times when analyzing a model, it becomes necessary to compare the behaviour of the model for different sets of parameters. For example, in the case of the ``ForestFire`` model, one would want to extract the effect of the ``lightning_frequency`` parameter on the cluster size, and run different simulations for different values of these parameters to achieve that.

Another use case for running multiple simulations is that you might want to generate some statistics by averaging over mutliple simulation runs. To that end, one would change the ``seed`` parameter that is used to initialize the random number generator; by choosing a different seed, the sequence of random numbers in the probabilistic functions is changed.

.. note::

  Always specifying the ``seed`` parameter also has the advantage of making the runs reproducible: With a fixed seed, a single simulation always has the same sequence of random numbers.

Adding a parameter sweep
""""""""""""""""""""""""

Let's start with this latter use case. Open the run configuration of the ``ForestFire`` model and change

.. code-block:: yaml

    seed: 42

to

.. code-block:: yaml

    seed: !sweep
      default: 42       # The value which is used if no sweep is done
      values: [1, 2, 3] # The values over which to sweep

This now says, that instead of using the default value for a single simulation, three simulations for the specified ``seed`` values are to be made. Let's see if it works:

.. code-block:: bash

  utopia run ForestFire ~/utopia_cfgs/ForestFire/test/run.yml --sweep

.. note::

  Do not forget the ``--sweep`` flag! This is required to tell *Utopia* that you want to run a parameter sweep. Alternatively, you can add a new entry ``perform_sweep: true`` to the *root level* of the configuration file, i.e. on the same level as the ``parameter_space`` key, with zero indentation.

You will see some log output from the ``multiverse``, stating that it is ``Adding tasks for simulation of 3 universes ...``.

Perhaps now is the time to talk about the nomenclature: In *Utopia*, a ``Multiverse`` is a set of several ``Universe``s, which are fully separated from each other: they can't interact in any way. This also means that each universe has a separate and *distinct* set of parameters. All universes live inside the multiverse. And, depending on the number of CPUs your machine has, they live (i.e., are being simulated) in parallel. And that's about where the analogy ends. ;)

After this brief detour, have a look at the output again. You will see how it is different to the one where you only run a single universe:

* You no longer see the direct simulation output, as this would flood the terminal.
* The progress bar now behaves differently.
* When loading the data, you see a larger data tree.

This already suggests, that more data was written. You can confirm that by opening the output directory.

What about the plots? Check the ``eval`` directory of your latest run.
You will notice that the default plots were applied to each universe separately and are placed inside a folder; the file name now contains the coordinates of the point in parameter space.

Adding more parameter sweeps
""""""""""""""""""""""""""""

Adding parameter sweeps is super easy.
Basically, you only have to add the ``!sweep`` indicator behind the parameter and specify the values (take care of the indentation value). That's it.

All parameters within the ``parameter_space`` level allow this yaml tag. There are, of course, more ways to specify parameters than explicitly giving the ``values``.
Let's change the ``lightning_frequency`` parameter in the run configuration and use logarithmically spaced values:

.. code-block:: yaml

      lightning_frequency:
        default: 1.0e-5
        logspace: [-5, -2, 7]  # 7 log-spaced values in [10^-5, 10^-2]
                               # Other ways to specify sweep values:
                               #   values: [1,2,3,4]  # taken as they are
                               #   range: [1, 4]      # passed to python range()
                               #   linspace: [1,4,4]  # passed to np.linspace
                               #   logspace: [0,2,3]  # passed to np.logspace

As you see, the ``values`` key was exchanged for the ``logspace`` key. Under the hood, the given list is unpacked into a certain python function, as noted above. This allows many ways to specify parameter dimension values.

Together with the three values for the ``seed`` dimensions, there are now 21 possible combinations of parameters. When you run the simulation, you will see exactly that: ``Adding tasks for simulation of 21 universes ...``

.. note::

  If you sweep over multiple parameters, all possible parameter combinations will be used, i.e. the cartesian product of each ``!sweep``-specified set of dimensions.
  With :math:`P_1 ... P_n` sweep definitions, you'll get an :math:`n`-dimensional parameter space with :math:`\Pi_{i=1}^n |P_i|` possible combinations.
  In other words: You'll quickly be in touch with the curse of dimensionality.

.. note::

  When you have a look at the output folders (which are just the names of the universes, e.g. ``uni23``) you might notice that they do not start at zero and might have gaps in between them.
  No need to worry if a universe is missing: This is because each point in parameter space needs to be associated with an index, and this includes the default values for each parameter dimension.
  To be consistent, the zero index of each parameter dimension maps to the default. Thus, the sweep values begin at index 1 and result in the pattern of indices you see.

There are a bunch of other things to do with parameter sweeps, which go beyond the scope of this cook book. (If you're keen to explore the features, you can have a look at the underlying `paramspace package <https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace>`_.)

As you see, parameter sweeps can be used to easily create huge amounts of data. And we all know: With (hopefully) great data, comes great responsibility.
Thus, let's now focus on how the plotting framework can be used to handle the multidimensional data.



Multiverse plots
^^^^^^^^^^^^^^^^

Let us plot the mean states of the mean universe states and use the run configuration of the previous part.

Recall, that in the ``SandPile`` model, you first created a plot configuration.
So, let us do it here equivalently: Create the file ``utopia_cfgs/ForestFire/test/plots.yml`` with the content:

.. code-block:: yaml

   # The plot.yml configuration file for a test simulation of the ForestFire model.
   ---
   ensemble_averaged_mean_state:
     # As you need the data of many universes, select the multiverse plot creator:
     creator: multiverse

     # The `select` key is used to select a hyperslab out of the data:
     select:
       field:
         # Choose the path in the data tree (see terminal output)
         path: data/ForestFire/state
         dims: [time, x, y]

     # Select the plot function just as for a universe plot
     module_file: ~/Path/to/my/plot/function/file.py
     plot_func: ensemble_averaged_mean_state

The file is located in the same directory as the run configuration to indicate that they belong together.

The short description of what you told *Utopia*'s frontend to do is:
Create a plot called ``ensemble_averaged_mean_state`` that should use all the multiverse data (``creator: multiverse``).
Make a ``select`` ion of data in a multidimensional array ``field`` and fill it with the data that you can find in the data tree of each single universe under the ``path: data/ForestFire/state``.
You can look up the path in the data tree that is printed out in the terminal.
This dataset has the following ``dims: [time, x, y]``. Remember, that you have two dimensional grid data for each time step.
This sets the names with which to access the individual dimensions separately on python side.
The dataset will obviously have another additional dimension, the ``seed`` you are sweeping over.
All this data is loaded into a `xarray <http://xarray.pydata.org/en/stable/>`_ ``DataSet`` object which will be given to the plotting function.

So, we roughly understand the first part of the configuration file.
Plotting will, however, not work: 
The second part of the configuration file states: Use the plotting function ``plot_func: ensemble_averaged_mean_state``, which is located in the ``module_file: ~/Path/to/my/plot/function/file.py`` and create the plot.
But this function does not exist yet.
So let us create it:

Create a file at a location of your choice, so let's choose: ``~/utopia_FFM_timeseries_plots.py`` 
(You would probably want to create it at another location 
– get inspired by the directory structure described earlier in this tutorial.).
Within this file, let us create the ``ensemble_averaged_mean_state`` function with the following content:

.. code-block:: python

  # User specific time-series plot for the ForestFire model.
  import numpy as np
  import matplotlib.pyplot as plt
  import xarray as xr

  from utopya import DataManager, UniverseGroup

  def ensemble_averaged_mean_state(dm: DataManager, *, 
                                  out_path: str, 
                                  # Here, you get the selected data
                                  mv_data: xr.Dataset,
                                  # Below, you can add further model specific arguments
                                  save_kwargs: dict=None, 
                                  **plot_kwargs):
      '''Plots the ensemble averaged mean state over multiple universes'''

      # Calculate the mean state averaged over all universes.
      # The mean is calculated over the dimensions: 'x', 'y', and 'seed'
      data = mv_data.mean(dim=['x', 'y', 'seed'])

      # Plot the data
      plt.plot(data['state'], **plot_kwargs)

      # Save and close the figure
      plt.savefig(out_path)
      plt.close()

Now, you still need to adapt the plot configuration from above because the path to python file (module) containing the plotting function is not set correctly yet.
So, adapt the parameter:

.. code-block:: yaml

    module_file: ~/utopia_FFM_timeseries_plots.py  # Choose the path, where you created the plot function!

Now, everything is ready and set such that the multiverse data, you have created in the previous run can be evaluated.
To do this, type the following command into your terminal:

.. code-block:: bash

  utopia eval ForestFire --plots-cfg ~/utopia_cfgs/ForestFire/test/plots.yml

Of course, if you want to do a new simulation run that creates new data you can also use the command that runs and afterwards directly evaluates the data:

.. code-block:: bash

  utopia run ForestFire ~/utopia_cfgs/ForestFire/test/run.yml --plots-cfg ~/utopia_cfgs/ForestFire/test/plots.yml

Now, go check the resulting plot. How does it look like? 

Of course, you would want to make the plot a bit more beautiful. 
For this, you can and should use the functionality to provide parameters in the plot configuration file. These are automatically available in the function body if you add the parameter key name as a function parameter.
This works exactly as in the plot creation for a single universe, described above.

In general, if you want your plot to be integrated into the *Utopia* model-specific plots,
you can add the plot function to a suitable file within the directory ``utopia/python/model_plots/ForestFire``.
However, this should only be done if the plot actually makes sense to have.
For the ``ensemble_averaged_mean_state`` this probably is not the case:

You could be wondering why this plot is not within the *Utopia* default plots.
Just ask yourself: Is it really necessary to do multiple realizations of the 
ForestFire model with just different random number seeds and average them? 
The answer is no because this system is ergodic.

So, always think about what you want to implement and whether it makes sense to do it or not.

To learn more about parameter sweeps, look at the :doc:`multidimensional data generation and plotting in Utopia  <../guides/parameter-sweeps>`

Closing Remarks
---------------

What did you learn?
^^^^^^^^^^^^^^^^^^^

Hopefully, you can answer this question by yourself. 🙂

Your learned the basics of:

- how to run any implemented model in *Utopia*,
- how *Utopia* is structured,
- what *Utopia* is capable of doing (at least the fundamental aspects),
- how to plot the generated data conveniently, and
- how to do parameter sweeps and plot them.

Summed up, you learned how to use the *Utopia* tool and the concepts that can be applied to all other models.

How to continue from here?
^^^^^^^^^^^^^^^^^^^^^^^^^^

Everything you learned in this somewhat unconventional cook book is generative. 
That means that you can apply your newly developed cooking skills to any model, 
following the philosophy: If you know how to boil water to cook some pasta you also know how to cook rice.
Of course, you will need to adjust some parameters.

So, just play around with different models and explore the world of chaotic, complex, and evolving systems. 🗺 ️

And, perhaps you even want to write your own *Utopia* model. Just follow the :doc:`Beginners Guide <../guides/beginners-guide>` ...

What if I have more questions?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Look if you can find some useful

- :doc:`documentation or guide <../index>`,
- questions and answers in the :doc:`FAQ <../faq/frontend>`, or
- information in the `C++ documentation <../../doxygen/html/index.html>`_.
