"""The WorkerManager class."""

import os
import sys
import queue
import warnings
import logging
import copy
import time
from datetime import datetime as dt
from typing import Union, Callable, Sequence, List, Set, Dict

from .task import WorkerTask, TaskList, SIGMAP
from .stopcond import StopCondition, SIG_STOPCOND
from .reporter import WorkerManagerReporter
from .tools import format_time

# Initialise logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class WorkerManager:
    """The WorkerManager class manages WorkerTasks.

    Attributes:
        nonzero_exit_handling (str): Stores the WorkerManager's behavior upon
            a worker exiting with a non-zero exit code. For 'ignore', nothing
            happens. For 'warn', a warning is printed. For 'raise', the log is
            shown and the WorkerManager exits with the same exit code as the
            WorkerTask exited with.
        num_workers (int): The number of parallel workers
        pending_exceptions (queue.Queue): A (FiFo) queue of Exception objects
            that will be handled by the WorkerManager during working. This is
            the interface that allows for other threads that have access to
            the WorkerManager to add an exception and let it be handled in the
            main thread.
        poll_delay (float): The delay (in s) after a poll
        reporter (WorkerManagerReporter): The associated reporter.
        rf_spec (dict): The report format specifications that are used
            throughout the WorkerManager. These are invoked at different points
            of the operation of the WorkerManager: while_working, after_work,
            after_abort, task_spawn, task_finished
        times (dict): Holds profiling information for the WorkerManager
    """

    def __init__(self,
                 num_workers: Union[int, str]='auto',
                 poll_delay: float=0.05,
                 lines_per_poll: int=20,
                 periodic_task_callback: int=None,
                 QueueCls=queue.Queue,
                 reporter: WorkerManagerReporter=None,
                 rf_spec: Dict[str, Union[str, List[str]]]=None,
                 save_streams_on: Sequence[str]=(),
                 nonzero_exit_handling: str='ignore',
                 interrupt_params: dict=None,
                 cluster_mode: bool=False,
                 resolved_cluster_params: dict=None):
        """Initialize the worker manager.

        Args:
            num_workers (Union[int, str], optional): The number of workers
                that can work in parallel. If 'auto' (default), uses
                os.cpu_count(). If below zero, deduces abs(num_workers) from
                the CPU count.
            poll_delay (float, optional): How long (in seconds) the delay
                between worker polls should be. For too small delays (<0.01),
                the CPU load will become significant.
            lines_per_poll (int, optional): How many lines to read from
                each stream during polling of the tasks. This value should not
                be too large, otherwise the polling is delayed by too much.
                By setting it to -1, all available lines are read.
            periodic_task_callback (int, optional): If given, an additional
                task callback will be invoked after every
                ``periodic_task_callback`` poll events.
            QueueCls (Class, optional): Which class to use for the Queue.
                Defaults to FiFo.
            reporter (WorkerManagerReporter, optional): The reporter associated
                with this WorkerManager, reporting on the progress.
            rf_spec (Dict[str, Union[str, List[str]]], optional): The names of
                report formats that should be invoked at different points of
                the WorkerManager's operation.
                Possible keys:
                    ``before_working``, ``while_working``, ``after_work``,
                    ``after_abort``, ``task_spawn``, ``task_finished``.
                All other keys are ignored.
                The values of the dict can be either strings or lists of
                strings, where the strings always refer to report formats
                registered with the WorkerManagerReporter. This argument
                updates the default report format specifications.
            save_streams_on (Sequence[str], optional): On which events to
                invoke :py:meth:`~utopya.task.WorkerTask.save_streams`
                *during* work.
                Should be a sequence containing one or both of the keys
                ``on_monitor_update``, ``periodic_callback``.
            nonzero_exit_handling (str, optional): How to react if a WorkerTask
                exits with a non-zero exit code. For 'ignore', nothing happens.
                For 'warn', a warning is printed and the last 5 lines of the
                log are shown. For 'raise', the last 20 lines of the log is
                shown, all other tasks are terminated, and the WorkerManager
                exits with the same exit code as the WorkerTask exited with.
                Note that 'warn' will not lead to any messages if the worker
                died by SIGTERM, which presumable originated from a fulfilled
                stop condition. Use 'warn_all' to also receive warnings in
                this case.
            interrupt_params (dict, optional): Parameters that determine how
                the WorkerManager behaves when receiving KeyboardInterrupts
                during working.
                Possible keys:
                    'send_signal': Which signal to send to the workers. Can be
                        SIGINT (default), SIGTERM, SIGKILL, or any valid signal
                        as integer.
                    'grace_period': how long to wait for the other workers to
                        gracefully shut down. After this period (in seconds),
                        the workers will be killed via SIGKILL. Default is 5s
                    'exit': whether to sys.exit at the end of start_working.
                        Default is True.
            cluster_mode (bool, optional): Whether similar tasks to those that
                are managed by this WorkerManager are, at the same time, worked
                on by other WorkerManager. This is relevant because the output
                of files might be affected by whether another WorkerManager
                instance is currently working on the same output directory.
                Also, in the future, this argument might be used to communicate
                between nodes.
            resolved_cluster_params (dict, optional): The corresponding cluster
                parameters.

        Raises:
            ValueError: For too negative `num_workers` argument
        """
        log.progress("Initializing WorkerManager ...")

        # Initialize attributes, some of which are property-managed
        self._num_workers = None
        self._poll_delay = None
        self._periodic_callback = periodic_task_callback
        self._tasks = TaskList()
        self._task_q = QueueCls()
        self._active_tasks = []
        self.stopped_tasks = TaskList()
        self._stop_conditions = set()
        self._reporter = None
        self._num_finished_tasks = 0
        self._nonzero_exit_handling = None
        self.save_streams_on = save_streams_on
        self._suppress_rf_specs = []
        self.pending_exceptions = queue.Queue()

        # Hand over arguments
        self.poll_delay = poll_delay
        self.lines_per_poll = lines_per_poll
        self.nonzero_exit_handling = nonzero_exit_handling
        self._cluster_mode = cluster_mode
        self._resolved_cluster_params = resolved_cluster_params
        self.interrupt_params = (interrupt_params if interrupt_params else {})

        if num_workers == 'auto':
            self.num_workers = os.cpu_count()

        elif num_workers < 0:
            try:
                self.num_workers = os.cpu_count() + num_workers
            except ValueError as err:
                raise ValueError("Received invalid argument `num_workers` of "
                                 "value {}. If giving a negative value, note "
                                 "that it needs to sum up with the CPU count "
                                 "({}) to a positive integer."
                                 "".format(num_workers, os.cpu_count())
                                 ) from err

        else:
            self.num_workers = num_workers

        # Reporter-related, setting default rf_spec
        if reporter:
            self.reporter = reporter

        self.rf_spec = dict(before_working='while_working',
                            while_working='while_working',
                            task_spawned='while_working',
                            task_finished='while_working',
                            after_work='after_work',
                            after_abort='after_work')
        if rf_spec:
            self.rf_spec.update(rf_spec)

        # Provide some information
        log.note("  Number of available CPUs:  %d", os.cpu_count())
        log.note("  Number of workers:         %d", self.num_workers)
        log.note("  Non-zero exit handling:    %s", self.nonzero_exit_handling)

        # Store some profiling information
        self.times = dict(init=dt.now(), start_working=None,
                          timeout=None, end_working=None)
        # These are also accessed by the reporter

        log.progress("Initialized WorkerManager.")

    # Properties ..............................................................
    @property
    def tasks(self) -> TaskList:
        """The list of all tasks."""
        return self._tasks

    @property
    def task_queue(self) -> queue.Queue:
        """The task queue."""
        return self._task_q

    @property
    def task_count(self) -> int:
        """Returns the number of tasks that this manager *ever* took care of.
        Careful: This is NOT the current number of tasks in the queue!
        """
        return len(self.tasks)

    @property
    def num_workers(self) -> int:
        """The number of workers that can work in parallel."""
        return self._num_workers

    @num_workers.setter
    def num_workers(self, val):
        """Set the number of workers that can work in parallel."""
        if val <= 0 or not isinstance(val, int):
            raise ValueError("Need positive integer for number of workers, "
                             "got {} of value {}.".format(type(val), val))

        elif val > os.cpu_count():
            warnings.warn("Set WorkerManager to use more parallel workers ({})"
                          "than there are cpu cores ({}) on this "
                          "machine.".format(val, os.cpu_count()),
                          UserWarning)

        self._num_workers = val
        log.debug("Set number of workers to %d", self.num_workers)

    @property
    def active_tasks(self) -> List[WorkerTask]:
        """The list of currently active tasks.

        Note that this information might not be up-to-date; a process might
        quit just after the list has been updated.
        """
        return self._active_tasks

    @property
    def num_finished_tasks(self) -> int:
        """The number of finished tasks. Incremented whenever a task leaves
        the active_tasks list, regardless of its exit status.
        """
        return self._num_finished_tasks

    @property
    def num_free_workers(self) -> int:
        """Returns the number of free workers."""
        return self.num_workers - len(self.active_tasks)

    @property
    def poll_delay(self) -> float:
        """The poll frequency in polls/second. Strictly speaking: the sleep
        time between two polls, which roughly equals the poll frequency."""
        return self._poll_delay

    @poll_delay.setter
    def poll_delay(self, val) -> None:
        """Set the poll frequency to a positive value."""
        if val <= 0.:
            raise ValueError("Poll delay needs to be positive, was "+str(val))
        elif val < 0.01:
            warnings.warn("Setting a poll delay of {} < 0.01s can lead to "
                          "significant CPU load. Consider choosing a higher "
                          "value.", UserWarning)
        self._poll_delay = val

    @property
    def stop_conditions(self) -> Set[StopCondition]:
        """All stop conditions that were ever passed to
        :py:meth:`~utopya.workermanager.WorkerManager.start_working` during the
        life time of this WorkerManager.
        """
        return self._stop_conditions

    @property
    def nonzero_exit_handling(self) -> str:
        """The action upon non-zero WorkerTask exit code."""
        return self._nonzero_exit_handling

    @nonzero_exit_handling.setter
    def nonzero_exit_handling(self, val: str):
        """Set the nonzero_exit_handling attribute.

        Args:
            val (str): The value to set it to. Can be: ignore, warn, raise

        Raises:
            ValueError: For invalid value
        """
        allowed_vals = ['ignore', 'warn', 'warn_all', 'raise']
        if val not in allowed_vals:
            raise ValueError("`nonzero_exit_handling` needs to be one of {}, "
                             "but was '{}'.".format(allowed_vals, val))

        self._nonzero_exit_handling = val

    @property
    def reporter(self) -> Union[WorkerManagerReporter, None]:
        """Returns the associated Reporter object or None, if none is set."""
        return self._reporter

    @reporter.setter
    def reporter(self, reporter):
        """Set the Reporter object for this WorkerManager.

        This includes a check for the correct type and whether the reporter was
        already set.
        """
        if not isinstance(reporter, WorkerManagerReporter):
            raise TypeError("Need a WorkerManagerReporter for reporting from "
                            "WorkerManager, got {}.".format(type(reporter)))
        elif self.reporter:
            raise RuntimeError("Already set the reporter; cannot change it.")

        self._reporter = reporter

        log.debug("Set reporter of WorkerManager.")

    @property
    def cluster_mode(self) -> bool:
        """Returns whether the WorkerManager is in cluster mode"""
        return self._cluster_mode

    @property
    def resolved_cluster_params(self) -> dict:
        """Returns a copy of the cluster configuration with all parameters
        resolved. This makes some additional keys available on the top level.
        """
        # Return the cached value as a _copy_ to secure it against changes
        return copy.deepcopy(self._resolved_cluster_params)

    # Public API ..............................................................

    def add_task(self, *,
                 TaskCls: type=WorkerTask,
                 **task_kwargs) -> WorkerTask:
        """Adds a task to the WorkerManager.

        Args:
            TaskCls (type, optional): The WorkerTask-like type to use
            **task_kwargs: All arguments needed for WorkerTask initialization.
                See :py:class:`utopya.task.WorkerTask` for all valid arguments.

        Returns:
            WorkerTask: The created WorkerTask object
        """
        # Evaluate save_stream callbacks
        save_streams = lambda t: t.save_streams(final=False)

        if 'monitor_updated' in self.save_streams_on:
            save_streams_on_monitor_update = True

        if 'periodic_callback' in self.save_streams_on:
            periodic_callback = save_streams
        else:
            # Do nothing
            periodic_callback = lambda _: None


        # Define the function needed to calculate the task's progress
        def calc_progress(task) -> float:
            """Uses the task's stream objects to calculate the progress.

            If no stream objects are available, returns 0.
            """
            # Check if parsed objects were available
            if not task.outstream_objs:
                return 0.

            # Extract the `progress` key from the latest entry; if it is not
            # available, use the same behaviour as above and return 0
            return task.outstream_objs[-1].get('progress', 0.)

        # Prepare the callback functions needed by the reporter . . . . . . . .
        def task_spawned(task):
            """Performs action after a task was spawned.

            - checks if stream-forwarding was activated
            - invokes the 'task_spawned' report specification
            """
            # As the task might have been configured to forward streams, it
            # needs to be checked whether this would clash with the reporter's
            # output to stdout.
            # First, check if that was already taken care of
            if self.reporter and not self.reporter.suppress_cr:
                # Nope -> need to check if the task will forward streams
                if task.streams.get('out') and task.streams['out']['forward']:
                    # Yes -> need to suppress carriage returns by reporter and
                    # reduce report invokations by the WorkerManager main loop
                    self.reporter.suppress_cr = True
                    self._suppress_rf_specs.append('while_working')

            # Invoke the report
            self._invoke_report('task_spawned', force=True)

        def task_finished(task):
            """Performs actions after a task has finished.

            - invokes the 'task_finished' report specification
            - registers the task with the reporter, which extracts information
              on the run time of the task and its exit status
            - in debug mode, performs an action upon non-zero task exit status
            """
            if self.reporter is not None:
                self.reporter.register_task(task)

            self._invoke_report('task_finished', force=True)

            # If there was a (non-zero) exit and the handling mode is set
            # accordingly, generate an exception and add it to the list of
            # pending exceptions. Handle exit codes that result from a stop
            # condition being fulfilled separately.
            if self.nonzero_exit_handling != 'ignore' and task.worker_status:
                if task.worker_status == 128+abs(SIGMAP[SIG_STOPCOND]):
                    exc = WorkerTaskStopConditionFulfilled(task)
                else:
                    exc = WorkerTaskNonZeroExit(task)
                self.pending_exceptions.put_nowait(exc)

        def monitor_updated(task):
            """Performs actions when there was a parsed object in the task's
            stream, i.e. when the monitor got an update.
            """
            if save_streams_on_monitor_update:
                save_streams(task)
            self._invoke_report('monitor_updated')

        # Prepare the arguments for the WorkerTask . . . . . . . . . . . . . .

        callbacks = dict(
            # Invoked by task itself
            spawn=task_spawned,
            finished=task_finished,
            parsed_object_in_stream=monitor_updated,
            #
            # Invoked by WorkerManager
            periodic=periodic_callback,
        )

        # Generate the WorkerTask-like object from the given parameters
        task = TaskCls(callbacks=callbacks,
                       progress_func=calc_progress,
                       **task_kwargs)

        # Append it to the task list and put it into the task queue
        self.tasks.append(task)
        self.task_queue.put_nowait(task)

        log.debug("Task %s (uid: %s) added.", task.name, task.uid)
        return task

    def start_working(self, *, detach: bool=False, timeout: float=None,
                      stop_conditions: Sequence[StopCondition]=None,
                      post_poll_func: Callable=None) -> None:
        """Upon call, all enqueued tasks will be worked on sequentially.

        Args:
            detach (bool, optional): If False (default), the WorkerManager
                will block here, as it continuously polls the workers and
                distributes tasks.
            timeout (float, optional): If given, the number of seconds this
                work session is allowed to take. Workers will be aborted if
                the number is exceeded. Note that this is not measured in CPU
                time, but the host systems wall time.
            stop_conditions (Sequence[StopCondition], optional): During the
                run these StopCondition objects will be checked
            post_poll_func (Callable, optional): If given, this is called after
                all workers have been polled. It can be used to perform custom
                actions during a the polling loop.

        Raises:
            NotImplementedError: for `detach` True
            ValueError: For invalid (i.e., negative) timeout value
            WorkerManagerTotalTimeout: Upon a total timeout
        """
        self._invoke_report('before_working')

        log.progress("Preparing to work ...")

        # Determine timeout arguments
        if timeout:
            if timeout <= 0:
                raise ValueError("Invalid value for argument `timeout`: {} -- "
                                 "needs to be positive.".format(timeout))

            # Already calculate the time after which a timeout would be reached
            self.times['timeout'] = time.time() + timeout

        # Set the variable needed for checking; if above condition was not
        # fulfilled, this will be None
        timeout_time = self.times['timeout']

        # Determine whether to detach the whole working loop
        if detach:
            # TODO implement the content of this in a separate thread.
            raise NotImplementedError("It is currently not possible to "
                                      "detach the WorkerManager from the "
                                      "main thread.")

        # Set some variables needed during the run
        poll_no = 0
        self.times['start_working'] = dt.now()

        # Inform about timeout and stop conditions
        if timeout:
            _to_fstr = "%X" if timeout < 60*60*12 else "%X, %d.%m.%y"
            _to_at = dt.fromtimestamp(timeout_time).strftime(_to_fstr)
        log.note("  Timeout:         %s", "None" if not timeout
                 else f"{_to_at} (in {format_time(timeout)})")
        log.note("  Stop conditions: %s", "None" if not stop_conditions
                 else ", ".join([sc.name for sc in stop_conditions]))

        log.hilight("Starting to work ...")

        # Keep track of the stop condition objects
        if stop_conditions:
            self._stop_conditions.update(set(stop_conditions))

        # Start with the polling loop
        # Basically all working time will be spent in there ...
        try:
            while self.active_tasks or self.task_queue.qsize() > 0:
                # Check total timeout
                if timeout_time is not None and time.time() > timeout_time:
                    raise WorkerManagerTotalTimeout()

                # Check if there was another reason for exiting
                self._handle_pending_exceptions()

                # Check if there are free workers
                if self.num_free_workers:
                    # Yes. => Try to grab a task and start working on it
                    try:
                        new_task = self._grab_task()

                    except queue.Empty:
                        # There were no tasks left in the task queue
                        pass

                    else:
                        # Succeeded in grabbing a task; worker spawned
                        self.active_tasks.append(new_task)
                    # NOTE Only a single task is grabbed here, even if there is
                    # more than one free worker. This is to assure that the
                    # while loop iterations have comparable run time, even if
                    # a task is added (which can take O(ms)). As this loop
                    # handles more than just grabbing new tasks, it is safer
                    # to have it run reliably and foreseeably.
                    # Also, the poll delay is usually not so large that there
                    # would be an issue with workers staying idle for too long.

                # Do stream-related actions for each task
                for task in self.active_tasks:
                    # Read the streams and forward them (if the tasks were
                    # configured to do so)
                    task.read_streams(
                        forward_directly=True,
                        max_num_reads=self.lines_per_poll,
                    )
                    task.forward_streams()

                # Invoke a report
                self._invoke_report('while_working')

                # Check stop conditions
                if stop_conditions:
                    # Compile a list of workers where the stop conditions
                    # were fulfilled and store them.
                    fulfilled = self._check_stop_conds(stop_conditions)
                    self.stopped_tasks += fulfilled

                    # Now signal those workers such that they terminate.
                    self._signal_workers(fulfilled, signal=SIG_STOPCOND)

                # Poll the workers. (Will also remove no longer active workers)
                self._poll_workers()

                # Call the post-poll function
                if post_poll_func is not None:
                    log.debug("Calling post_poll_func %s ...",
                              post_poll_func.__name__)
                    post_poll_func()

                # Invoke periodic callback for all tasks
                if (
                    self._periodic_callback and
                    poll_no % self._periodic_callback == 0
                ):
                    self._invoke_periodic_callbacks()

                # Some information
                poll_no += 1
                log.debug("Poll # %6d:  %d active tasks",
                          poll_no, len(self.active_tasks))

                # Delay the next poll
                time.sleep(self.poll_delay)

            # Finished working
            # Handle any remaining pending exceptions
            self._handle_pending_exceptions()

        except (KeyboardInterrupt, WorkerManagerTotalTimeout) as exc:
            # Got interrupted or a total timeout.
            # Interrupt the workers, giving them some time to shut down ...

            # Suppress reporter to use CR; then inform via log messages
            if self.reporter:
                self.reporter.suppress_cr = True

            log.warning("Received %s.", type(exc).__name__)

            # Extract parameters from config
            # Which signal to send to workers
            signal = self.interrupt_params.get('send_signal', 'SIGINT')

            # The grace period within which the tasks have to shut down
            grace_period = self.interrupt_params.get('grace_period', 5.)

            # Send the signal
            log.info("Sending signal %s to %d active task(s) ...",
                     signal, len(self.active_tasks))
            self._signal_workers(self.active_tasks, signal=signal)

            # Continuously poll them for a certain grace period in order to
            # find out if they have shut down.
            log.warning("Allowing %s for %d task(s) to shut down ... "
                        "(Ctrl + C to kill them now.)",
                        format_time(grace_period, ms_precision=1),
                        len(self.active_tasks))
            grace_period_start = time.time()

            try:
                while True:
                    self._poll_workers()

                    # Check whether to leave the loop
                    if not self.active_tasks:
                        break
                    elif time.time() > grace_period_start + grace_period:
                        raise KeyboardInterrupt

                    # Need to continue. Delay polling ...
                    time.sleep(self.poll_delay)

            except KeyboardInterrupt:
                log.critical("Killing workers of %d tasks now ...",
                             len(self.active_tasks))
                self._signal_workers(self.active_tasks, signal='SIGKILL')

                # Wait briefly (killing shouldn't take long), then poll
                # one last time to update all task's status.
                time.sleep(0.5)
                self._poll_workers()

            # Store end time and invoke a report
            self.times['end_working'] = dt.now()
            self._invoke_report('after_abort', force=True)

            log.hilight("Work session ended.")

            if (
                type(exc) is KeyboardInterrupt
                and self.interrupt_params.get('exit', True)
            ):
                # Exit with appropriate exit code (128 + abs(signum))
                log.warning("Exiting after KeyboardInterrupt ...")
                sys.exit(128 + abs(SIGMAP[signal]))

            # Otherwise, just return control to the calling scope

        except WorkerManagerError as err:
            # Some error not related to the non-zero exit code occurred.
            # Gracefully terminate remaining tasks before re-raising the
            # exception

            # Suppress reporter to use CR; then inform via log messages
            if self.reporter:
                self.reporter.suppress_cr = True

            log.critical("Did not finish working! See above for error log.")

            # Now terminate the remaining active tasks
            log.hilight("Terminating active tasks ...")
            self._signal_workers(self.active_tasks, signal='SIGTERM')

            # Store end time and invoke a report
            self.times['end_working'] = dt.now()
            self._invoke_report('after_abort', force=True)

            # For some specific error types, do not raise but exit with status
            if isinstance(err, WorkerTaskNonZeroExit):
                log.critical("Exiting now ...")
                sys.exit(err.task.worker_status)

            # Some other error occurred; just raise
            log.critical("Re-raising error ...")
            raise

        # Register end time and invoke final report
        self.times['end_working'] = dt.now()
        self._invoke_report('after_work', force=True)

        print("")
        log.success("Finished working. Total tasks worked on: %d",
                    self.task_count)

    # Non-public API ..........................................................

    def _invoke_report(self, rf_spec_name: str, *args, **kwargs):
        """Helper function to invoke the reporter's report function"""
        if self.reporter is None:
            return

        # Check whether to suppress this rf_spec
        if self._suppress_rf_specs and rf_spec_name in self._suppress_rf_specs:
            # Do not report this one
            return

        # Resolve the spec name
        rfs = self.rf_spec[rf_spec_name]

        if not isinstance(rfs, list):
            rfs = [rfs]

        for rf in rfs:
            self.reporter.report(rf, *args, **kwargs)

    def _grab_task(self) -> WorkerTask:
        """Will initiate that a task is gotten from the queue and that it
        spawns its worker process.

        Returns:
            WorkerTask: The WorkerTask grabbed from the queue.

        Raises:
            queue.Empty: If the task queue was empty
        """

        # Get a task from the queue
        try:
            task = self.task_queue.get_nowait()

        except queue.Empty as err:
            raise queue.Empty("No more tasks available in tasks queue."
                              ) from err

        else:
            log.debug("Got task %s from queue. (Priority: %s)",
                      task.uid, task.priority)

        # Let it spawn its own worker
        task.spawn_worker()

        # Now return the task
        return task

    def _poll_workers(self) -> None:
        """Will poll all workers that are in the working list and remove them
        from that list if they are no longer alive.
        """
        # Poll the task's worker's status
        for task in self.active_tasks:
            if task.worker_status is not None:
                # This task has finished. Need to rebuild the list
                break
        else:
            # Nothing to rebuild
            return

        # Broke out of the loop, i.e.: at least one task finished
        old_len = len(self.active_tasks)

        # have to rebuild the list of active tasks now...
        self.active_tasks[:] = [t for t in self.active_tasks
                                if t.worker_status is None]
        # NOTE this will also poll all other active tasks and potentially not
        #      add them to the active_tasks list again.

        # Now, only active tasks are in the list, but the list is shorter
        # Can deduce the number of finished tasks from this
        self._num_finished_tasks += (old_len - len(self.active_tasks))

        return

    def _check_stop_conds(self, stop_conds: Sequence[StopCondition]
                          ) -> Set[WorkerTask]:
        """Checks the given stop conditions for the active tasks and compiles
        a list of tasks that needs to be terminated.

        Args:
            stop_conds (Sequence[StopCondition]): The stop conditions that
                are to be checked.

        Returns:
            List[WorkerTask]: The WorkerTasks whose workers need to be
                terminated
        """
        to_terminate = []
        log.debug("Checking %d stop condition(s) ...", len(stop_conds))

        for sc in stop_conds:
            log.debug("Checking stop condition '%s' ...", sc.name)

            # Compile the list of tasks that fulfil a stop condition
            fulfilled = [t for t in self.active_tasks
                         if (t not in self.stopped_tasks and sc.fulfilled(t))]

            if fulfilled:
                log.debug("Stop condition '%s' fulfilled for %d task(s):  %s",
                          sc.name, len(fulfilled),
                          ", ".join([t.name for t in fulfilled]))
                to_terminate += fulfilled

        # Return as set to be sure that they are unique
        return set(to_terminate)

    def _invoke_periodic_callbacks(self):
        """Invokes the ``periodic`` callback function of each active task."""
        for task in self.active_tasks:
            task._invoke_callback('periodic')

    def _signal_workers(self, tasks: Union[str, List[WorkerTask]],
                        *, signal: Union[str, int]) -> None:
        """Send signals to a list of WorkerTasks.

        Args:
            tasks (Union[str, List[WorkerTask]]): strings 'all' or 'active' or
                a list of WorkerTasks to signal
            signal (Union[str, int]): The signal to send
        """
        if isinstance(tasks, str):
            if tasks == 'all':
                tasks = self.tasks
            elif tasks == 'active':
                tasks = self.active_tasks
            else:
                raise ValueError("Tasks cannot be specified by string '{}', "
                                 "allowed strings are: 'all', 'active'."
                                 "".format(tasks))

        if not tasks:
            log.debug("No worker tasks to signal.")
            return

        log.debug("Sending signal %s to %d task(s) ...", signal, len(tasks))
        for task in tasks:
            task.signal_worker(signal)

        log.debug("All tasks signalled. Tasks' worker status:\n  %s",
                  ", ".join([str(t.worker_status) for t in tasks]))

    def _handle_pending_exceptions(self) -> None:
        """This method handles the list of pending exceptions during working,
        starting from the one added most recently.

        As the WorkerManager occupies the main thread, it is difficult for
        other threads to signal to the WorkerManager that an exception
        occurred.
        The pending_exceptions attribute allows such a handling; child threads
        can just add an exception object to it and they get handled during
        working of the WorkerManager.

        This method handles the following exception types in a specific manner:

            - ``WorkerTaskStopConditionFulfilled``: never raising or logging
            - ``WorkerTaskNonZeroExit``: raising or logging depending on the
              value of the ``nonzero_exit_handling`` property

        Returns:
            None

        Raises:
            exc: The exception that was added first to the queue of pending
                exceptions
        """

        def log_task_stream(task: WorkerTask, *, num_entries: int,
                            stream_name: str='out') -> None:
            """Logs the last `num_entries` from the log of the `stream_name`
            of the given WorkerTask object using log.error
            """
            # Get the stream
            stream = task.streams[stream_name]

            # Get lines and print stream using logging module
            lines = stream['log'][-num_entries:]
            log.error("Last ≤%d lines of combined stdout and stderr:\n"
                      "\n  %s\n", num_entries, "\n  ".join(lines))

        if self.pending_exceptions.empty():
            log.debug("No exceptions pending.")
            return
        # else: there was at least one exception

        # Go over all exceptions
        while not self.pending_exceptions.empty():
            # Get one exception off the queue
            exc = self.pending_exceptions.get_nowait()

            # Currently, only WorkerTaskNonZeroExit exceptions are handled here
            # If the type does not match, can directly raise it
            if not isinstance(exc, WorkerTaskNonZeroExit):
                log.error("Encountered a pending exception that requires "
                          "raising!")
                raise exc

            log.debug("Handling %s ...", exc.__class__.__name__)

            # Take care of stop conditions, which do NOT need to be raised
            if isinstance(exc, WorkerTaskStopConditionFulfilled):
                continue

            # Distinguish different ways of handling these exceptions
            # Ignore all
            if self.nonzero_exit_handling == 'ignore':
                # Done here. Continue with the next exception
                continue

            # Ignore terminated tasks for `warn` and `ignore` levels
            elif (    abs(exc.task.worker_status) == SIGMAP['SIGTERM']
                  and self.nonzero_exit_handling not in ['warn_all', 'raise']):
                continue

            # else: will generate some log output, so need to adjust Reporter
            #       to not use CR characters which would mangle up the output
            if self.reporter:
                self.reporter.suppress_cr = True

            # Provide some info on the exit status

            if self.nonzero_exit_handling in ['warn', 'warn_all']:
                # Print the error and the last few lines of the error log
                log.warning(str(exc))
                log_task_stream(exc.task, num_entries=8)

                # Nothing else to do
                continue

            # At this stage, the handling mode is 'raise'. Show more log lines:
            log.critical(str(exc))
            log_task_stream(exc.task, num_entries=24)

            # By raising here, the except block in start_working will be
            # invoked and terminate workers before calling sys.exit
            raise exc

        # The pending_exceptions list is now empty
        log.debug("Handled all pending exceptions.")


# Custom exceptions -----------------------------------------------------------


class WorkerManagerError(BaseException):
    """The base exception class for WorkerManager errors"""


class WorkerManagerTotalTimeout(WorkerManagerError):
    """Raised when a total timeout occurred"""


class WorkerTaskError(WorkerManagerError):
    """Raised when there was an error in a WorkerTask"""


class WorkerTaskNonZeroExit(WorkerTaskError):
    """Can be raised when a WorkerTask exited with a non-zero exit code."""

    def __init__(self, task: WorkerTask, *args, **kwargs):
        # Store the task
        self.task = task

        # Pass everything else to the parent init
        super().__init__(*args, **kwargs)

    def __str__(self) -> str:
        """Returns information on the error"""
        signals = [signal for signal, signum in SIGMAP.items()
                   if signum == abs(self.task.worker_status)]

        return (f"Task '{self.task.name}' exited with non-zero exit "
                f"status: {self.task.worker_status}.\nThis may originate from "
                f"the following signals:  {', '.join(signals)}.\n"
                "Googling these might help with identifying the error. "
                "Also, inspect the log and the log file for further error "
                "messages. To increase verbosity, run in debug mode, e.g. by "
                "passing the --debug flag to the CLI.")


class WorkerTaskStopConditionFulfilled(WorkerTaskNonZeroExit):
    """An exception that is raised when a worker-specific stop condition was
    fulfilled. This allows being handled separately to other non-zero exits.
    """
