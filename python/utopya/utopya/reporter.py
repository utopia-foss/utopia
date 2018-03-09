"""Implements the Reporter class"""

class Reporter:
    def __init__(self, report_dir: str,):
        """Initialize the Reporter.

        Args:
            report_dir (str): Description
        """
        self.report_dir = report_dir

    def report(self, targets: list) -> None:
        """When called, creates a report"""
        if 'stdout' in targets:
            print("Hey. :)")


class WorkerManagerReporter(Reporter):
    """The specialised case of a reporter for a WorkerManager"""

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        # Additional attributes
        self.wm = None  # needs to be set by the WorkerManager
