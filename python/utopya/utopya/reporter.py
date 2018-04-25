"""Implementation of the Reporter class."""

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
            writer(report, report_no=self.num_reports)

        # Update counter and last report time
        self.num_reports += 1
        self.last_report = dt.now()

        return True


# -----------------------------------------------------------------------------

class Reporter:
    """The Reporter class holds general reporting capabilities.

    It needs to be subclassed in order to specialise its reporting functions.
    """

    def __init__(self, *, report_formats: Union[List[str], Dict[str, dict]]=None, default_format: str=None):
        """Initialize the Reporter for the WorkerManager.
        
        Args:
            report_formats (Union[List[str], Dict[str, dict]], optional):
                The report formats to use with this reporter. If given as list
                of strings, the strings are the names of the report formats as well as those of the parsers; all other parameters are the defaults. If given as dict of dicts, the keys are the names of
                the formats and the inner dicts are the parameters to create report formats from.
            default_format (str, optional): The name of the default report
                format; if None is given, the .report method requires the name
                of a report format.
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

        # Create dicts to store counters and times in
        self.counters = OrderedDict()
        self.times = OrderedDict()
        self.counters['reports'] = 0
        self.times['init'] = dt.now()

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
            TypeError: Invalid `write_to` type
            ValueError: A report format with this `name` already exists
        """
        if name in self.report_formats:
            raise ValueError("A report format with the name {} already exists."
                             "".format(name))

        # Get the parser function
        parser_name = parser if parser else name
        try:
            parser = getattr(self, '_parse_' + parser_name)
        except AttributeError as err:
            raise ValueError("No parser named '{}' available in {}!"
                             "".format(parser_name,
                                       self.__class__.__name__)) from err

        # If given, create a lambda function that already passes the kwargs
        if parser_kwargs:
            parser = lambda *a, **kws: parser(*a, **parser_kwargs, **kws)

        # Determine the writers
        if isinstance(write_to, str):
            # Ensure it is of the dict-format
            write_to = {write_to: {}}

        if not isinstance(write_to, dict):
            raise TypeError("Invalid type for argument `write_to`; needs to "
                            "be either a string or a dict of dicts.")

        writers = {}
        for writer_name, params in write_to.items():
            try:
                writer = getattr(self, '_write_to_' + writer_name)
            except AttributeError as err:
                raise ValueError("No writer named '{}' available in {}!"
                                 "".format(parser_name,
                                           self.__class__.__name__)) from err
            
            # If given, create a lambda function that already passes the kwargs
            if params:
                writer = lambda *a, **kws: writer(*a, **params, **kws)

            # Store in dict of writers
            writers[writer_name] = writer
            log.debug("Added writer with name '%s'.", writer_name)

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

    def parse_and_write(self, *, parser: str, write_to: str, parser_kwargs: dict=None, writer_kwargs: dict=None):
        """This function allows to select a parser and writer explicitly."""

        # Determine the parser    
        parser = getattr(self, "_parse_" + parser)
        parser_kwargs = parser_kwargs if parser_kwargs else {}

        # Parse the report
        report = parser()
        log.debug("Parsed report using %s, got string of length %d.",
                  parser.__name__, len(report))

        # Determine the writer
        writer = getattr(self, "_write_to_" + write_to)
        writer_kwargs = writer_kwargs if writer_kwargs else {}

        # Write the report
        writer(report, **writer_kwargs)
        log.debug("Wrote report using %s .", writer.__name__)

    # Parser methods ..........................................................
    
    def _parse_runtime(self, *, report_no: int=None) -> str:
        """The default parser; returns reporter run time."""
        runtime = (dt.now() - self.times['init']).total_seconds()
        return ("Reporter run time:  {}  and counting ..."
                "".format(tools.format_time(runtime)))

    # Writer methods ..........................................................

    def _write_to_stdout(self, s: str, *, flush: bool=True, report_no: int=None, **print_kws):
        """Writes the given string to stdout"""
        print(s, flush=flush, **print_kws)

    def _write_to_log(self, s: str, *, lvl: int=10, report_no: int=None):
        """Writes the given string via the logging module"""
        log.log(lvl, s)

    def _write_to_file(self, s: str, *, path: str, mode: str='a', report_no: int=None):
        """Writes the given string to a file"""
        raise NotImplementedError


# -----------------------------------------------------------------------------

class WorkerManagerReporter(Reporter):
    """This class reports on the state of the WorkerManager."""

    def __init__(self, wm, **reporter_kwargs):
        """Initialize the Reporter for the WorkerManager."""

        super().__init__(**reporter_kwargs)

        # Store the WorkerManager and associate it with this reporter
        self._wm = wm
        wm.reporter = self
        log.debug("Associated reporter with WorkerManager.")

        # Attributes

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
        num['queued'] = self.wm.task_queue.qsize()
        num['active'] = len(self.wm.active_tasks)
        num['finished'] = num['total'] - (num['queued'] + num['active'])
        return num


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

    def _parse_progress_bar(self, *, num_cols: int=tools.TTY_COLS - 10, report_no: int=None):
        """Returns a progress bar"""
        # Define the symbols to use
        syms = dict(finished="▓", active="░", queued=" ", space=" ")

        # Calculate number of ticks
        pb_width = num_cols - (7 + 2)

        # Get the task counter and check that some tasks have been assigned
        cntr = self.task_counters

        if cntr['total'] <= 0:
            return "(No tasks assigned to WorkerManager yet.)"

        # Calculate the ticks
        ticks = dict()
        ticks['finished'] = int(cntr['finished'] / cntr['total'] * pb_width)
        ticks['active'] = int(cntr['active'] / cntr['total'] * pb_width)
        ticks['queued'] = int(cntr['queued'] / cntr['total'] * pb_width)
        ticks['space'] = pb_width - sum(ticks.values())
        # Note: the space ticks are needed as int is rounding down

        # Format the progress bar
        return ("╠{:}{:}{:}{:}╣ {p:>5.1f}%"
                "".format(syms['finished'] * ticks['finished'],
                          syms['active'] * ticks['active'],
                          syms['queued'] * ticks['queued'],
                          syms['space'] * ticks['space'],
                          p=cntr['finished']/cntr['total'] * 100))

