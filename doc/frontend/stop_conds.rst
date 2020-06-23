.. _stop_conds:

Stop Conditions
===============
Depending on the model under investigation, the required number of iteration steps may widely vary.
Let's say we are in a scenario where we are investigating convergence behaviour of a model; depending on the parameter configuration, the time to reach convergence can span many orders of magnitude.

In order to save computational resources, it would be useful to **dynamically stop simulations**, depending on their current state.

The **stop conditions** feature implements exactly that:
Depending on a timeout or on information conveyed to the frontend using the ``monitor`` method, running simulations can be told to stop.

.. contents::
   :local:
   :depth: 2

----

Using stop conditions
---------------------

Configuring a simulation run
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To configure such stop conditions, add an entry like the following to your :ref:`run configuration <feature_meta_config>`:

.. literalinclude:: ../../python/utopya/test/cfg/stop_conds.yml
    :language: yaml
    :start-after: ---

This example showcases three registered stop conditions.
The first two implement the exact same condition, a timeout.
They do so once in short and once in long syntax. (The short syntax is useful if ``to_check`` contains only a single function).

The third condition checks against the current model state, as conveyed to the frontend via its :ref:`monitor method <stop_conds_monitor>`.

Currently, the following stop condition functions are available in the :py:mod:`~utopya.stopcond_funcs` module:

* :py:func:`~utopya.stopcond_funcs.timeout_wall` terminates a *single* universe if it took too much time (measured against the clock on the wall, not CPU time)
* :py:func:`~utopya.stopcond_funcs.check_monitor_entry` reads the latest monitor entries and compares it to some ``value`` using an ``operator``.

.. note::

    If you are missing a stop condition function, please let us know by opening an issue or a merge request `via the project page <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia>`_.

.. hint::

    To check if a monitor entry is in an interval, pass multiple entries to the ``to_check`` argument; these are AND-connected.
    Example:

    .. code-block:: yaml

        run_kwargs:
          stop_conditions:
            # Will stop if: 41.9 <= {monitor entry} <= 42.1
            - !stop-condition
              name: fixation
              description: terminates once reaching the known fixpoint at 42
              to_check:
                - &check_density
                  func: check_monitor_entry
                  entry_name: MyModel.density
                  operator: '>='
                  value: 41.9
                - <<: *check_density
                  operator: '<='
                  value: 42.1

.. note::

    Of course, stopping a simulation *can* also be implemented directly on C++ side.
    However, with the stop conditions feature you will have more flexibility and do not need to implement additional logic into your model;
    you only need to provide the monitor entries and set the configuration entries.


During the simulation run
^^^^^^^^^^^^^^^^^^^^^^^^^
Let's say that we choose the following run configuration:

.. literalinclude:: ../../python/utopya/test/cfg/stop_conds_integration.yml
    :language: yaml
    :start-after: ---
    :end-before: worker_manager

The log output will then show that it detected both the timeout and the stop conditions:

.. code-block:: plain-text

    PROGRESS multiverse      Adding tasks for simulation of 13 universes ...
    INFO     multiverse      Added 13 tasks.
    PROGRESS workermanager   Preparing to work ...
    NOTE     workermanager     Timeout:         13:37:42 (in 1m 40s)
    NOTE     workermanager     Stop conditions: timeout_wall, high state
    HILIGHT  workermanager   Starting to work ...

During the rest of the simulation, everything is as always.
If the stop conditions kick in, the run should be finished more quickly, though.

.. note::

    There will be *no* extra log output if a universe was stopped.
    Also, the progress bar will show the status of the run in the same was as without stop conditions: it does not distinguish universes that *finished due to a stop condition* from those that finished regularly after ``num_steps``.

.. hint::

    If the stop conditions are **not** detected by the :py:class:`~utopya.workermanager.WorkerManager`,  make sure that the ``run_kwargs`` end up in the :ref:`meta configuration <feature_meta_config>`.
    You can do so by inspecting the ``config`` directory in the output folder, where all configuration parts are backed-up separately.


After the simulation run
^^^^^^^^^^^^^^^^^^^^^^^^
To find out which universes were stopped *due to a stop condition*, inspect the ``_report.txt`` file in the output directory.
It will contain information on which stop conditions were fulfilled for which universes.

In the example shown above (using the ``dummy`` model and a purely illustrational set of stop conditions) all simulations end up being stopped due to the ``high state`` stop condition rather than the wall timeout.

.. code-block:: plain-text

    Stop Conditions
    ---------------

      13 / 13 universes were stopped due to at least one of the following stop conditions:

      StopCondition 'timeout_wall'
          (None)

      StopCondition 'high state': stops simulation when state gets too high
          uni01, uni02, uni03, uni04, uni05, uni06, uni07, uni08, uni09, uni10, uni11, uni12, uni13

.. hint::

    The ``_report.txt`` file is already created *during* the run and updated when individual simulations end.
    If you are interested in that information before the end of the simulation run, you can also inspect it during the run.


.. _stop_conds_monitor:

Conveying information from your model to the frontend
-----------------------------------------------------
As mentioned above, the ``check_monitor_entry`` function can make use of the values of a monitor entry and use those to stop a simulation.

For an implementation example, let's look at the :doc:`ForestFire`\ 's ``monitor`` method:

.. literalinclude:: ../../src/utopia/models/ForestFire/ForestFire.hh
    :language: c++
    :start-after: /// Provide monitoring information
    :end-before: /// Write data
    :dedent: 4

Further information on how to set monitor entries can be found `in the doxygen documentation <../../doxygen/html/class_utopia_1_1_data_i_o_1_1_monitor.html>`_.

.. note::

    The ``monitor`` method is only invoked after certain *time* intervals, independent of the number of iteration steps.
    As these are invoked quite seldom, it usually is no big issue if you perform more costly operations within ``monitor``.

    To control how frequent the monitor data is emitted and thus becomes available to the frontend, set the ``parameter_space.monitor_emit_inveral`` entry.

    We suggest to **not** choose values that are much smaller than a second.
    This is because *all* monitor data is retained by the frontend, thus potentially eating up your memory if the monitor is emitting too frequently or the simulation time is very long.


Details
-------
What exactly happens once a stop condition is fulfilled?

If the frontend detects that a stop condition for a specific task (i.e., a universe) is fulfilled, the following procedure takes place:

1. The frontend sends the ``SIGUSR1`` signal to the running simulation
2. The ``Model`` base class catches that signal and sets a flag, denoting that the simulation should stop.
3. The model will finish its current iteration step (i.e., the ``iterate`` call).
4. The model will then quit with a non-zero exit code
5. The corresponding code is caught by the frontend, allowing to detect that the simulation ended due to a stop condition. Unlike for regular non-zero exit codes, no verbose log messages are printed, nor are exceptions being raised.
6. Some tracking variables are updated to reflect this information.
7. At the end of the simulation, the ``_report.txt`` shows information on which universe was stopped due to which stop condition.
