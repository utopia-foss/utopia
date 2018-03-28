"""Implementation of the Reporter class."""
from datetime import datetime as dt
from typing import Union

class Reporter:
    """The Reporter is where everything is reported.
    
    Reportery reportery...
    """

    def __init__(self, report_dir: str,):
        """Initialize the Reporter.

        Args:
            report_dir (str): Description
        """
        self.report_dir = report_dir

    def report(self, targets: list, content: Union[str, list, dict]=None) -> None:
        """When called, creates a report."""
        # ensure targets is list of targets:
        if not isinstance(targets, list):
            target_list = list()
            target_list.append(targets)

        if 'stdout' in targets:
            if content:
                print(content)
            else:
                print("Hey. :)")


class WorkerManagerReporter(Reporter):
    """The specialised case of a reporter for a WorkerManager.
    Uses Data from the monitor (from backend) handeled via Workermanager ?"""

    def __init__(self, **kwargs):
        """Initialize the Reporter for the WorkerManager."""
        super().__init__(**kwargs)

        # Additional attributes
        self.wm = None  # needs to be set by the WorkerManager

    def report_process(self, proc: dict, targets: list) -> None:
        """Report statistics about a process.

        This method should only be called if the process finished already.

        NOTE: this method should be called in WorkerManager._worker_finished
        """
        if isinstance(proc['end_time'], dt):
            # approximate runtime as datetime object
            runtime = proc['end_time'] - proc['create_time']

        else:
            raise ValueError("Only datetime.datetime objects are accepted.")

        self.report(targets=targets, content=runtime)

    def report_progress(self, proc: dict, tasks: dict, targets: list):  # tasks dict with finish, active, remaining in queue
        """ Report progress of the run. """
