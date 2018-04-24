"""Implementation of the Reporter class."""

import sys
import logging
from datetime import datetime as dt
from typing import Union

# Initialise logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class Reporter:
    """The Reporter class holds general reporting capabilities.

    It needs to be subclassed in order to specialise its reporting functions.
    """

    def __init__(self, *, parser: str='default', parser_kwargs: dict=None, write_to: str='stdout', writer_kwargs: dict=None):
        """Initialize the Reporter for the WorkerManager."""

        super().__init__()

        # Prepare the default parser
        self.parser = getattr(self, "_parse_"+parser)
        self.parser_kwargs = parser_kwargs if parser_kwargs else {}

        # Resolve write functions and create a dictionary of bound func calls
        self.writer = getattr(self, "_write_to_" + write_to)
        self.writer_kwargs = writer_kwargs if writer_kwargs else {}

        # Add attributes
        self.counters = dict(reports=0)
        self.times = dict(init=dt.now(), last_report=None)

        log.debug("Reporter.__init__ finished.")

    # Public API ..............................................................

    def report(self, *, parser: str=None, parser_kwargs: dict=None, write_to: str=None, writer_kwargs: dict=None):
        """Create a report.

        This will use the default values, unless the arguments `parser` or
        `write_to` are given.
        
        Args:
            parser (str, optional): Description
            parser_kwargs (dict, optional): Description
            write_to (str, optional): Description
            writer_kwargs (dict, optional): Description
        """
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
            writer = getattr(self, "_write_to_" + writer)
            writer_kwargs = writer_kwargs if writer_kwargs else {}

        # Write the report
        writer(rep, **writer_kwargs)
        log.debug("Wrote report using %s .", writer.__name__)

        # Increment counter and times
        self.counters['reports'] += 1
        self.times['last_report'] = dt.now()

    # Parser methods ..........................................................
    
    def _parse_default(self) -> str:
        """The default parser; returns reporter initialsation time."""
        "Report #{:d}".format(self.counters['reports'] + 1)

    # Writer methods ..........................................................

    def _write_to_stdout(self, s: str, flush: bool=True, **print_kws):
        """Writes the given string to stdout"""
        print(s, flush=flush, **print_kws)

    def _write_to_file(self, s: str, path: str, mode: str='a'):
        """Writes the given string to a file"""
        raise NotImplementedError


class WorkerManagerReporter(Reporter):
    """This class reports on the state of the WorkerManager."""

    def __init__(self, wm, parser: str='one_line', **reporter_kwargs):
        """Initialize the Reporter for the WorkerManager."""

        super().__init__(**reporter_kwargs)

        # Store the WorkerManager and associate it with this reporter
        self._wm = wm
        wm.reporter = self

        log.debug("Initialised and associated WorkerManagerReporter.")

    @property
    def wm(self):
        """Returns the associated WorkerManager."""
        return self._wm

    def _parse_one_line(self) -> str:
        return "foo"
