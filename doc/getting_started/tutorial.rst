.. _tutorial:

Tutorial
========

.. note::

  If you notice any errors in this tutorial, other parts of the documentation, or the code — even minor ones — please open an `issue <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues>`_ on the GitLab project page.

  In general, if you have *any* kind of question or remark regarding Utopia, you should **open an issue.**
  This applies even to small or very specific questions; other people might have the same small or very specific question.
  This approach preserves the question as well as the answer to it.

  Thank you! :)

In this tutorial, you will learn how to configure, run, and evaluate the models already implemented in Utopia.
After having worked through it, you can apply the learned concepts and methods to all available models; the content and available parameters may change but the structure remains the same.
You can also use this tutorial as a reference guide, in which you can look up how to run a model simulation or configure plots.

This requires that you already installed Utopia as described in the :doc:`README <../README>`.
If you have not done so already, please do so now.
Alternatively, you can use the **Utopia docker image**, which does not require an installation. For usage instructions, see `ccees/utopia <https://hub.docker.com/r/ccees/utopia>`_ on docker hub.
Note that some parts of the tutorial are not relevant when using Utopia from within the docker container.

Also, note that this guide does not go into how building your own model in Utopia works; for that, please refer to the :ref:`impl_step_by_step`.
Before you start building your model, however, you should have familiarized yourself with the core concepts of Utopia and the workflow of Git projects, as covered by this tutorial and the :ref:`dev_workflow` page, respectively.

So much for an introduction. Let's dive in!

.. contents::
   :local:
   :depth: 2

.. note::

  These boxes will be used below to give additional remarks. They are not strictly essential to the tutorial and you can skip them, if you want.

.. warning::

  These boxes, on the other hand, contain important information that you should have a look at.

.. hint::

  Read these boxes if you are running Utopia from within the `Utopia docker container <https://hub.docker.com/r/ccees/utopia>`_.

----

.. _activate_venv:

First steps
-----------
Utopia can be controlled fully from its command line interface (CLI). And that's what we're going to start with.

The first step is to make the CLI accessible.

.. hint::

  Inside the docker container, the CLI is always accessible.
  In other words, you are always within ``(utopia-env)``, even if the shell does not show that. You can thus skip the following commands.

To do so, go to the ``utopia`` directory and run the following commands (Do not copy the ``$``'s!):

.. code-block:: console

  $ cd build             # ... to go to the utopia build directory
  $ source ./activate    # ... to enter the virtual environment

Utopia operates inside a so-called virtual environment.
With the ``source ./activate`` command, you have entered it and should now see ``(utopia-env)`` appearing on the left-hand side of your shell.
When working with Utopia, make sure to always be in this virtual environment.

.. note::

    **Virtual Environment** A Python virtual environment is a software environment that is separated from the main Python environment on your machine.
    That way, the software installed into it does not interfere with the other Python software you have installed.
    The marker ``(utopia-env)`` tells you that you are within the Utopia virtual environment in your terminal session.

.. warning::

    If you are using ``csh`` or ``fish`` shells, you *cannot* use the command given above.
    The Python ``venv`` package provides different ``activate`` scripts for these shells, and they are located in ``build/utopia-env/bin``.
    In this case, execute

    .. code-block:: console

      $ cd build                           # Enter the Utopia build directory
      $ source ./utopia-env/activate.csh   # Activate venv for csh, OR ...
      $ source ./utopia-env/activate.fish  # ... for fish

Now let's look at how to run a model using the CLI. On the command line, run:

.. code-block:: console

  $ utopia run --help

You should get a wall of text. Don't be scared, all this log output is your friend.
The command shows you all the possible parameters that you might need to run your model. Use it whenever you forget which options you have.

At the very top of the log, you will see the ``usage`` specified, with optional parameters in square brackets. As you see there, the only *non*-optional parameter is the name of the model you want to run.
For testing purposes, a ``dummy`` model is available. Let's try it out:

.. code-block:: console

   $ utopia run dummy

This should give you some output and, ideally, end with the following line:

.. code-block:: console

  INFO   utopia       All done.

If that is the case: Congratulations! You just ran your first (dummy) Utopia simulation. :)

If not, you probably got the following error message:

.. code-block:: console

  FileNotFoundError: Could not find command to execute! Did you build your binary?

Alright, so let's build the ``dummy`` binary: Make sure you are in the ``build`` directory and then call ``make dummy``. After that command succeeds, you will be able to run the dummy model.


.. note::

  The CLI you interacted with so far is part of the so-called Utopia **Frontend**. It is a Python framework that manages the simulation and evaluation of a model.
  It not only supplies the CLI, but also reads in a configuration, manages multi-core simulations, provides a plotting infrastructure and more.
  As mentioned, the frontend operates in a virtual environment, in which all necessary software is installed in the required version.

.. note::

  The ``utopia`` CLI commands always attempts to run through completely and only stops if there were any major problems.
  So, make sure to always check the terminal output, for example if you are missing plotting results! All errors will be printed out. To increase verbosity, you can add the ``--debug`` flag to your commands.

Warm-up Examples
^^^^^^^^^^^^^^^^
Let us go through a couple of examples to show how flexible and interactive Utopia can be just from the command line.
Don't bother with the simulation output for now; that will be addressed soon.

* ``utopia run dummy --set-params dummy.foo=1.23 dummy.bar=42`` allows to set model specific parameters (here: ``foo`` and ``bar`` of the ``dummy`` model) directly from the command line.
* ``utopia run dummy --no-plot`` will run the model without creating any plots. It can be useful if you are only interested in the data created or the terminal output.
* ``utopia eval dummy`` loads the data of the previous simulation of the named model and performs the default evaluation on it.
* ``utopia eval dummy --plot-only state_mean`` only creates the plot with the specified name.
* ``utopia eval dummy --interactive`` starts an interactive plotting session.

Notice that ``utopia eval`` uses the ``eval`` subcommand. You can run ``utopia --help`` to see what other subcommands are available.

Now you should be reasonably warmed-up with the CLI. Let's get to running an actual simulation.


Running a Simulation
--------------------
Diving deeper into Utopia is best done alongside an actual model implementation; here, let's go with the ``SandPile`` model.
Due to its simplicity, this model is the perfect place to start, allowing you to focus on how Utopia works.

The ``SandPile`` model
^^^^^^^^^^^^^^^^^^^^^^
The ``SandPile`` model is a simple cellular automata model first described in the seminal work by `Bak et al. <https://doi.org/10.1103/PhysRevLett.59.381>`_ in 1987.
It models heaps of sand and how their slopes differ from a critical value. For more information on the model, check out the corresponding :doc:`model documentation <../models/SandPile>`.


Run the model and see what happens
""""""""""""""""""""""""""""""""""
Let us run the model:

.. code-block:: console

  $ utopia run SandPile

You see how easy it is to run a model? 🙂
But where are the simulation results?

Navigate to your home folder. You should find a folder named ``utopia_output``.
Follow the path ``~/utopia_output/SandPile/YYMMDD-hhmmss/``, where ``YYMMDD-hhmmss`` is the timestamp of the simulation, i.e., the date and time the model has been run (more on this :ref:`below <directory_structure>`.)

.. hint::

  Inside the docker container, the home directory of the ``utopia`` user is mapped to a local directory on your host machine, usually your current working directory.
  You will find the ``utopia_output`` there.

You should see three different folders:

* ``config``: Here, all the model configuration files are stored. You already learned how to set parameters in the terminal through the command line interface. But from the number of files inside the folder you can probably already guess that there are more options to set parameters. You will explore the possibilities in due course.
* ``data``: Here, the simulation data is stored.
* ``eval``: Here, the results of the data evaluation are stored. All saved plots are inside this folder.

This directory structure already hints at the three basic steps that are executed during a model run:

1. Combine different configurations, prepare the simulation run(s) and start them.
2. Store the data
3. Read in the data and evaluate it through automatically called plotting functions.

So, to get an idea of how the simulation went, let us have a look at the ``SandPile`` model plots. These are plots implemented alongside the model that show the relevant model behaviour. `Below <#plotting>`_, you will learn how to adjust these plots; for now, let us use these only to understand the behaviour of changes in the model parameters.

Navigate to the ``eval/YYMMDD-hhmmss/`` folder and open ``mean_slope.pdf``.
Inside the ``eval`` folder there is again a time-stamped folder.
Every time you evaluate a simulation, a new folder is created.
This way, no evaluation result is ever overwritten.

The ``mean_slope.pdf`` file contains the plot of the mean slope over time.
You can see that only four time steps are shown.
That is because by default Utopia runs three iteration steps producing four data points taking into account the initial state.
You can run

.. code-block:: console

  $ utopia run SandPile --num-steps 1000

and open the new plot (remember to go down the new data tree). It should show a more interesting plot now. You can also look at the plot for the complementary cumulative probability distribution in the ``compl_cum_prob_dist.pdf`` file or at any other plot.

Increasing ``num_steps`` can make the evaluation take quite a while, as you probably already saw for the movie creation of the simulation with 1000 iteration steps.
If you further increase ``num_steps``, you will most certainly need to select which plot to create (e.g. by appending ``--plot-only mean_slope`` to ``utopia run``).

Try the following to see how fast calculating the complementary cumulative probability distribution can be, even for a large number of iterations, when other, more time-consuming plots are ignored:

.. code-block:: console

  $ utopia run SandPile --num-steps 50000 --plot-only compl_cum_prob_dist

Alternatively, you can set the ``--write-every`` and ``--write-start`` parameters to decrease the amount of data that is written out. The choice obviously depends on what you want to simulate and investigate with the model.

.. _directory_structure:

Directory structure
^^^^^^^^^^^^^^^^^^^
Let's take a brief detour and have a look at the directory structure of the Utopia repository, the output folder, and where you can place the configuration files you will need in the rest of this tutorial.

Assuming that you have Utopia installed inside your home directory, the directory structure should look somewhat like this (only most relevant directories listed here):

.. code-block:: console

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
        └─┬ utopia
          └─┬ models         # The model implementations
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

.. hint::

  In the Utopia docker container, the ``utopia`` framework repository is located at ``/utopia``.

.. note::

  When working on a *separate* Utopia model repository, the structure is basically the same. The differences are the following:

  * The models are implemented in ``src/models`` instead of ``src/utopia/models``.
  * There is no ``include`` and no ``python/utopya`` directory.

This might be a bit overwhelming, but you will soon know your way around this.

You are already familiar with the ``build`` directory, needed for the build commands and to enter the virtual environment. Other important ones will be the model implementations and the model plots; you can ignore the others for now.

.. hint::

  Remember that what relates to the home directory below is your *mounted* directory in the docker container.
  Inside the docker container, the output directory is ``~/io/utopia_output``.

The Utopia frontend also took care of creating an ``utopia_output`` directory, which by default is inside your home directory. The output is ordered by the name of the model you ran and the timestamp of the simulation:

.. code-block:: console

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

As Utopia makes frequent use of configuration files, let's take care that they don't become scattered all over the place.
It makes sense to build up another folder hierarchy for each model, which helps you organize the different Utopia run and evaluation settings for different models:

.. code-block:: console

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

  The above is the directory structure this tutorial will follow. You are free to choose a different structure, just take care to adapt the paths given in this tutorial accordingly.

    - Utopia need not be installed in the home directory; it can be located wherever it suits you.
    - The configuration file directory can also be anywhere, though it makes sense to make it easily accessible from the command line.
    - For changing the output directory, have a look at the corresponding question in the :ref:`FAQ <faq_sim_control>` to see how this is done.

  In fact, the more natural place, for the ``utopia_cfgs`` would be within the
  top-level ``Utopia`` directory, right *beside* the ``utopia`` repository.
  But you are free to choose all that. :)


Changing parameters
^^^^^^^^^^^^^^^^^^^
Alright, back to the model now.

What is this business with the configuration files and how can you actually change the model parameters? Enter: your first configuration file:

  - If you have not done so already, create the ``~/utopia_cfgs/SandPile`` directory
  - In it, to keep things sorted, create another directory named ``test``
  - Inside of the ``~/utopia_cfgs/SandPile/test/`` folder create an empty ``run.yml`` file

.. hint::

    You can create a directory and an empty plain text file right from the terminal *inside* the docker container:

    .. code-block:: console

        $ mkdir -p ~/io/utopia_cfgs/SandPile/test
        $ touch ~/io/utopia_cfgs/SandPile/test/run.yml

    The created file can then be opened in the text editor of your choice.
    As the directory is mounted, you can also do this on the host system.

Now, copy the following lines into it:

.. code-block:: yaml

  # The run.yml configuration file for a test simulation of the SandPile model.
  ---
  parameter_space:
    # Number of simulation steps
    num_steps: 2000

The syntax you see here is called `YAML <https://en.wikipedia.org/wiki/YAML>`_, a human-readable data-serialization language. We (and many other projects) use it for configuration purposes, exactly because it is so easy to write and read.
Just to give you an idea: A key-value pair can be specified simply with the ``key: value`` string. And to bundle multiple keys under a parent key, lines can be indented (here: using two spaces), as you see above.

.. note::

  In Utopia, all files with a ``.yml`` endings are configuration files.
  To learn more about YAML, you can have a look at the `learnXinYminutes tutorial <https://learnxinyminutes.com/docs/yaml/>`_; there are of course plenty of others online.

As you can see, the parameters are all bundled under the ``parameter_space`` key. With the above configuration, you set the number of iteration steps to ``2000``, overwriting the default value of ``3``.

Remember that every parameter you provide here will overwrite the default parameters. However, this is only the case if you put them in the correct location – in other words: the correct indentation level is important!

Now, you can run the model with the new parameters by passing the configuration file to the CLI:

.. code-block:: console

  $ utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml

As you see, the path to the run configuration is just placed directly behind the ``model_name`` parameter.
The model should then run for 2000 iteration steps.

.. note::

  You can run the ``utopia`` command from *any* directory (as long as you are inside the virtual environment). Instead of giving absolute paths, you can also just move to a directory and pass a relative path to the CLI.

So, let us go and check the resulting plot.
If everything went correctly, the ``mean_slope.pdf`` should show a plot with 2001 data points.

If you recall, you have already encountered a possibility to change parameters using the CLI and adding the parameters directly after the ``utopia run`` command.
So, let us suppose that we have the run configuration from above and add another parameter to the CLI, like this:

.. code-block:: console

  $ utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml --num-steps 1000

How many iteration steps will the model run?

The answer is: 1000 steps. Parameters provided in the CLI overwrite parameters from configuration files!
This gives you more flexibility for trying out parameters quickly.
You can also see that in the log messages, where it will say something like:

.. code-block:: console

  $ utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml --num-steps 1000
  INFO     utopia         Parsing additional command line arguments ...
  INFO     utopia         Updates to meta configuration:

  parameter_space: {num_steps: 1000}

  INFO     multiverse     Initializing Multiverse for 'SandPile' model ...
  INFO     multiverse     Loaded meta configuration.
  ...


Of course, often you want to change more parameters, especially model specific ones. At the same time, you might want to leave some of the default parameters as they are.
To that end, Utopia follows an approach where you can import the default parameters and then overwrite them. To do so, expand your ``run.yml`` file such that it looks like this:

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
    SandPile:
      # Below, you can make updates to these values. Only add the values you
      # want to _change_ from the defaults.
      # ...

Notice that there now is a whole ``SandPile:`` key. This is the part of the configuration that is available to the ``SandPile`` model. The model will have access only to parameters below this key.
By adding additional keys to it, the default configuration is updated recursively at each indentation level.

So far, so good. But what are the model's default parameters? Each models
default configuration is included in its documentation. For example, in the
documentation of the ``SandPile`` model you will find a section with the
default parameters; it looks like this:

.. literalinclude:: ../../src/utopia/models/SandPile/SandPile_cfg.yml
   :language: yaml
   :start-after: ---

.. note::

  You can also locate the default model configuration of the ``SandPile``
  model at ``src/utopia/models/SandPile/SandPile_cfg.yml``. This file really is only
  intended as a *reference*; to actually change parameters, it's best to use the ``run.yml`` file. That way, you can always fall back on default values.

Let's change the grid resolution to a more interesting value. In your
``run.yml``, add the following entry to the ``SandPile`` entry:

.. code-block:: yaml

  cell_manager:
    grid:
      resolution: 128

Make sure to indent it! As is clear from the key name, this changes the grid resolution in the so-called cell manager to 128 cells per unit length of the physical space.

Run the model again and look at the resulting plots. What happened?

The resolution encodes the number of grid cells per unit length in the space.
You could also think about changing the extent of the space, which by default is the unit square.
The run configuration would then be

.. code-block:: yaml

  space:
    extent: [2., 1.]

  cell_manager:
    grid:
      resolution: 128

This would result in 256 cells in x and 128 cells in y direction.

By the way: what you just learned also applies to all other models.
You only need to know the model specific parameters, which you can find in the model documentation.
So, just check out another model and change its parameters as you like. 😎

.. note::

  **Changing the model configurations:** Technically, it is possible to change the model parameters in the file where the defaults are specified.
  However, this is **not** advisable at all! As the name says, these files are to carry the *default* parameters and are not expected to change.
  Instead, write your own run configuration files, as described in this section.
  This ensures among other things that all models always work with their default configuration and that tests are guaranteed to run quickly and pass.
  Basically, you prevent the universe from collapsing.

.. warning::

  **YAML indentation level:** In Utopia, nearly every option can be set through a configuration parameter.
  With these, it is important to take care of the correct indentation level.
  If you place a parameter at the wrong location, it will often be ignored, sometimes even without warning! A common mistake at the beginning is to place model specific parameters outside of the ``<model_name>`` scope (see text).

.. warning::
  Take care to choose model parameters wisely:

  1. Parameters such as ``cell_manager.grid.resolution`` can lead to a dramatically increased computation time.
  2. Some parameters have requirements which can also depend on other parameters. If this is the case, you normally find a comment above the corresponding parameters.

  It is prudent to first run a simulation at a minimal (small) configuration to see if everything works as desired.

.. note::

  **User configuration:** It is possible to create a so-called *user configuration file*. This file contains all settings that are user- or machine-specific, such as on how many cores to run a simulation, or where to store the output data.
  See how to create a user configuration by typing ``utopia config --help`` in your terminal.
  For more information, have a look at the frontend :ref:`FAQ <faq_frontend>`.


Plotting
--------
Utopia aims to make it easy to couple the simulation of a model with its evaluation. To that end, the Utopia frontend provides a plotting framework that loads the generated simulation data and can provide it to plotting functions, which then takes care of the data evaluation.

There are multiple ways in which plots can be generated:

* Each model can implement model-specific plot functions
* General plotting functions are available (to avoid recreating code over and over)
* External Python plotting scripts can be specified

Like many other parts of Utopia, this relies on a YAML-based configuration interface in which the plotting function to be used is specified and the parameters can be passed.

First, let's look at how a custom configuration can be used to adjust the behavior of existing model plots. Let's assume that – using the above steps – you have arrived at a run configuration you are happy with and you now want to run a simulation and afterwards create some plots from it.


Creating the simulation data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To not re-run simulations all the time (you would and could not do that after a very long simulation), let us first create some simulation data and then focus only on evaluating it:

.. code-block:: console

  $ utopia run SandPile ~/utopia_cfgs/SandPile/test/run.yml --no-plot

The ``--no-plot`` option results in the plots being skipped. You can now invoke the evaluation separately:

.. code-block:: console

  $ utopia eval SandPile

This will load the data of the *most recent* simulation run and perform the default plots.
You will see that a new folder has been created in the ``eval`` folder of the most recently run ``SandPile`` simulation. The evaluation results are placed in a new subfolder with the timestamp of the ``utopia eval`` invocation.

.. note::

  If you want to do the same with other simulation output, you have to specify either a path to the run directory (can be absolute or relative) or its timestamp; ``utopia eval`` will do its best to find the desired directory.
  Check the log output if the correct directory was identified and, as always, see ``utopia eval --help`` for... well: help.

Customizing default plots
^^^^^^^^^^^^^^^^^^^^^^^^^
Let us now customize the default plots.

Above, you used the ``~/utopia_cfgs/SandPile/test/run.yml`` file to configure the *simulation*. For *plots*, you can do just the same: Create a new file named ``plots.yml`` right beside the already existing configuration file.

Now, what will have to go into that file in order to manipulate the existing plot configuration?

Plots go by a name. To find out the names of the configured plots, let's first have a look at the terminal output: In the log messages you will see output from the ``plot_mngr`` module informing you about which plots were performed:

.. code-block:: console

  ...
  INFO     utopia      Plotting...
  INFO     plot_mngr   Performing plots from 5 entries ...
  INFO     plot_mngr   Performing 'mean_slope' plot ...
  INFO     plot_mngr   Finished 'mean_slope' plot.
  INFO     plot_mngr   Performing 'area_fraction' plot ...
  INFO     plot_mngr   Finished 'area_fraction' plot.
  INFO     plot_mngr   Performing 'cluster_size_distribution' plot ...
  INFO     plot_mngr   Finished 'cluster_size_distribution' plot.
  INFO     plot_mngr   Performing 'compl_cum_prob_dist' plot ...
  INFO     plot_mngr   Finished 'compl_cum_prob_dist' plot.
  INFO     plot_mngr   Performing 'slope_and_avalanche_anim' plot ...
  INFO     ca          Plotting animation with 4 frames of 2 properties each ...
  INFO     ca          Animation finished.
  INFO     plot_mngr   Finished 'slope_and_avalanche_anim' plot.
  INFO     plot_mngr   Successfully performed plots for 5 configuration(s).
  INFO     utopia      Plotting finished.
  ...

As you see there, multiple plots are configured.
To find out more about the default plot configuration, locate the corresponding section in the ``SandPile`` :doc:`model documentation <../models/SandPile>`.

There, you will find the same names as extracted from the log as keys on the root level of the configuration file. It looks something like this:

.. literalinclude:: ../../src/utopia/models/SandPile/SandPile_plots.yml
   :language: yaml
   :start-after: ---
   :end-before: # --- An animation

You will notice that the default configuration does not contain an awful lot of parameters. This is because it employs so-called "configuration inheritance" using the ``based_on`` key.
It imports an existing so-called "base plot configuration" and uses the additionally specified keys to update it recursively. This avoids copy-pasting configurations and allows to combine configurations using multiple inheritance.
For more information, you can continue reading :ref:`in the frontend plotting section <plot_cfg_inheritance>`.

Let's just adjust the ``mean_slope`` plot function.
To that end, add the following to your ``plots.yml`` file:

.. code-block:: yaml

  my_mean_slope_plot:
    based_on: mean_slope

This creates a new plot named ``my_mean_slope_plot``, which in this form produces the same result as the regular ``mean_slope`` plot:

.. code-block:: console

  $ utopia eval SandPile --plots-cfg ~/utopia_cfgs/SandPile/test/plots.yml

Confirm in the logs that only the ``my_mean_slope_plot`` plot was created.
Now check out the run directory, where a new directory inside ``eval`` (with the current timestamp) will hold the plot output.

Feel free to customize the plot configuration by changing parameters in your ``plots.yml`` file. To find out which configuration parameters are available, check out the base plot configuration for the model.

.. note::

    To avoid loading the data into memory upon each invocation of ``utopia eval``, you can use the ``--interactive`` option to enter into interactive plotting mode.
    There, you can change the arguments given to ``utopia eval`` and repeatedly create plots without discarding the data from memory.

As an example, let's now adjust the configuration a bit:

.. code-block:: yaml

  my_mean_slope_plot:
    based_on: mean_slope

    # Adjust linestyle and colour
    linestyle: dashed
    marker: '*'
    color: hotpink

Enjoy the plot! :)

.. note::

  You can run the CLI in debug mode, which will produce tracebacks and help you understand what's going on:

  .. code-block:: console

    utopia eval <model_name> --debug --plots-cfg <path/to/plots.yml>

  This is very useful when you run into errors in the plot functions, as the program then stops and gives you more information on what went wrong.

.. warning::

  As with the default model configuration, the default plot configuration is best left untouched. **To modify it, you should always pass a new plot configuration.**
  Note that, currently, the configuration you are passing to the CLI is not updating the existing default plots.

As you see, you can change *some* of the parameters of the plots; but only the ones the person who implemented the plot function chose to expose. Further along this tutorial, you will see how you can define your own plotting functions.


Animations
^^^^^^^^^^
At one point you might be interested in making cool animations of the state variables, but you might be deterred as it is generally hard to do. Not with Utopia\ !
In fact, it is part of the default plotting system. You might have noticed the corresponding plot configuration already. It looks something like this:

.. literalinclude:: ../../src/utopia/models/SandPile/SandPile_plots.yml
   :language: yaml
   :start-after: # --- An animation of the CA `slope`
   :end-before: # --- An animation of the CA `avalanche`

.. note::

    To create animations, you need to have ``ffmpeg`` installed.
    For more information, have a look at the recommended dependencies section in the :doc:`README <../README>`.


Quite a few more parameters here. Let's try and understand the most important ones:

* ``enabled: false`` is used here to disable the plot by default
* ``model_name: SandPile`` tells the plot function to use the data of that model
* ``to_plot`` allows specifying which properties to plot. You can also add more properties here and it will access the data depending on the name of the property.

In this case, the plot configuration was not specified as a base plot, thus you need to *copy* the configuration into your ``plots.yml``.
First thing to change would be to remove the ``enabled: false`` entry.
Run ``utopia eval`` with this plot configuration now and see what happens.

You can try the following things to get to know the capabilities of the ``.ca.state`` plotting function provided by utopya:

* Change the ``cmap``
* Add a ``limits`` argument to the property
* Try to add another property. To know which name to use, check out the printed data tree in the terminal log. (Be careful with indentation levels)

Now that your animation is configured, you might want to run a simulation with a larger grid and more time steps. Go for it! :)

.. warning::

  Before you launch some cool million-step simulation on a 1024 x 1024 grid, remember that it all needs to be stored somewhere and this might either flood your RAM or your hard drive / SSD ... or both.

.. note::

  If you *just* want to enable a disabled default plot and not change anything in the plot configuration, the CLI is here to help:
  ``utopia eval <model_name> --plot-only <plot_name1> <plot_name2> ...``.

.. note::

    If you see unexpected artifacts in the animation, try choosing a higher pixel resolution.
    See :ref:`plot_animations` for more information.



Using your own plot functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
You might want to have a bit more flexibility with what you plot. For example, you might be interested in plotting the standard deviation for the level of the cells for each time step.
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
          dm: The DataManager object that contains all loaded data
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
      """A plot for the standard deviation of the state.

      Args:
          dm:       The DataManager object that contains all loaded data
          out_path: The generated path at which this plot should be saved
          uni:      Contains the data from the single selected universe
      """
      # Get the slope data
      slope = uni['data/SandPile/slope']

      # Calculate the standard deviation over the spatial dimensions
      slope_std = slope.std(['x', 'y'])

      # Plot it over time
      plt.plot(slope_std.coords['time'], slope_std)

      # Set the aesthetics
      plt.xlabel("Time [Iteration Steps]")
      plt.ylabel("Sand Slope Std. Dev.")

      # Save the figure to the specified output path
      plt.savefig(out_path)

.. note::

  The ``slope`` and ``slope_std`` in the example above both behave like ``xarray.DataArray`` objects. As you see, they allow to specify operations on labelled dimensions, which not only reduces errors but is also far easier to read.
  You can read up on it `in their documentation <http://xarray.pydata.org/en/stable/data-structures.html#dataarray>`_.

.. hint::

  The plotting framework is actually more advanced.
  For example, it provides a ``PlotHelper`` framework which makes plotting much easier by allowing to configure your plots' aesthetics via the configuration, i.e. without touching any code.
  For example, you can set the labels and limits of the plots conveniently via the configuration.

  Furthermore, there is a :ref:`data transformation framework <external_plot_creator_DAG_support>` that follows a similar approach: making it possible to select specific data and apply transformations to it before passing it to a generic plotting function.

  To read up on both, check out the :ref:`plotting documentation <eval_plotting>`.


Parameter Sweeps and the ``Multiverse``
---------------------------------------
Alright. With the above, you already came into contact with a lot of Utopia's features. Well done for making it this far!
In this section, you will learn how to perform multiple simulations for different sets of parameters and how to plot them.

Furthermore, this section will use a different model for the examples.
This is to show that what you learned above can be applied to other models in Utopia, not only the ``SandPile`` model.


The ``ForestFire`` Model
^^^^^^^^^^^^^^^^^^^^^^^^
The ``ForestFire`` model uses a cellular automaton to represent a forest, where each CA cell can either be ``empty`` or a ``tree``.
Each tree can ignite with a certain probability, which will lead to whole cluster of trees (i.e., all connected trees) burning down.
You can find out more in the :ref:`model documentation <model_ForestFire>`.

Let's dive right in and have a short test run of this model:

.. code-block:: console

  $ utopia run ForestFire

Just as for the ``SandPile`` model, it will create and run a simulation with 4 time steps.

.. note::

  Make sure you have built the ``ForestFire`` binary before trying to run it:
  
  .. code-block:: console

    $ cd build
    $ make ForestFire
  


Parameters
""""""""""
To see the parameters available to the ``ForestFire`` model, let's have a look at that model's default configuration by opening its description in the Utopia documentation.
It looks like this:

.. literalinclude:: ../../src/utopia/models/ForestFire/ForestFire_cfg.yml
   :language: yaml
   :start-after: ---

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

    ForestFire:
      # Below, you can make updates to these values. Only add the values you
      # want to _change_ from the defaults.

      # Set the initial tree density, value in [0, 1]
      cell_manager:
        cell_params:
          p_tree: 0.0

You will surely see similarities to the run configuration used in the ``SandPile`` model. Again, the model-independent parameters are on the top level inside the ``parameter_space``: ``num_steps`` and ``seed`` (and others that we are not overwriting here).
As above, the model-specific default parameters are already imported into the ``ForestFire`` mapping.

Now, pass the configuration to the CLI:

.. code-block:: console

  $ utopia run ForestFire ~/utopia_cfgs/ForestFire/test/run.yml

Compare the output with that with non-zero initial density. What happens when you turn on percolation mode? Feel free to play around. :)


Parameter sweeps
^^^^^^^^^^^^^^^^
Often times when analyzing a model, it becomes necessary to compare the behaviour of the model for different sets of parameters. For example, in the case of the ``ForestFire`` model, one would want to extract the effect of the ``p_lightning`` parameter on the cluster size, and run different simulations for different values of these parameters to achieve that.

Another use case for running multiple simulations is that you might want to generate some statistics by averaging over multiple simulation runs. To that end, one would change the ``seed`` parameter that is used to initialize the random number generator; by choosing a different seed, the sequence of random numbers in the probabilistic functions is changed.

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

This now says that instead of using the default value for a single simulation, three simulations for the specified ``seed`` values are to be made. Let's see if it works:

.. code-block:: console

  $ utopia run ForestFire ~/utopia_cfgs/ForestFire/test/run.yml --sweep

.. note::

  Do not forget the ``--sweep`` flag! This is required to tell Utopia that you want to run a parameter sweep. Alternatively, you can add a new entry ``perform_sweep: true`` to the *root level* of the configuration file, i.e. on the same level as the ``parameter_space`` key, with zero indentation.

You will see some log output from the ``multiverse``, stating that it is

.. code-block:: console

  Adding tasks for simulation of 3 universes ...

Perhaps now is the time to talk about the nomenclature:
In Utopia, a *multiverse* is a set of several *universes*, which are fully separated from each other: they can **not** interact in any way during the simulation.
This also means that each universe has a separate and *distinct* set of parameters.
Depending on the number of CPUs your machine has, they exist (i.e., are being simulated) in parallel. And that's about where the analogy ends. ;)

After this brief detour, have a look at the output again. You will see how it is different to the one where you only ran a single universe:

* You no longer see the direct simulation output, as this would flood the terminal.
* The progress bar now behaves differently.
* When loading the data, you see a larger data tree.

This already suggests that more data was written. You can confirm that by opening the output directory.

What about the plots? Check the ``eval`` directory of your latest run.
You will notice that the default plots were applied to each universe separately and are placed inside a folder; the file name now contains the coordinates of the point in parameter space. We will discuss how to create a *single* plot from multiverse data shortly.

Adding more parameter sweeps
""""""""""""""""""""""""""""
Adding parameter sweeps is super easy.
Basically, you only have to add the ``!sweep`` indicator behind the parameter and specify the values (take care of the indentation value). That's it.

All parameters within the ``parameter_space`` level allow this yaml tag. There are, of course, more ways to specify parameters than explicitly giving the ``values``.
Let's change the ``p_lightning`` parameter in the run configuration and use logarithmically spaced values:

.. code-block:: yaml

      p_lightning: !sweep
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

  If you sweep over multiple parameters, all possible parameter combinations will be used, i.e. the Cartesian product of each ``!sweep``-specified set of dimensions.
  With :math:`P_1 ... P_n` sweep definitions, you'll get an :math:`n`-dimensional parameter space with :math:`\Pi_{i=1}^n |P_i|` possible combinations.
  In other words: You'll quickly feel the curse of dimensionality.

.. note::

  When you have a look at the output folders (which are just the names of the universes, e.g. ``uni23``) you might notice that they do not start at zero and might have gaps in between them.
  No need to worry if a universe is missing: This is because each point in parameter space needs to be associated with an index, and this includes the default values for each parameter dimension.
  To be consistent, the zero index of each parameter dimension maps to the default. Thus, the sweep values begin at index 1 and result in the pattern of indices you see.

There are a bunch of other things to do with parameter sweeps, which go beyond the scope of this tutorial. (If you're keen to explore the features, you can have a look at the underlying `paramspace package <https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace>`_.)

As you see, parameter sweeps can be used to easily create huge amounts of data. And we all know: with (hopefully) great data comes great responsibility.
Thus, let's now focus on how the plotting framework can be used to handle the multidimensional data.



Multiverse plots
^^^^^^^^^^^^^^^^
Let us plot the mean states of the mean universe states and use the run configuration of the previous part.

Recall that in the ``SandPile`` model, you first created a plot configuration.
So, let's do it in the same way: create a file ``~/utopia_cfgs/ForestFire/test/plots.yml`` with the content:

.. code-block:: yaml

   # The plot.yml configuration file for a test simulation of the ForestFire model.
   ---
   ensemble_averaged_mean_state:
     # As you need the data of many universes, select the multiverse plot creator
     creator: multiverse

     # Select a hyperslab of the multidimensional data
     select:
       # Choose the path within each universe (see data tree in terminal output)
       field: data/ForestFire/kind
       # NOTE The "kind of a cell" is equivalent to its state: empty (0) or tree (1)

     # Select the plot function, just as for a universe plot
     module_file: ~/path/to/my/plot/function/file.py
     plot_func: ensemble_averaged_mean_state

The file is located in the same directory as the run configuration to indicate that they belong together.

The short description of what you told Utopia's frontend to do is:

1. Create a plot called ``ensemble_averaged_mean_state`` that should use all the multiverse data (``creator: multiverse``).
2. Before entering the plot function, ``select`` data:

    1. For each universe, access the ``field`` located under ``data/ForestFire/kind``. You can look up the path in the data tree that is printed out in the terminal. This dataset has the following dimensions: ``time``, ``x``, and ``y``; remember that you have two-dimensional grid data for each time step.
    2. Align all these data using the parameter sweep dimensions into ...
    3. ... a higher-dimensional data structure, here: an `xarray <http://xarray.pydata.org/en/stable/>`_ ``Dataset`` object. This will obviously have another additional dimension: the ``seed`` you are sweeping over. If you have more sweep dimensions, they will also appear in the ``Dataset``. The data variable ``kind`` stored in that ``xarray.Dataset`` will thus have four dimensions.

3. Pass this multidimensional data to the plot function as ``mv_data`` argument.

.. note::

  For more information on how to select data from a multiverse, take a look :ref:`here <select_mv_data>`.

We now roughly understand the first part of the configuration file.
However, plotting won't quite work yet.
The second part of the configuration file says: sse the plotting function ``plot_func: ensemble_averaged_mean_state``, which is located in the ``module_file: ~/path/to/my/plot/function/file.py`` and create the plot.
But this function does not yet exist. Let's create it.

Create a file at a location of your choice, for instance: ``~/utopia_FFM_timeseries_plots.py``.
(You would probably want to create it at another location
– get inspired by the directory structure described earlier in this tutorial.)
Within this file, we will write the ``ensemble_averaged_mean_state`` function:

.. code-block:: python

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
      """Plots the ensemble averaged mean state over multiple universes"""
      # Calculate the mean state averaged over all universes.
      # The mean is calculated over the dimensions: 'x', 'y', and 'seed'
      mean_state = mv_data['kind'].mean(['x', 'y', 'seed'])

      # Plot it
      plt.plot(mean_state, **plot_kwargs)

      # Save and close the figure
      plt.savefig(out_path)
      plt.close()

Lastly, we still need to adapt the plot configuration from above, because the path to the python file (module) containing the plotting function is not yet correctly set.
So, adapt the parameter:

.. code-block:: yaml

    module_file: ~/utopia_FFM_timeseries_plots.py  # Choose the path, where you created the plot function!

Now everything is ready and set such that the multiverse data you have created in the previous run can be evaluated.
To do this, type the following command into your terminal:

.. code-block:: console

  $ utopia eval ForestFire --plots-cfg ~/utopia_cfgs/ForestFire/test/plots.yml

Of course, if you want to do a new simulation run that creates new data you can also use the command that runs and afterwards directly evaluates the data:

.. code-block:: console

  $ utopia run ForestFire ~/utopia_cfgs/ForestFire/test/run.yml --sweep --plots-cfg ~/utopia_cfgs/ForestFire/test/plots.yml

Go and check the resulting plot.What does it look like?

To learn more about parameter sweeps, take a look at :ref:`run_parameter_sweeps`.

.. note::

  In general, if you want your plot to be integrated into the Utopia model-specific plots, you can add the plot function to a suitable file within the directory ``utopia/python/model_plots/ForestFire``.
  However, this should only be done if the plot actually makes sense to have.
  For the ``ensemble_averaged_mean_state`` this probably is not the case.

  You could be wondering why this plot is not already among the default plots.
  The answer is that doing multiple realizations of the
  ForestFire model with just different random number seeds and averaging them makes little sense, because this system is ergodic.


Closing Remarks
---------------
What did you learn?
^^^^^^^^^^^^^^^^^^^
You learned the basics of:

- how to run any implemented model in Utopia,
- how Utopia is structured,
- the fundamental aspects of what Utopia is capable of doing (but this was really just the tip of the iceberg),
- how to plot the generated data conveniently, and
- how to perform and plot parameter sweeps.

In short, you learned how to use tools and concepts that can be applied to all other models in Utopia.


How to continue from here?
^^^^^^^^^^^^^^^^^^^^^^^^^^
The skills you learned in this tutorial are general enough that you can apply them to any model, following the philosophy: if you know how to cook pasta, you also know how to cook rice.
Of course, some adjustments will always need to be made.

A good idea is to just play around with different models and explore the world of chaotic, complex, and evolving systems. 🗺

Perhaps you may even want to write your own Utopia model. To that end you should work with the features provided by GitLab.
If you have never heard of GitLab or Github, or worked with git-based version control, and you don't know what a ``master`` or a *branch* is, and even less what is meant by *committing* or *merging*, continue with the :ref:`dev_workflow` guide.

After mastering that guide, acutally implementing your own model is described in the :ref:`impl_step_by_step`.


What if I have more questions?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Look if you can find some useful information ...

- ... elsewhere in :ref:`this documentation <welcome>` (try the search function),
- ... questions and answers in the :ref:`backend FAQ <faq_core>` or :ref:`frontend FAQ <faq_frontend>`, or
- ... information in the :ref:`cpp_docs`.
