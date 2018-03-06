"""For functions that are not bound to classes, but useful."""
import yaml


def recursive_update(self, d: dict, u: dict) -> dict:
    """Update dict d with values from dict u."""
    for key, val in u.items():
        if isinstance(val, dict):
            # Already a Mapping, continue recursion
            d[key] = recursive_update(d.get(key, {}), val)
        else:
            # Not a mapping -> at leaf -> update value
            d[key] = val 	# ... which is just u[key]
    return d
