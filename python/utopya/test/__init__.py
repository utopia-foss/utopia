
# Set default log level to DEBUG
import logging
logging.basicConfig(level=logging.DEBUG)

# Silence some modules that are too verbose
logging.getLogger('paramspace').setLevel(logging.INFO)

logging.getLogger('utopya.task').setLevel(logging.INFO)
logging.getLogger('utopya.reporter').setLevel(logging.INFO)
