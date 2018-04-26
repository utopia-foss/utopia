"""Implementation of the Reporter class."""

import os
import sys
import logging
from datetime import datetime as dt
from datetime import timedelta
from typing import Union, List, Callable, Dict
from collections import OrderedDict

import numpy as np

import utopya.tools as tools

# Initialise logger
log = logging.getLogger(__name__)

# Local constants
TTY_MARGIN = 2

# -----------------------------------------------------------------------------

class ReportFormat:

    def __init__(self, *, parser: Callable, writers: List[Callable], min_report_intv: float=None):
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

    def report(self, *, force: bool=False) -> bool:
        """Parses and writes a report corresponding to this object's format.
        
        If within the minimum report interval, will return False.
        
        Args:
            force (bool, optional): If True, will ignore the min_report_intv
        
        Returns:
            bool: Whether a report was generated or not
        
        """
        if not force and self.reporting_blocked:
            # Do not report
            return False

        # Generate the report
        log.debug("Creating report using '%s' parser...", self.parser.__name__)
        report = self.parser(report_no=self.num_reports)
        log.debug("Created report of length %d.", len(report))

        # Write the report
        for writer_name, writer in self.writers.items():
            log.debug("Writing report using '%s' ...", writer_name)
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

    def __init__(self, *, report_formats: Union[List[str], Dict[str, dict]]=None, default_format: str=None, report_dir: str=None):
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
        """

        super().__init__()

        # Initialise property-managed attributes
        self._report_formats = dict()
        self._default_format = None

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
        self.report_dir =os.path.expanduser(report_dir) if report_dir else None

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

    # Public API ..............................................................

    def add_report_format(self, name: str, *, parser: str=None, write_to: Union[str, Dict[str, dict]]='stdout', min_report_intv: float=None, rf_kwargs: dict=None, **parser_kwargs):
        """Add a report format to this reporter.
        
        Args:
            name (str): The name of this format
            parser (str, optional): The name of the parser; if not given, the
                name of the report format is assumed
            write_to (Union[str, Dict[str, dict]], optional): The name of the
                writer. If this is a dict of dict, the keys will be
                interpreted as the names of the writers and the nested dict as
                the **kwargs to the writer function.
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

    def report(self, report_format: str=None, *, force: bool=False) -> bool:
        """Create a report with the given format; if none is given, the default
        format is used.
        
        Args:
            report_format (str, optional): The report format to use
            force (bool, optional): If True, will ignore the minimum report
                interval.
        
        Returns:
            bool: Whether there was a report
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
        return rf.report(force=force)

    def parse_and_write(self, *, parser: Union[str, Callable], write_to: Union[str, Callable], **parser_kwargs):
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
                  parser.__name__, len(report))

        # Determine the writers
        writers = self._resolve_writers(write_to)

        # Write the report
        for writer_name, writer in writers.items():
            writer(report)
            log.debug("Wrote report using %s .", writer_name)

    # Private methods .........................................................
    
    def _resolve_parser(self, parser: Union[str, Callable], **parser_kwargs) -> Callable:
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

        # If given, create a lambda function that already passes the kwargs
        if parser_kwargs:
            log.debug("Binding parser_kwargs to parser method ...")
            parser = lambda *a, **kws: parser(*a, **parser_kwargs, **kws)

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
        # The target dict of callables
        writers = {}

        # First, need to bring the argument into a uniform structure.
        # This requires checking the many possible input formats

        # If a single callable is already given, return that
        if callable(write_to):
            return [write_to]

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
                    if item.__name__ in writers:
                        raise ValueError("Given writer callable with name "
                                         "'{}' already exists!"
                                         "".format(item.__name__))
                    writers[item.__name__] = item

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
            
            # If given, create a lambda function that already passes the kwargs
            if params:
                writer = lambda *a, **kws: writer(*a, **params, **kws)

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
        print(s, flush=flush, **print_kws)

    def _write_to_stdout_noreturn(self, s: str, *, prepend="  "):
        """Writes to stdout without ending the line. Always flushes.
        
        Args:
            s (str): The string to write
            prepend (str, optional): Is prepended to the string; useful because
                the cursor might block this point of the terminal
            report_no (int, optional): accepted from ReportFormat call
        """
        print(prepend + s, flush=True, end='\r')

    def _write_to_log(self, s: str, *, lvl: int=10):
        """Writes the given string via the logging module.
        
        Args:
            s (str): The string to log
            lvl (int, optional): The level at which to log at; default: DEBUG
        """
        log.log(lvl, s)

    def _write_to_file(self, s: str, *, path: str="_report.txt", mode: str='w'):
        """Writes the given string to a file
        
        Args:
            s (str): The string to write
            path (str): The path to write it to; will be assumed relative to
                the `report_dir` attribute; if that is not given, `path` needs
                to be absolute. By default, assumes that there is a report_dir
                given and writes to "_report.txt" in there.
            mode (str, optional): Writing mode of that file
        """
        # Check that the given path is valid
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

    def __init__(self, wm, **reporter_kwargs):
        """Initialize the Reporter for the WorkerManager."""

        super().__init__(**reporter_kwargs)

        # Make sure that a format 'working' is available
        if 'working' not in self.report_formats:
            log.debug("No report format 'working' found; adding one "
                      "because it is needed by the WorkerManager.")

            # Add a default configuration
            self.add_report_format('working', parser='progress_bar',
                                   write_to='stdout_noreturn')

        # Store the WorkerManager and associate it with this reporter
        self._wm = wm
        wm.reporter = self
        log.debug("Associated reporter with WorkerManager.")

        # Attributes
        self.runtimes = []

        log.debug("WorkerManagerReporter initialised.")

    @property
    def wm(self):
        """Returns the associated WorkerManager."""
        return self._wm

    # Properties that extract info from the WorkerManager .....................

    @property
    def task_counters(self) -> OrderedDict:
        """Returns a dict of task statistics:
        total: total number of tasks associated with the WorkerManager
        finished: number of finished tasks
        active: number of active tasks
        queued: number of unfinished tasks
        """
        num = OrderedDict()
        num['total'] = self.wm.task_count
        num['active'] = len(self.wm.active_tasks)
        num['finished'] = self.wm.num_finished_tasks
        return num

    # Methods working on data .................................................

    def register_runtime(self, task):
        """Given the task object, extracts the runtime and stores it.
        
        This can be used as a callback function from a WorkerTask object.
        
        Args:
            task (WorkerTask): The WorkerTask to extract the runtime from.
        """
        if 'run_time' in task.profiling:
            self.runtimes.append(task.profiling['run_time'])

    def calc_runtime_statistics(self, min_num: int=10) -> OrderedDict:
        """Calculates the current runtime statistics.
        
        Returns:
            OrderedDict: name of the calculated statistic and its value, i.e.
                the runtime in seconds
        """
        if len(self.runtimes) <= min_num:
            # Only calculate if there is enough data
            return None
        
        # Throw out Nones and convert to np.array
        rts = np.array([rt for rt in self.runtimes if rt is not None])
        
        # Calculate statistics
        d = OrderedDict()
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

    # Parser methods ..........................................................

    def _parse_task_counters(self, *, report_no: int=None) -> str:
        """Return a string that shows the task counters"""
        return ",  ".join(["{}: {}".format(k, v)
                           for k, v in self.task_counters.items()])

    def _parse_progress(self, *, report_no: int=None) -> str:
        """Returns a progress string"""
        cntr = self.task_counters

        if cntr['total'] <= 0:
            return "(No tasks assigned to WorkerManager yet.)"

        return ("Finished  {fin:>{digs:}d} / {tot:d}  ({p:.1f}%)"
                "".format(fin=cntr['finished'], tot=cntr['total'],
                          digs=len(str(cntr['total'])),
                          p=cntr['finished']/cntr['total'] * 100))

    def _parse_progress_bar(self, *, num_cols: int=(tools.TTY_COLS - TTY_MARGIN), show_total: bool=False, report_no: int=None):
        """Returns a progress bar"""
        # Get the task counter and check that some tasks have been assigned
        cntr = self.task_counters

        if cntr['total'] <= 0:
            return "(No tasks assigned to WorkerManager yet.)"

        # Define the symbols and format strings to use, calculating the
        # progress bar width alongside
        syms = dict(finished="▓", active="░", space=" ")

        if show_total:
            fstr = "  ╠{:}{:}{:}╣ {p:>5.1f}%  (of {total:d}) "
            pb_width = num_cols - (14 + 5 + len(str(cntr['total'])))
        
        else:
            fstr = "  ╠{:}{:}{:}╣ {p:>5.1f}% "
            pb_width = num_cols - (7 + 5)

        # Fall back to regular progress indicator if there is not enough space
        # for a progress bar
        if pb_width < 4:
            return self._parse_progress(report_no=report_no)

        # Calculate the ticks
        ticks = dict()
        factor = pb_width / cntr['total']
        ticks['finished'] = int(round(cntr['finished'] * factor))
        ticks['active'] = int(round(cntr['active'] * factor))
        ticks['space'] = pb_width - sum(ticks.values())
        # Note: the space ticks are needed as int is rounding down

        # Format the progress bar
        return (fstr.format(syms['finished'] * ticks['finished'],
                            syms['active'] * ticks['active'],
                            syms['space'] * ticks['space'],
                            p=cntr['finished']/cntr['total'] * 100,
                            total=cntr['total']))

    def _parse_runtime_stats(self, fstr: str="  {k:<13s} {v:}", join_char="\n") -> str:
        """Parses the runtime statistics dict into a multiline string"""
        rtstats = self.calc_runtime_statistics()

        parts = [fstr.format(k=k, v=tools.format_time(v, ms_precision=1))
                 for k, v in rtstats.items()]

        return join_char.join(parts)


