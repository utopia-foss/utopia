"""Implementation of the Reporter class."""

import sys
import logging
from datetime import datetime as dt
from datetime import timedelta
from typing import Union
from collections import OrderedDict

import numpy as np

import utopya.tools as tools

# Initialise logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class Reporter:
    """The Reporter class holds general reporting capabilities.

    It needs to be subclassed in order to specialise its reporting functions.
    """

    def __init__(self, *, min_report_intv: float=None, parser: str='default', parser_kwargs: dict=None, write_to: str='stdout', writer_kwargs: dict=None):
        """Initialize the Reporter for the WorkerManager.
        
        Args:
            min_report_intv (float, optional): The minimum time that needs to
                have passed since the last report before another report is
                created.
            parser (str, optional): Description
            parser_kwargs (dict, optional): Description
            write_to (str, optional): Description
            writer_kwargs (dict, optional): Description
        """

        super().__init__()

        # Property-managed attributes
        self._min_report_intv = None

        # Store the minimum report interval
        self.min_report_intv = min_report_intv

        # Prepare the default parser
        self.parser = getattr(self, "_parse_"+parser)
        self.parser_kwargs = parser_kwargs if parser_kwargs else {}

        # Resolve write functions and create a dictionary of bound func calls
        self.writer = getattr(self, "_write_to_" + write_to)
        self.writer_kwargs = writer_kwargs if writer_kwargs else {}

        # Add counters and times dicts and fill them with initial values
        self.counters = OrderedDict()
        self.times = OrderedDict()

        self.counters['reports'] = 0
        self.times['init'] = dt.now()
        self.times['last_report'] = dt.fromtimestamp(0)  # waaay back

        log.debug("Reporter.__init__ finished.")

    # Properties ..............................................................

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
        """Determines whether the reporter should report now or not.

        One reason why it should not report can be that the last report
        happened very recently. There can potentially be more reasons.
        """
        if not self.min_report_intv:
            # Never blocked
            return False
        # Check time since last report
        return (dt.now() - self.times['last_report']) < self.min_report_intv

    # Public API ..............................................................

    def report(self, *, force: bool=False, parser: str=None, parser_kwargs: dict=None, write_to: str=None, writer_kwargs: dict=None) -> bool:
        """Create a report.
        
        This will use the default values, unless the arguments `parser` or
        `write_to` are given.
        
        Args:
            force (bool, optional): If True, will ignore the minimum report
                interval.
            parser (str, optional): Description
            parser_kwargs (dict, optional): Description
            write_to (str, optional): Description
            writer_kwargs (dict, optional): Description
        
        Returns:
            bool: Whether there was a report
        """
        if not force and self.reporting_blocked:
            # Should not report right now
            return False

        # Determine the parser
        if not parser:
            # Use the default parser
            parser = self.parser
            parser_kwargs = self.parser_kwargs
        else:
            parser = getattr(self, "_parse_" + parser)
            parser_kwargs = parser_kwargs if parser_kwargs else {}

        # Parse the report
        rep = parser()
        log.debug("Parsed report using %s .", parser.__name__)

        # Determine the writer
        if write_to is None:
            writer = self.writer
            writer_kwargs = self.writer_kwargs
        else:
            writer = getattr(self, "_write_to_" + write_to)
            writer_kwargs = writer_kwargs if writer_kwargs else {}

        # Write the report
        writer(rep, **writer_kwargs)
        log.debug("Wrote report using %s .", writer.__name__)

        # Increment counter and times
        self.counters['reports'] += 1
        self.times['last_report'] = dt.now()

        return True

    # Parser methods ..........................................................
    
    def _parse_default(self) -> str:
        """The default parser; returns reporter initialsation time."""
        return "Report #{:d}".format(self.counters['reports'] + 1)

    # Writer methods ..........................................................

    def _write_to_stdout(self, s: str, flush: bool=True, **print_kws):
        """Writes the given string to stdout"""
        print(s, flush=flush, **print_kws)

    def _write_to_log(self, s: str, lvl: int=10):
        """Writes the given string via the logging module"""
        log.log(lvl, s)

    def _write_to_file(self, s: str, path: str, mode: str='a'):
        """Writes the given string to a file"""
        raise NotImplementedError


# -----------------------------------------------------------------------------

class WorkerManagerReporter(Reporter):
    """This class reports on the state of the WorkerManager."""

    def __init__(self, wm, parser: str='progress', **reporter_kwargs):
        """Initialize the Reporter for the WorkerManager."""

        super().__init__(parser=parser, **reporter_kwargs)

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

    def _parse_task_counters(self) -> str:
        """Return a string that shows the task counters"""
        return ",  ".join(["{}: {}".format(k, v)
                           for k, v in self.task_counters.items()])

    def _parse_progress(self) -> str:
        """Returns a progress string"""
        cntr = self.task_counters

        if cntr['total'] <= 0:
            return "(No tasks assigned to WorkerManager yet.)"

        return ("Finished  {fin:>{digs:}d} / {tot:d}  ({p:.1f}%)"
                "".format(fin=cntr['finished'], tot=cntr['total'],
                          digs=len(str(cntr['total'])),
                          p=cntr['finished']/cntr['total'] * 100))

    def _parse_progress_bar(self, num_cols: int=tools.TTY_COLS - 10):
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
