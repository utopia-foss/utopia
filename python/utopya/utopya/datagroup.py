"""Implements data group classes specific to the Utopia output data structure.

They are based on the dantro's OrderedDataGroup class.
"""

import logging

from dantro.base import BaseDataGroup
from dantro.group import OrderedDataGroup

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class MultiverseGroup(OrderedDataGroup):
    """This group is meant to manage the `uni` group of the loaded data, i.e.
    the group where output of all universe groups is stored in.

    Its main aim is to provide easy access to universes. By default, universes
    are only identified by their ID, which is a zero-padded _string_. This
    group adds the ability to access them via integer indices.

    Furthermore, it will _in the future_ allow to access a subspace of the
    parameter space.
    """

    def __init__(self, *args, **kwargs):
        """Initializes the MultiverseGroup.
        
        Args:
            *args: Passed on to OrderedDataGroup.__init__
            **kwargs: Passed on to OrderedDataGroup.__init__
        """
        log.debug("MultiverseGroup.__init__ called.")

        # Initialise completely via parent method
        super().__init__(*args, **kwargs)

        # Create some private attribute used for caching ID access information
        self._max_id = None
        self._num_digs = None

        log.debug("MultiverseGroup.__init__ finished.")

    def __getitem__(self, key: int):
        """Returns the universe with the given ID.

        Universes are internally identified by a zero-padded string; as this
        is a rather fragile way of accessing them, it should be avoided!
        However, the DataManager's load method internally relies on strings
        as paths, which is why it cannot be prohibited here.
        """
        if isinstance(key, int):
            # Generate a padded string to access the universe
            key = self._padded_id_from_int(key)

        # Use the parent method to return the value
        return super().__getitem__(key)

    def __contains__(self, key: int) -> bool:
        """Checks if a universe with the given ID is stored in this group.

        Universes are internally identified by a zero-padded string; as this
        is a rather fragile way of accessing them, it should be avoided!
        However, the DataManager's load method internally relies on strings
        as paths, which is why it cannot be prohibited here.
        """
        if isinstance(key, int):
            # Generate a string from the given integer
            key = self._padded_id_from_int(key)

        return super().__contains__(key)

    def _padded_id_from_int(self, uni_id: int) -> str:
        """This generates a zero-padded universe ID string from the given int.

        When called for the first time, it checks the given keys and
        determines the maximum value and the number of digits it has; these
        values are then cached.
        If an ID larger than the maximum value is tried to be accessed, the
        check is performed again.
        """

        # Define a helper function
        def get_max_id_and_num_digs() -> tuple:
            """Checks all keys, then returns the max_id, num_digits tuple
            
            Returns:
                tuple: (max_id, num_digits)
            """
            # Get the current maximum ID
            max_id = self._max_id if self._max_id is not None else -1
            num_digs = self._num_digs if self._num_digs is not None else 0
            
            # Go over all keys and check
            # NOTE reverse iteration starts with the large IDs, which should
            #      reduce the number of assignments
            for k in reversed(self.keys()):
                # Separately, check for the maximum ID and number of digits.
                # It has to happen separately, as there is the possibility of
                # a larger zero-padding then would be needed for int(k).
                if int(k) > max_id:
                    max_id = int(k)

                if len(k) > num_digs:
                    num_digs = len(k)

            return max_id, len(str(max_id))

        # If cache values are not yet set or it is larger than the current one,
        # perform the check for available keys
        if self._max_id is None or uni_id > self._max_id:
            self._max_id, self._num_digs = get_max_id_and_num_digs()

        # Now, the cache values are up to date
        # Check the arguments; cannot be larger than the max_id now
        if uni_id > self._max_id:
            raise KeyError("No universe with ID {} available. The highest ID "
                           "found in {} was {}.".format(uni_id, self.logstr,
                                                        self._max_id))

        elif uni_id < 0:
            raise KeyError("Universe IDs cannot be negative! {} is negative."
                           "".format(uni_id))

        # Everything ok. Generate the zero-padded string.
        return "{id:0{digs:d}d}".format(id=uni_id, digs=self._num_digs)
