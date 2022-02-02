"""Implementation of the Reporter class."""

import os
import sys
import logging
from shutil import get_terminal_size as _get_terminal_size
from functools import partial
from datetime import datetime as dt
from datetime import timedelta
from typing import Union, List, Callable, Dict
from collections import OrderedDict, Counter, deque

import numpy as np
import paramspace as psp

from .tools import format_time, TTY_COLS

# Initialise logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class ReportFormat:

    def __init__(self, *, parser: Callable, writers: List[Callable],
                 min_report_intv: float=None):
        """Initialises a ReportFormat object, which gathers callables needed to
        create a report in a certain format.

        Args:
            parser (Callable): The parser method to use
            writers (List[Callable]): The writer method(s) to use
            min_report_intv (float, optional): The minimum report interval of
                reports in this format.
        """

        # Store parser and writers
        self.parser = parser
        self.writers = writers

        # Set minimum report interval
        self._min_report_intv = None
        self.min_report_intv = min_report_intv

        # Store num of reports and time of last report
        self.num_reports = 0
        self.last_report = dt.fromtimestamp(0)  # waaay back

    @property
    def min_report_intv(self) -> Union[timedelta, None]:
        """Returns the minimum report intv"""
        return self._min_report_intv

    @min_report_intv.setter
    def min_report_intv(self, sec: float):
        """Set the minimum report interval"""
        self._min_report_intv = timedelta(seconds=sec) if sec else None

    @property
    def reporting_blocked(self) -> bool:
        """Determines whether this ReportFormat was generated"""
        if not self.min_report_intv:
            # Never blocked
            return False
        # Check time since last report
        return (dt.now() - self.last_report) < self.min_report_intv

    def report(self, *, force: bool=False, parser_kwargs: dict=None) -> bool:
        """Parses and writes a report corresponding to this object's format.

        If within the minimum report interval, will return False.

        Args:
            force (bool, optional): If True, will ignore the min_report_intv
            parser_kwargs (dict, optional): Keyword arguments passed on to the
                parser

        Returns:
            bool: Whether a report was generated or not

        """
        if not force and self.reporting_blocked:
            # Do not report
            return False

        # Generate the report
        log.debug("Creating report using parser '%s' ...", self.parser)
        report = self.parser(report_no=self.num_reports,
                             **(parser_kwargs if parser_kwargs else {}))
        log.debug("Created report of length %d.", len(report))

        # Write the report
        for writer_name, writer in self.writers.items():
            log.debug("Writing report using writer '%s' ...", writer_name)
            writer(report)

        # Update counter and last report time
        self.num_reports += 1
        self.last_report = dt.now()

        return True


# -----------------------------------------------------------------------------

class Reporter:
    """The Reporter class holds general reporting capabilities.

    It needs to be subclassed in order to specialise its reporting functions.
    """

    def __init__(self, *,
                 report_formats: Union[List[str], Dict[str, dict]]=None,
                 default_format: str=None, report_dir: str=None,
                 suppress_cr: bool=False):
        """Initialize the Reporter for the WorkerManager.

        Args:
            report_formats (Union[List[str], Dict[str, dict]], optional): The
                report formats to use with this reporter. If given as list
                of strings, the strings are the names of the report formats as
                well as those of the parsers; all other parameters are the
                defaults. If given as dict of dicts, the keys are the names of
                the formats and the inner dicts are the parameters to create
                report formats from.
            default_format (str, optional): The name of the default report
                format; if None is given, the .report method requires the name
                of a report format.
            report_dir (str, optional): if reporting to a file; this is the
                base directory that is reported to.
            suppress_cr (bool, optional): Whether to suppress carriage return
                characters in writers. This option is useful when the reporter
                is not the only class that writes to a stream.
        """

        super().__init__()

        # Initialise property-managed attributes
        self._report_formats = dict()
        self._default_format = None
        self._suppress_cr = False

        # Ensure the report_formats argument is a dict
        if report_formats is None:
            report_formats = dict()

        elif isinstance(report_formats, (list, tuple)):
            report_formats = {f: dict() for f in report_formats}

        # And add these report formats to the reporter
        for name, params in report_formats.items():
            self.add_report_format(name, **params)

        # Set the default report format, if given
        if default_format:
            self.default_format = default_format

        # Store the report dir
        if report_dir:
            self.report_dir = os.path.expanduser(str(report_dir))
        else:
            self.report_dir = None

        # Other attributes
        self.suppress_cr = suppress_cr  # NOTE writers need to implement this

        log.debug("Reporter.__init__ finished.")

    # Properties ..............................................................

    @property
    def report_formats(self) -> dict:
        """Returns the dict of ReportFormat objects."""
        return self._report_formats

    @property
    def default_format(self) -> Union[None, ReportFormat]:
        """Returns the default report format or None, if not set."""
        return self._default_format

    @default_format.setter
    def default_format(self, name: str):
        """Given the name of the report formats, set the default value."""
        if name is not None:
            self._default_format = self.report_formats[name]
            log.debug("Set default report format to '%s'.", name)
        else:
            self._default_format = None
            log.debug("Unset default report format.")

    @property
    def suppress_cr(self) -> bool:
        """Whether to suppress a carriage return. Objects using the reporter
        can set this property to communicate that they will be putting content
        into the stdout stream as well. The writers can check this property
        and adjust their behaviour accordingly.
        """
        return self._suppress_cr

    @suppress_cr.setter
    def suppress_cr(self, val: bool):
        """Set the suppress_cr property.

        When setting this to True the first time, a linebreak is issued in
        order to not overwrite any previously written lines that ended with
        a carriage return character.
        """
        # Go to the next line
        if val and not self.suppress_cr:
            print("")

        # Set the value
        self._suppress_cr = val

    # Public API ..............................................................

    def add_report_format(self, name: str, *, parser: str=None,
                          write_to: Union[str, Dict[str, dict]]='stdout',
                          min_report_intv: float=None, rf_kwargs: dict=None,
                          **parser_kwargs):
        """Add a report format to this reporter.

        Args:
            name (str): The name of this format
            parser (str, optional): The name of the parser; if not given, the
                name of the report format is assumed
            write_to (Union[str, Dict[str, dict]], optional): The name of the
                writer. If this is a dict of dict, the keys will be
                interpreted as the names of the writers and the nested dict as
                the ``**kwargs`` to the writer function.
            min_report_intv (float, optional): The minimum report interval (in
                seconds) for this report format
            rf_kwargs (dict, optional): Further kwargs to ReportFormat.__init__
            **parser_kwargs: The kwargs to the parser function

        Raises:
            ValueError: A report format with this `name` already exists
        """
        if name in self.report_formats:
            raise ValueError("A report format with the name {} already exists."
                             "".format(name))

        # Get the parser and writer function
        parser = self._resolve_parser(parser if parser else name,
                                      **parser_kwargs)
        writers = self._resolve_writers(write_to)

        # Initialise the ReportFormat object with the parsers and writers
        rf = ReportFormat(parser=parser, writers=writers,
                          min_report_intv=min_report_intv,
                          **(rf_kwargs if rf_kwargs else {}))

        self._report_formats[name] = rf
        log.debug("Added report format '%s' to %s.",
                  name, self.__class__.__name__)

    def report(self, report_format: str=None, **kwargs) -> bool:
        """Create a report with the given format; if none is given, the default
        format is used.

        Args:
            report_format (str, optional): The report format to use
            **kwargs: Passed on to the ReportFormat.report() call

        Returns:
            bool: Whether there was a report

        Raises:
            ValueError: If no default format was set and no report format name
                was given
        """
        # Get the report format to use
        if report_format is None:
            if self.default_format is None:
                raise ValueError("Either a default format needs to be set for "
                                 "this {} or the name of the report format "
                                 "needs to be supplied to the .report method."
                                 "".format(self.__class__.__name__))

            rf = self.default_format

        else:
            rf = self.report_formats[report_format]

        # Delegate reporting to the ReportFormat class
        return rf.report(**kwargs)

    def parse_and_write(self, *, parser: Union[str, Callable],
                        write_to: Union[str, Callable], **parser_kwargs):
        """This function allows to select a parser and writer explicitly.

        Args:
            parser (Union[str, Callable]): The parser method to use.
            write_to (Union[str, Callable]): The write method to use. Can also
                be a sequence of names and/or callables or a Dict. For allowed
                specification formats, see the ._resolve_writers method.
            **parser_kwargs: Passed to the parser, if given
        """

        # Determine the parser
        parser = self._resolve_parser(parser, **parser_kwargs)

        # Parse the report
        report = parser()
        log.debug("Parsed report using %s, got string of length %d.",
                  parser, len(report))

        # Determine the writers
        writers = self._resolve_writers(write_to)

        # Write the report
        for writer_name, writer in writers.items():
            writer(report)
            log.debug("Wrote report using %s .", writer_name)

    # Private methods .........................................................

    def _resolve_parser(self, parser: Union[str, Callable],
                        **parser_kwargs) -> Callable:
        """Given a string or a callable, returns the corresponding callable.

        Args:
            parser (Union[str, Callable]): If a callable is already given,
                returns that; otherwise looks for a parser method with the
                given name in the attributes of this class.
            **parser_kwargs: Arguments that should be passed to the parser.
                If given, a new function is created where these arguments are
                already included.

        Returns:
            Callable: The desired parser function

        Raises:
            ValueError: If no parser with the given name is available
        """
        if not callable(parser):
            # A name was given; try to resolve from attributes
            try:
                parser = getattr(self, '_parse_' + parser)
            except AttributeError as err:
                raise ValueError("No parser named '{}' available in {}!"
                                 "".format(parser,
                                           self.__class__.__name__)) from err

            log.debug("Resolved parser: %s", str(parser))

        # parser is now a callable

        # If given, partially apply the kwargs
        if parser_kwargs:
            log.debug("Binding parser_kwargs to parser method ...")
            parser = partial(parser, **parser_kwargs)

        return parser

    def _resolve_writers(self, write_to) -> Dict[str, Callable]:
        """Resolves the given argument to a list of callable writer functions.

        Args:
            write_to: a specification of the writers to use. Allows many
                different ways of specifying the writer functions:
                - str: the name of the writer method of this reporter
                - Callable: the writer function to use
                - sequence of str and/or Callable: the names and/or functions
                to use
                - Dict[str, dict]: the names of the writer functions and
                additional keyword arguments.

        Returns:
            Dict[str, Callable]: the writers (key: name, value: writer method)

        Raises:
            TypeError: Invalid `write_to` argument
            ValueError: A writer with that name was already added or a writer
                with the given name is not available.

        """
        def get_callable_name(c) -> str:
            """Returns the name of the callable"""
            if hasattr(c, '__name__'):
                return c.__name__
            # Does not have that attribute, e.g. because it is a partial func
            return str(c)

        # The target dict of callables
        writers = {}

        # First, need to bring the argument into a uniform structure.
        # This requires checking the many possible input formats

        # If a single callable is already given, return that
        if callable(write_to):
            return {get_callable_name(write_to): write_to}

        # If a single string is given, bring it into dict format
        elif isinstance(write_to, str):
            write_to = {write_to: {}}

        # If a list is given, move the callables to the writers dict; the items
        # that are strings remain.
        elif isinstance(write_to, (list, tuple)):
            wt = {}
            for item in write_to:
                if callable(item):
                    # Check if already present
                    if item in writers.values():
                        raise ValueError("Given writer callable with name "
                                         "'{}' was already added!"
                                         "".format(get_callable_name(item)))
                    writers[get_callable_name(item)] = item

                elif isinstance(item, str):
                    # Add an empty entry to the new write_to dict
                    wt[item] = dict()

                else:
                    raise TypeError("One item of given `write_to` argument {} "
                                    "of type {} was neither a string nor a "
                                    "callable! "
                                    "".format(write_to, type(write_to)))

            # Use the new write_to dict
            write_to = wt

        # Ensure that the format is a dict now
        if not isinstance(write_to, dict):
            raise TypeError("Invalid type {} for argument `write_to`!"
                            "".format(type(write_to)))

        # Now populate the writers dict with the remaining str-specified funcs
        for writer_name, params in write_to.items():
            try:
                writer = getattr(self, '_write_to_' + writer_name)

            except AttributeError as err:
                raise ValueError("No writer named '{}' available in {}!"
                                 "".format(writer_name,
                                           self.__class__.__name__)) from err

            # If given, partially apply the params
            if params:
                writer = partial(writer, **params)

            # Store in dict of writers
            writers[writer_name] = writer
            log.debug("Added writer with name '%s'.", writer_name)

        return writers

    # Parser methods ..........................................................

    # None available in the base class

    # Writer methods ..........................................................

    def _write_to_stdout(self, s: str, *, flush: bool=True, **print_kws):
        """Writes the given string to stdout using the print function.

        Args:
            s (str): The string to write
            flush (bool, optional): Whether to flush directly; default: True
            **print_kws: Other print function keyword arguments
        """
        if self.suppress_cr and print_kws.get('end') == "\r":
            # Enforce line feed
            print_kws['end'] = "\n"

        print(s, flush=flush, **print_kws)

    def _write_to_stdout_noreturn(self, s: str, *, prepend="  "):
        """Writes to stdout without ending the line. Always flushes.

        Args:
            s (str): The string to write
            prepend (str, optional): Is prepended to the string; useful because
                the cursor might block this point of the terminal
            report_no (int, optional): accepted from ReportFormat call
        """
        if not self.suppress_cr:
            print(prepend + s, flush=True, end='\r')
        else:
            print(prepend + s, flush=True, end='\n')

    def _write_to_log(self, s: str, *, lvl: int=10, skip_if_empty: bool=False):
        """Writes the given string via the logging module.

        Args:
            s (str): The string to log
            lvl (int, optional): The level at which to log at; default: DEBUG (10)
            skip_if_empty (bool, optional): Whether to skip writing if ``s`` is
                empty.
        """
        if not s and skip_if_empty:
            return
        log.log(lvl, s)

    def _write_to_file(self, s: str, *, path: str="_report.txt",
                       mode: str='w', skip_if_empty: bool=False):
        """Writes the given string to a file

        Args:
            s (str): The string to write
            path (str, optional): The path to write it to; will be assumed
                relative to the ``report_dir`` attribute; if that is not
                given, ``path`` needs to be absolute. By default, assumes that
                there is a ``report_dir`` given.
            mode (str, optional): Writing mode of that file
            skip_if_empty (bool, optional): Whether to skip writing if ``s`` is
                empty.

        Raises:
            ValueError: If ``report_dir`` was not set and ``path`` is relative.
        """
        if not s and skip_if_empty:
            log.debug("Not writing to file because the given string is empty.")
            return

        # For given relative paths, join them to the report directory
        if not os.path.isabs(path):
            if not self.report_dir:
                raise ValueError("Need either an absolute `path` argument or "
                                 "initialise the {} with the `report_dir` "
                                 "argument such that `path` can be "
                                 "interpreted relative to that directory."
                                 "".format(self.__class__.__name__))

            path = os.path.join(self.report_dir, path)

        log.debug("Writing given string (length %d) to %s ...", len(s), path)

        with open(path, mode) as file:
            file.write(s)

        log.debug("Finished writing.")

# -----------------------------------------------------------------------------

class WorkerManagerReporter(Reporter):
    """This class reports on the state of the WorkerManager."""

    # Margin to use when writing to terminal
    TTY_MARGIN = 4

    # Symbols to use in progress bar parser
    PROGRESS_BAR_SYMBOLS = dict(
        finished="▓", active_progress="▒", active="░", space=" "
    )

    # .........................................................................

    def __init__(self, wm: 'utopya.workermanager.WorkerManager', *,
                 mv: 'utopya.multiverse.Multiverse'=None, **reporter_kwargs):
        """Initialize the Reporter for the WorkerManager.

        Args:
            wm (utopya.workermanager.WorkerManager): The associated
                WorkerManager instance
            mv (utopya.multiverse.Multiverse, optional): The Multiverse this
                reporter is used in.
                If this is provided, it can be used in report parsers, e.g. to
                provide additional information on simulations.
            **reporter_kwargs: Passed on to parent method
        """
        super().__init__(**reporter_kwargs)

        # Make sure that formats 'while_working' and 'after_work' are available
        if 'while_working' not in self.report_formats:
            log.debug("No report format 'while_working' found; adding one "
                      "because it is needed by the WorkerManager.")

            self.add_report_format('while_working', parser='progress_bar',
                                   write_to='stdout_noreturn')

        if 'after_work' not in self.report_formats:
            log.debug("No report format 'after_work' found; adding one "
                      "because it is needed by the WorkerManager.")

            self.add_report_format('after_work', parser='progress_bar',
                                   write_to='stdout_noreturn')

        # Store the WorkerManager and associate it with this reporter
        self._wm = wm
        wm.reporter = self
        log.debug("Associated reporter with WorkerManager.")

        # Other attributes
        self.mv = mv
        self.runtimes = []
        self.exit_codes = Counter()

        # For retaining some information for ETA calculation
        self._eta_info = dict()

        log.debug("WorkerManagerReporter initialised.")

    @property
    def wm(self):
        """Returns the associated WorkerManager."""
        return self._wm

    # Properties that extract info from the WorkerManager .....................

    @property
    def task_counters(self) -> OrderedDict:
        """Returns a dict of task counters:

            - ``total``: total number of registered WorkerManager tasks
            - ``active``: number of currently active tasks
            - ``finished``: number of finished tasks, *including* tasks that
              were stopped via a stop condition
            - ``stopped``: number of tasks for which stop conditions were
              fulfilled, see :ref:`stop_conds`
        """
        num = OrderedDict()
        num['total'] = self.wm.task_count
        num['active'] = len(self.wm.active_tasks)
        num['finished'] = self.wm.num_finished_tasks
        num['stopped'] = len(self.wm.stopped_tasks)
        return num

    @property
    def wm_progress(self) -> float:
        """The WorkerManager progress, between 0 and 1."""
        cntr = self.task_counters

        if cntr['total'] == 0:
            # No tasks were added yet, progress is zero
            return 0.

        # Get the active tasks' progress, in range [0, 1]
        active_progress = self.wm_active_tasks_progress

        # Calculate the total progress
        return (  cntr['finished']/cntr['total']
                + active_progress * cntr['active']/cntr['total'])

    @property
    def wm_active_tasks_progress(self) -> float:
        """The active tasks' progress

        If there are no active tasks in the worker manager, returns 0
        """
        progs = [t.progress for t in self._wm.active_tasks]
        if progs:
            return np.mean(progs)
        return 0.

    @property
    def wm_elapsed(self) -> Union[timedelta, None]:
        """Seconds elapsed since start of working or None if not yet started"""
        times = self.wm.times

        if times['start_working'] is None:
            # Not started yet
            return None

        elif times['end_working'] is None:
            # Currently working: measure against now
            return dt.now() - times['start_working']

        # Finished working: measure against end of work
        return times['end_working'] - times['start_working']

    @property
    def wm_times(self) -> dict:
        """Return the characteristics WorkerManager times. Calls
        :py:meth:`~utopya.reporter.WorkerManagerReporter.get_progress_info`
        without any additional arguments.
        """
        return self.get_progress_info()

    # Methods working on data .................................................

    def register_task(self, task: 'utopya.task.WorkerTask'):
        """Given the task object, extracts and stores some information.

        The information currently extracted is the run time and the exit code.

        This can be used as a callback function from a WorkerTask object.

        Args:
            task (utopya.task.WorkerTask): The WorkerTask to extract
                information from.
        """
        # Register the runtime
        if 'run_time' in task.profiling:
            self.runtimes.append(task.profiling['run_time'])

        # Increment the counter belonging to this exit status
        self.exit_codes[task.worker_status] += 1

    def calc_runtime_statistics(self, min_num: int=10) -> OrderedDict:
        """Calculates the current runtime statistics.

        Returns:
            OrderedDict: name of the calculated statistic and its value, i.e.
                the runtime in seconds
        """
        if len(self.runtimes) < min_num:
            # Only calculate if there is enough data
            return None

        # Throw out Nones and convert to np.array
        rts = np.array([rt for rt in self.runtimes if rt is not None])

        # Calculate statistics
        d = OrderedDict()
        d['total (CPU)'] = np.sum(rts)
        d['total (wall)']= np.sum(rts) / min(self._wm.num_workers, len(rts))
        d['mean']        = np.mean(rts)
        d[' (last 50%)'] = np.mean(rts[-len(rts)//2:])
        d[' (last 20%)'] = np.mean(rts[-len(rts)//5:])
        d[' (last 5%)']  = np.mean(rts[-len(rts)//20:])
        d['std']         = np.std(rts)
        d['min']         = np.min(rts)
        d['at 25%']      = np.percentile(rts, 25)
        d['median']      = np.median(rts)
        d['at 75%']      = np.percentile(rts, 75)
        d['max']         = np.max(rts)

        return d

    def get_progress_info(self, **eta_options) -> Dict[str, float]:
        """Compiles a dict containing progress information for the current
        work session.

        Args:
            **eta_options: Passed on to method calculating ``est_left``,
                :py:meth:`~utopya.reporter.WorkerManagerReporter._compute_est_left`.

        Returns:
            Dict[str, float]: Progress information. Guaranteed to contain the
                keys ``start``, ``now``, ``elapsed``, ``est_left``,
                ``est_end``, and ``end``.
        """
        d = dict(
            start=self.wm.times['start_working'],
            now=dt.now(),
            elapsed=self.wm_elapsed,
            est_left=None,
            est_end=None,
            end=self.wm.times['end_working'],
        )

        # Add estimate time remaining and ETA, if the WorkerManager started.
        if d['start'] is not None:
            progress = self.wm_progress
            if progress > 0.:
                d['est_left'] = self._compute_est_left(progress=progress,
                                                       elapsed=d['elapsed'],
                                                       **eta_options)

        if d['est_left'] is not None:
            d['est_end'] = d['now'] + d['est_left']

        return d

    def _compute_est_left(
        self,
        *,
        progress: float,
        elapsed: timedelta,
        mode: str = "from_start",
        progress_buffer_size: int = 60,
    ) -> timedelta:
        """Computes the estimated time left until the end of the work session
        (ETA) using the current progress value and the elapsed time.
        Depending on ``mode``, additional information may be included in the
        calculation.

        Args:
            progress (float): The current progress value, in (0, 1]
            elapsed (timedelta): The elapsed time since start
            mode (str, optional): By which mode to calculate the ETA. Available
                modes are:

                    - ``from_start``, where ETA is computed from the start of
                        work session.
                    - ``from_buffer``, where ETA is computed from a more
                        recent point during the work session. This uses a
                        buffer to keep track of recent progress and computes
                        the ETA against the oldest record (controlled by
                        argument ``progress_buffer_size``), giving more
                        accurate estimates for long-running work sessions.

            progress_buffer_size (int, optional): The size of the ring buffer
                used in  ``from_buffer`` mode.

        Returns:
            timedelta: Estimate for how much time is left until the end of the
                work session.
        """
        if mode is None or mode == "from_start":
            return ((1. - progress) / progress) * elapsed

        elif mode == "from_buffer":
            # Get / set up the progress buffer: a circular buffer which holds
            # at most ``progress_buffer_size`` elements.
            # Each element is a (progress, elapsed) tuple.
            try:
                pbuf = self._eta_info["progress_buffer"]

            except KeyError:
                log.debug("Setting up progress buffer (maxlen: %d) ...",
                          progress_buffer_size)
                pbuf = deque([(0., timedelta(0.))],
                             maxlen=progress_buffer_size)
                self._eta_info["progress_buffer"] = pbuf

            # Add new information to buffer
            pbuf.append((progress, elapsed))

            # Compute progress speed compared to first element of buffer
            _progress, _elapsed = pbuf[0]
            dp = progress - _progress

            if elapsed == _elapsed or dp <= 1.e-16:
                # Buffer useless or too little progress made; use simpler mode
                return self._compute_est_left(
                    progress=progress, elapsed=elapsed, mode="from_start"
                )

            return ((1. - progress) / dp) * (elapsed - _elapsed)

        else:
            raise ValueError(
                f"Invalid ETA computation mode '{mode}'! "
                "Available modes: from_start, from_buffer"
            )


    # Parser methods ..........................................................

    # One-line parsers . . . . . . . . . . . . . . . . . . . . . . . . . . . .

    def _parse_task_counters(self, *, report_no: int=None) -> str:
        """Return a string that shows the task counters of the WorkerManager

        Args:
            report_no (int, optional): Passed by ReportFormat call

        Returns:
            str: A str representation of the task counters of the WorkerManager
        """
        return ",  ".join(["{}: {}".format(k, v)
                           for k, v in self.task_counters.items()])

    def _parse_progress(self, *, report_no: int=None) -> str:
        """Returns a progress string

        Args:
            report_no (int, optional): Passed by ReportFormat call

        Returns:
            str: A simple progress indicator
        """
        cntr = self.task_counters

        if cntr['total'] <= 0:
            return "(No tasks assigned to WorkerManager yet.)"

        return ("Finished  {fin:>{digs:}d} / {tot:d}  ({p:.1f}%)"
                "".format(fin=cntr['finished'], tot=cntr['total'],
                          digs=len(str(cntr['total'])),
                          p=cntr['finished']/cntr['total'] * 100))

    def _parse_progress_bar(
        self, *,
        num_cols: Union[str, int]="fixed",
        fstr: str="  ╠{ticks[0]:}{ticks[1]:}{ticks[2]:}{ticks[3]:}╣ {info:}{times:}",
        info_fstr: str="{total_progress:>5.1f}% ",
        show_times: bool=False,
        times_fstr: str="| {elapsed:} elapsed | ~{est_left:} left ",
        times_fstr_final: str="| finished in {elapsed:} ",
        times_kwargs: dict={},
        report_no: int=None,
    ) -> str:
        """Returns a progress bar.

        It shows the amount of finished tasks, active tasks, and a percentage.

        Args:
            num_cols (Union[str, int], optional): The number of columns
                available for creating the progress bar. Can also be a string
                ``adaptive`` to poll terminal size upon each call, or ``fixed``
                to use the number of columns determined at import time.
            fstr (str, optional): The format string for the final output.
                Should contain the ``ticks`` 4-tuple, which makes up the
                progress bar, and can optionally contain the``info`` and
                ``times`` segments, formatted using the respective format
                string arguments.
            info_fstr (str, optional): The format string for the ``info``
                section of the final output. Available keys:

                    - ``total_progress``
                    - ``active_progress``
                    - ``cnt``, the task counters dictionary, see:
                        :py:meth:`~utopya.reporter.WorkerManagerReporter.task_counters`

            show_times (bool, optional): Whether to show a short version of the
                results of the times parser
            times_fstr (str, optional): Format string for times information
            times_fstr_final (str, optional): Format string for times
                information once the work session has ended
            times_kwargs (dict, optional): Passed on to ``times`` parser.
                Only used if ``show_times`` is set.
            report_no (int, optional): Passed by ReportFormat call

        Returns:
            str: The one-line progress bar
        """
        # Get the task counter and check that some tasks have been assigned
        cntr = self.task_counters

        if cntr['total'] <= 0:
            return "(No tasks assigned to WorkerManager yet.)"

        # Determine the format string for the times
        if show_times:
            if self.wm_progress == 1.:
                times_fstr = times_fstr_final
            times_str = self._parse_times(fstr=times_fstr, **times_kwargs)

        else:
            times_str = ""

        # Determine number of available columns
        if num_cols == "adaptive":
            num_cols = (
                _get_terminal_size((TTY_COLS, 20)).columns - self.TTY_MARGIN
            )

        elif num_cols == "fixed":
            num_cols = TTY_COLS - self.TTY_MARGIN

        # Get the active tasks' mean progress and calculate the total progress
        # (calling the wm_progress property would lead to inconsistencies)
        active_progress = self.wm_active_tasks_progress
        total_progress = (  cntr['finished']/cntr['total']
                          + active_progress*cntr['active']/cntr['total'])

        # Get the information string ready
        info_str = info_fstr.format(total_progress=total_progress * 100,
                                    active_progress=active_progress * 100,
                                    cnt=cntr)

        # Determine the progress bar width
        pb_width = num_cols - (5 + len(info_str) + len(times_str))

        # Only return percentage indicator if the width would be _very_ short
        if pb_width < 4:
            return " {:>5.1f}% ".format(cntr['finished']/cntr['total'] * 100)

        # Calculate the ticks for finished tasks
        ticks = dict()
        factor = pb_width / cntr['total']
        ticks['finished'] = int(round(cntr['finished'] * factor))

        # Calculate the active ticks and those in progress
        # NOTE Important to round only one of the two, leads to artifacts
        #      otherwise
        ticks['active_progress'] = int(active_progress * cntr['active']*factor)
        ticks['active'] = (  int(round(cntr['active']*factor))
                           - ticks['active_progress'])

        # Calculate spaces from the sum of all of the above
        ticks['space'] = pb_width - sum(ticks.values())

        # Have all info now, let's go format!
        syms = self.PROGRESS_BAR_SYMBOLS
        return fstr.format(
            ticks=(
                syms['finished'] * ticks['finished'],
                syms['active_progress'] * ticks['active_progress'],
                syms['active'] * ticks['active'],
                syms['space'] * ticks['space'],
            ),
            info=info_str, times=times_str,
        )

    def _parse_times(self, *,
                     fstr: str="Elapsed:  {elapsed:<8s}  |  Est. left:  {est_left:<8s}  |  Est. end:  {est_end:<10s}",
                     timefstr_short: str="%H:%M:%S",
                     timefstr_full: str="%d.%m., %H:%M:%S",
                     use_relative: bool=True,
                     times: dict=None,
                     report_no: int=None,
                     **progress_info_kwargs) -> str:
        """Parses the worker manager time information, including estimated
        time left or others.

        Args:
            fstr (str, optional): The main format string; gets as keys the
                results of the WorkerManager time information. Available keys:
                'elapsed', 'est_left', 'est_end', 'start', 'now', 'end'
            timefstr_short (str, optional): A time format string for absolute
                dates; short version.
            timefstr_full (str, optional): A time format string for absolute
                dates; long (ideally: full) version.
            use_relative (bool, optional): Whether for a date difference of 1
                to use relative dates, e.g. "Today, 13:37"
            times (dict, optional): A dict of times to use; this is mainly
                for testing purposes!
            report_no (int, optional): The report number passed by ReportFormat
            **progress_info_kwargs: Passed on to method calculating progress
                :py:meth:`~utopya.reporter.WorkerManagerReporter.get_progress_info`

        Returns:
            str: A string representation of the time information
        """
        # If no explicit times were given, calculate them now
        if times is None:
            times = self.get_progress_info(**progress_info_kwargs)

        # The dict of strings that is filled and passed to the fstr
        tstrs = dict()

        # Convert some values to duration strings
        if times['elapsed']:
            tstrs['elapsed'] = format_time(times['elapsed'])
        else:
            tstrs['elapsed'] = "(not started)"

        if times['est_left']:
            tstrs['est_left'] = format_time(times['est_left'],
                                            max_num_parts=2)
        else:
            # No est_left available
            # Distinguish between finished and not started simulaltions
            if self.wm_progress == 1.:
                tstrs['est_left'] = "(finished)"
            else:
                tstrs['est_left'] = "∞"

        # Check if the start and end times are given
        if not (times['start'] and times['est_end']):
            # Not given -> not started yet
            tstrs['start'] = "(not started)"
            tstrs['now'] = times['now'].strftime(timefstr_full)
            tstrs['est_end'] = "(unknown)"
            tstrs['end'] = "(unknown)"

        else:
            # Were given.
            # Decide which time format string to use, depending on whether
            # start and end are on the same day, and whether to put a manual
            # prefix in front
            prefixes = dict()

            # Calculate timedelta in days
            delta_days = (times['est_end'].date()
                          - times['start'].date()).days

            if delta_days == 0:
                # All on the same day -> use short format, no prefixes
                timefstr = timefstr_abs = timefstr_short

            elif delta_days == 1 and use_relative:
                # Use short format with prefixes
                timefstr = timefstr_short
                timefstr_abs = timefstr_full

                if times['now'].date() == times['start'].date():
                    # Same day as start -> end is tomorrow
                    prefixes['start'] = "Today, "
                    prefixes['est_end'] = "Tomorrow, "
                else:
                    # Same day as est end -> start was yesterday
                    prefixes['start'] = "Yesterday, "
                    prefixes['est_end'] = "Today, "

            else:
                # Full format
                timefstr = timefstr_abs = timefstr_full

            # Create the strings
            tstrs['start'] = times['start'].strftime(timefstr)
            tstrs['now'] = times['now'].strftime(timefstr_abs)
            tstrs['est_end'] = times['est_end'].strftime(timefstr)

            if times['end']:
                tstrs['end'] = times['end'].strftime(timefstr_abs)
            else:
                tstrs['end'] = "(unknown)"

            # Add prefixes
            for key, prefix in prefixes.items():
                tstrs[key] = prefix + tstrs[key]

        return fstr.format(**tstrs)

    # Multi-line parsers . . . . . . . . . . . . . . . . . . . . . . . . . . .

    def _parse_runtime_stats(self, *, fstr: str="  {k:<13s} {v:}",
                             join_char="\n", report_no: int=None) -> str:
        """Parses the runtime statistics dict into a multiline string

        Args:
            fstr (str, optional): The format string to use. Gets passed the
                keys 'k' and 'v' where k is the name of the entry and v its
                value.
            join_char (str, optional): The join character / string to put the
                elements together.
            report_no (int, optional): Passed by ReportFormat call

        Returns:
            str: The multi-line runtime statistics
        """
        rtstats = self.calc_runtime_statistics()

        parts = [fstr.format(k=k, v=format_time(v, ms_precision=1))
                 for k, v in rtstats.items()]

        return join_char.join(parts)

    def _parse_report(self, *, fstr: str="  {k:<{w:}s}  {v:}",
                      min_num: int=4, report_no: int=None,
                      show_individual_runtimes: bool=True,
                      task_label_singular: str="task",
                      task_label_plural: str="tasks",
                      ) -> str:
        """Parses a report for all tasks that were being worked on into a
        multiline string. The headings can be adjusted by keyword arguments.

        Args:
            fstr (str, optional): The format string to use. Gets passed the
                keys ``k`` and ``v`` where ``k`` is the name of the entry and
                ``v`` its value. Note that this format string is also used
                with ``v`` being a non-numeric value. Also, ``w`` can be used
                to have a key column of constant width.
            min_num (int, optional): The minimum number of universes needed to
                calculate runtime statistics.
            report_no (int, optional): Passed by ReportFormat call
            show_individual_runtimes (bool, optional): Whether to report
                individual universe runtimes; default: True. This should be
                disabled if there are a huge number of universes.
            task_label_singular (str, optional): The label to use in the report
                when referring to a single task.
            task_label_plural (str, optional): The label to use in the report
                when referring to multiple tasks.

        Returns:
            str: The multi-line simulation report string
        """
        # List that contains the parts that will be written
        parts = []

        # Calculate the runtime statistics and add them to the parts
        rtstats = self.calc_runtime_statistics(min_num=min_num)

        if rtstats is not None:
            parts += ["Runtime Statistics"]
            parts += ["------------------"]
            parts += [""]
            parts += ["The statistics below are calculated from all "
                      f"individual {task_label_singular} run times."]
            parts += [""]
            parts += ["  # {}:  {} / {}".format(task_label_plural,
                                                len(self.runtimes),
                                                len(self.wm.tasks))]
            parts += [""]
            parts += [fstr.format(k=k, v=format_time(v, ms_precision=1), w=12)
                      for k, v in rtstats.items()]
            parts += [""]
            parts += [""]

        # In cluster mode, add more information
        if self.wm.cluster_mode:
            _rcps = self.wm.resolved_cluster_params

            parts += ["Cluster Mode Information"]
            parts += ["------------------------"]
            parts += [""]
            parts += [fstr.format(k=k, v=_rcps[k], w=12)
                      for k in ('node_name', 'node_index', 'num_nodes',
                                'node_list', 'job_id', 'job_name',
                                'job_account', 'cluster_name', 'num_procs')
                      if _rcps.get(k) is not None]
            parts += [""]
            parts += [""]

        # If stop conditions were fulfilled, inform about those
        if self.wm.stop_conditions:
            def task_names(sc: set) -> str:
                if not sc.fulfilled_for:
                    return "(None)"
                return ", ".join(sorted([t.name for t in sc.fulfilled_for]))

            parts += ["Stop Conditions"]
            parts += ["---------------"]
            parts += [""]
            parts += [f"  {len(self.runtimes)} / {len(self.wm.tasks)} "
                      f"{task_label_plural} were stopped due to at least one "
                      "of the following stop conditions:"]
            parts += [""]
            parts += [f"  {sc}\n"
                      f"      {task_names(sc)}\n"
                      for sc in self.wm.stop_conditions]
            parts += [""]
            parts += [""]


        # Add individual universe run times
        if show_individual_runtimes:
            parts += [f"{task_label_singular.capitalize()} Runtimes"]
            parts += ["-" * len(parts[-1])]
            parts += [""]

            max_name_len = max([12] + [len(t.name) for t in self.wm.tasks])

            for task in self.wm.tasks:
                if 'run_time' in task.profiling:
                    rt = task.profiling['run_time']
                    parts += [fstr.format(k=task.name,
                                          v=format_time(rt, ms_precision=1),
                                          w=max_name_len)]

        return " \n".join(parts)

    def _parse_pspace_info(self, *, fstr: str, report_no: int=None) -> str:
        """Provides information about the parameter space.

        Extracts the ``parameter_space`` from the associated Multiverse's meta
        configuration and provides information on that.

        If there are multiple tasks specified, it is assumed that a sweep is or
        was being carried out and an information string is retrieved from the
        :py:class:`paramspace.paramspace.ParamSpace` object, which is then
        returned.
        If only a single task was defined, returns an empty string.

        Args:
            report_no (int, optional): Passed by ReportFormat call

        Returns:
            str: If there is more than one task, returns the result of
                :py:meth:`paramspace.paramspace.ParamSpace.get_info_str`.
                If not, returns a string denoting that there was only one task.
        """
        if self.mv is None:
            raise ValueError("No Multiverse associated with this reporter! "
                             "Cannot parse ParamSpace information.")

        pspace = self.mv.meta_cfg['parameter_space']
        if not isinstance(pspace, psp.ParamSpace):
            raise TypeError(f"Expected a ParamSpace object, got:\n\n{pspace}")

        if len(self.wm.tasks) <= 1:
            return ""

        return fstr.format(num_tasks=len(self.wm.tasks),
                           sweep_info=pspace.get_info_str())


    # Writer methods ..........................................................

    def _write_to_file(self, *args, path: str='_report.txt',
                       cluster_mode_path: str='{0:}_{node_name:}{ext:}',
                       **kwargs):
        """Overloads the parent method with capabilities needed in cluster mode

        All args and kwargs are passed through. If in cluster mode, the path
        is changed such that it includes the name of the node.

        Args:
            *args: Passed on to parent method
            path (str, optional): The path to save to
            cluster_mode_path (str, optional): The format string to use for the
                path in cluster mode. _Requires_ to contain the format key
                '{0:}' which retains the given `path`, extension split off.
                Extension can be used via 'ext' (already includes the dot).
                Additional format keys: 'node_name', 'job_id'.
            **kwargs: Passed on to parent method
        """
        if not self.wm.cluster_mode:
            return super()._write_to_file(*args, path=path, **kwargs)

        # else: in cluster mode. Use the information to build a new path
        # Existing information
        base_path, ext = os.path.splitext(path)
        fstr_args = [base_path]
        fstr_kwargs = dict(ext=ext)

        # Gather cluster mode arguments
        fstr_kwargs['node_name'] = self.wm.resolved_cluster_params['node_name']
        fstr_kwargs['job_id'] = self.wm.resolved_cluster_params['job_id']

        # Build the new path
        path = cluster_mode_path.format(*fstr_args, **fstr_kwargs)

        # And call the parent method
        return super()._write_to_file(*args, path=path, **kwargs)
