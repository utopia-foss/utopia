"""This module implements the StopCondition class, which is used by the
WorkerManager to stop a worker process in certain situations."""

import logging
from subprocess import Popen

import yaml

# Initialise logger
log = logging.getLogger(__name__)


class StopCondition:
    """A StopCondition object holds information on the conditions in which a worker process should be stopped.

    It is formulated in a general way, applying to all Workers. The attributes
    of this class store the information required to deduce whether the
    condition if fulfilled or not.
    """
    def __init__(self, name: str, description: str=None):
        """Create a new stop condition"""
        self.name = name
        self.description = description

    def __str__(self) -> str:
        return ("StopCondition '{name:}':\n"
                "  {desc:}\n".format(name=self.name, desc=self.description))

    def fulfilled(self, proc: Popen, *, worker_info: dict) -> bool:
        """Checks if the stop condition is fulfilled for the given worker, using the information from the dict.
        
        Args:
            proc (Popen): Worker object that is to be checked
            worker_info (dict): The information dict for this specific worker
        """
        pass


# -----------------------------------------------------------------------------

def stop_cond_constructor(loader, node) -> StopCondition:
    """constructor for creating a StopCondition object from a mapping"""
    log.debug("Encountered tag associated with StopCondition.")

    if isinstance(node, yaml.nodes.MappingNode):
        log.debug("Constructing mapping ...")
        mapping = loader.construct_mapping(node, deep=True)
        stop_cond = StopCondition(**mapping)
    else:
        raise TypeError("StopCondition can only be constructed from a mapping "
                        "node, got node of type {} "
                        "with value:\n{}".format(type(node), node))

    return stop_cond
