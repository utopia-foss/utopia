.. _impl_nested:

Nesting Models – the Post-Model Era
===================================

.. note::

    This is an advanced feature.
    Only couple models once each of them has been tested individually.

The coupling of two or more models is explicitly allowed in Utopia, which provides the functionality to couple even complicated model hierarchies.
Every model is placed one level below its parent or super-model (with the ``PseudoParent`` at the top).
Hence, the child or sub-model is a member of the super-model and the configuration is passed through the super-model following their hierarchy.


Structural decisions for model nesting
---------------------------------------

Operating coupled models usually requires a couple of additional thoughts:

* The super-model has to ``iterate`` the sub-model as per your design;
  this implies that there are no constraints on how time maps between the different models, indeed the super-model has full control of the relative time scales;
  e.g. time can evolve in parallel, faster or slower, or even not synchronous at all.
* For an independent model, the ``run()`` command includes several operations, that now have to be manually organized by the super-model.
  Below you can find the most important ones; depending on the design not all of them need to be implemented, but should at least be considered carefully:

The ``prolog``
^^^^^^^^^^^^^^
A function that is to be called before the first iteration of this model.
Its default function includes writing the initial state.


The ``epilog``
^^^^^^^^^^^^^^
A function that is called after the last iteration of this model.
Ideally it should be called directly after the last iteration, though this is not a requirement.
Check the model's documentation.


The ``iterate`` method
^^^^^^^^^^^^^^^^^^^^^^
This is where the model actually progresses.
The timing when the individual model is iterated is organised by the parent.
The iteration of a sub-model could be alternating to that of the super-model with synchronously progressing time, but likewise you could consider iterating a sub-model more often, increasing temporal resolution, or iterating the sub-model until a condition is fulfilled, e.g. a steady state reached.
Also you could consider only iterating the sub-model up to a stop-condition in the parent's prolog to generate an initial state.
In any case, the super-model should keep track of time in its sub-models, if not trivial.
The configuration of model parameters and data writing remain accessible through the configuration for every involved model at the respective hierarchical level.

.. note::

  Do not use the ``run`` command of any sub-model, as it performs the global ``num_steps`` iterations (including ``prolog`` and ``epilog``) inherited throughout the hierarchy, rather than a model-level number of steps.
  The ``time_max`` of a sub-model can be exceeded by iteration.
  
  
Handling interrupts (optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The model may receive a signal to stop iteration, e.g. due to a break condition or the user interrupting the simulation run.
Upon that signal, the ``stop_now`` flag is set to ``true``, indicating that the iteration should stop and the model should shut down.
A grace period (default: 2s, configurable via frontend) is given; after that, the model process is killed, which may lead to loss of data.
If – for special reasons – a system of coupled models needs to perform a specific task at the breakpoint, the flag may be queried using ``this->stop_now.load()``.
Be aware that time-intensive tasks should *not* be carried out after the breakpoint; the aim is to swiftly take down the model object.
Also note that this flag is not part of the public interface and may change unexpectedly.


Monitoring progress (optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If the runtime intensive tasks are moved away from the super-model, you might find that monitoring progress is less useful.
You can overwrite the progress using the ``set_time_entries(time, time_max)`` function of the monitor manager with an estimate of current ``time`` and ``time_max`` dependent on the particular model design in the highest level model.

.. warning::

  In most cases, monitoring progress will be non-trivial and the proposed method does not provide a generic tool for runtime estimation.
  The function is not tested for the above purpose and might change without notice!
  
  
Conditional data writing (optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As the temporal progress of the models may become asynchronous you might want to write data explicitly at the times of interaction; to do so check out the triggers and deciders for writing data upon a condition that come with the :ref:`data manager <datamanager>`.


.. hint::

  For an example, see the :ref:`Environment model <model_Environment>`, which is intended to be used as a child-model and includes a guide on how to use it.



Dynamically spawning submodels
------------------------------
In some situations, it might be desired to dynamically create instances of submodels.
As an example use case, one might want to couple multiple existing CA-based models using a network structure, where each network node is an instance of the CA model.
The super-model then takes care of the interaction between these models.

For such a scenario, one needs to be able to dynamically create submodel instances.
To do so, the following points need to be taken into account:

* The submodel instances all need their own *unique* ``name``
* By default, the submodels will extract their own configuration from the super-model's configuration *using their own name*; in this dynamic case, such a configuration entry will probably not exist.
  Thus, the ``custom_cfg`` argument of the model constructor *needs* to be used when instantiating the submodel instances.
  The super-model is thus free to pass a configuration to the submodel.
