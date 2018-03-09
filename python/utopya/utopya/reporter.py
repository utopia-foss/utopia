"""Implementation of the Reporter class."""
import time

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

    def report(self, targets: list) -> None:
        """When called, creates a report."""
        if 'stdout' in targets:
            print("Hey. :)")


class WorkerManagerReporter(Reporter):
    """The specialised case of a reporter for a WorkerManager."""

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
        # approximate runtime in seconds
        runtime = proc['end_time'] - proc['create_time']

        if 'stdout' in targets:
            print(runtime)  # this needs to be fancier :)
