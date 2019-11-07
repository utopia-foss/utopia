"""The utopya package implements the frontend of Utopia"""

# Specify the version
__version__ = '0.6.0'
# NOTE This needs to correspond to the one in setup.py
# TODO single-source this!

# Use the dantro-provided logging module (with additional log levels)
from dantro.logging import getLogger
from dantro.logging import REMARK as DEFAULT_LOG_LEVEL

log = getLogger(__name__)

# Add colour logging to the root logger
# See API reference:  https://coloredlogs.readthedocs.io/en/latest/api.html
import coloredlogs

coloredlogs.install(logger=log,
                    level=DEFAULT_LOG_LEVEL,
                    fmt="%(levelname)-8s %(module)-15s %(message)s",
                    level_styles=dict(
                        trace=dict(color=242),              # grey
                        debug=dict(color=242),              # grey
                        remark=dict(color=246),             # grey
                        note=dict(color=23),                # faint blue
                        info=dict(bright=True),
                        progress=dict(color='green'),
                        hilight=dict(color='yellow', bold=True),
                        success=dict(color='green', bold=True),
                        warning=dict(color=202, bold=True), # orange
                        error=dict(color='red'),
                        critical=dict(color='red', bold=True)
                        ),
                    field_styles=dict(
                        levelname=dict(color='black', bold=True),
                        module=dict(color=242, faint=True)
                        ))

log.debug("Logging configured.")

# Define or import some global variables ......................................

# The global model registry object
from .model_registry import MODELS


# Import classes that should be easily accessible .............................
from .multiverse import Multiverse, FrozenMultiverse
from .datamanager import DataManager
from .datagroup import UniverseGroup, MultiverseGroup
from .model import Model
