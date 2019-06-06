"""Supplies basic YAML interface, inherited from dantro and paramspace"""

from dantro.tools import yaml, load_yml, write_yml
import paramspace

# Make sure the yaml instances are really shared
assert yaml is paramspace.yaml
