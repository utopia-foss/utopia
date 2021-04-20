"""This module implements the :py:class:`~utopya.parameter.Parameter` class
which is used when validating model and simulation parameters.
"""

import logging
import operator
from math import isinf as _isinf
from numbers import Number
from typing import Any, Tuple, Union, Sequence

import numpy as np
import paramspace as psp


# Local constants
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class ValidationError(ValueError):
    """Raised upon failure to validate a parameter"""
    pass


class Parameter:
    """The parameter class is used when a model parameter needs to be validated
    before commencing the model run. It can hold information on the parameter
    itself as well as its valid range and type and other meta-data.

    Per default, the ``Parameter`` class should be assumed to handle *scalar*
    parameters like numerical values or strings. For validating sequence-like
    parameters, corresponding specializing classes are to be implemeted.
    """

    # Available modes for :py:meth:`~utopya.parameter.Parameter.from_shorthand`
    SHORTHAND_MODES = (
        'is-probability',
        'is-positive',
        'is-int',
        'is-negative',
        'is-positive-int',
        'is-string',
        'is-negative-int',
        'is-bool',
        'is-unsigned',
    )

    # Comparators for the ``limits`` check, depending on ``limits_mode``
    LIMIT_COMPS = {
        '[': operator.ge,
        '(': operator.gt,
        ']': operator.le,
        ')': operator.lt,
    }

    # Possible limit modes
    LIMIT_MODES = ('[]', '()', '[)', '(]')

    # Default YAML tag to use for representing
    yaml_tag = '!param'

    # .........................................................................

    def __init__(self, *, default: Any,
                 name: str=None, description: str=None,
                 is_any_of: Sequence[Any]=None,
                 limits: Tuple[Union[None, float], Union[None, float]]=None,
                 limits_mode: str='[]',
                 dtype: Union[str, type]=None):
        """Creates a new Parameter object, which holds a default value as well
        as some constraints on the possible values this parameter can assume.

        Args:
            default (Any): the default value of the parameter.
            name (str, optional): the name of the parameter.
            description (str, optional): a description of this parameter or
                its effects.
            is_any_of (Sequence[Any], optional): a sequence of possible values
                this parameter can assume.
                If this parameter is given, ``limits`` cannot be used.
            limits (Tuple[Union[None, float], Union[None, float]], optional):
                the upper and lower bounds of the parameter (only applicable
                to scalar numerals). If None, the bound is assumed to be
                negative or positive infinity, respectively. Whether boundary
                values are included into the interval is controlled by the
                ``limits_mode`` argument.
                This argument is mutually exclusive with ``is_any_of``!
            limits_mode (str, optional): whether to interpret the limits as an
                open, closed, or semi-closed interval.
                Possible values: ``'[]'`` (closed, default), ``'()'`` (open),
                ``'[)'``, and ``'(]'``.
            dtype (Union[str, type], optional): expected data type of this
                parameter. Accepts all strings that are accepted by
                `numpy.dtype <https://numpy.org/doc/stable/reference/generated/numpy.dtype.html>`_ ,
                eg. ``int``, ``float``, ``uint16``, ``string``.

        Raises:
            TypeError: On a ``limits`` argument that was not tuple-like or if
                a ``limits`` argument was given but the ``default`` was a
            ValueError: if an invalid ``limits_mode`` is passed, if ``limits``
                and ``is_any_of`` are *both* passed, or if the ``limits``
                argument did not have length 2.
        """
        if limits_mode not in self.LIMIT_MODES:
            raise ValueError(f"Unrecognized `limits_mode` argument "
                             f"'{limits_mode}'! "
                             f"Choose from: {', '.join(self.LIMIT_MODES)}")

        if limits and is_any_of:
            raise ValueError("The arguments `limits` and `is_any_of` cannot "
                             "be specified at the same time!")

        if limits is not None:
            # Ensure tuple, correct length, and numerical default value
            if not isinstance(limits, (tuple, list)):
                raise TypeError("Expected a tuple-like `limits` argument, but "
                                f"got {type(limits).__name__} with "
                                f"value {limits}!")
            limits = tuple(limits)

            if len(limits) != 2:
                raise ValueError("`limits` argument should be of length 2! "
                                 f"Got: {limits}")

            if default is not None and not isinstance(default, Number):
                raise TypeError("Unable to use the `limits` argument for non-"
                                f"numerical default value '{default}'! Use "
                                "the `is_any_of` argument instead.")

        self._default = default
        self._name = name
        self._description = description
        self._limits = limits
        self._limits_mode = limits_mode
        self._is_any_of = tuple(is_any_of) if is_any_of else None
        self._dtype = np.dtype(dtype) if dtype is not None else None


    # .. Magic Methods ........................................................

    def __eq__(self, other) -> bool:
        if not isinstance(other, type(self)):
            return False
        return self.__dict__ == other.__dict__

    def __str__(self) -> str:
        info = {attr[1:]: getattr(self, attr)
                for attr in ('_name', '_is_any_of', '_dtype', '_limits')
                if getattr(self, attr) is not None}
        if info:
            if 'limits' in info:
                info['limits_mode'] = repr(self._limits_mode)
            info_str = ", ".join([f"{k}: {v}" for k, v in info.items()])
            return f"<Parameter, default: {self._default}, {info_str}>"
        return f"<Parameter, default: {self._default}>"

    # .. Properties ...........................................................

    @property
    def default(self):
        """The default value for this parameter"""
        return self._default

    @property
    def name(self):
        """The name of this parameter"""
        return self._name

    @property
    def description(self):
        """The description of this parameter"""
        return self._description

    @property
    def limits(self) -> Union[tuple, None]:
        """The limits of this parameter"""
        return self._limits

    @property
    def limits_mode(self) -> str:
        """The mode used when evaluating the limits"""
        return self._limits_mode

    @property
    def is_any_of(self) -> Tuple[Any]:
        """Possible values this parameter may assume"""
        return self._is_any_of

    @property
    def dtype(self) -> Union[np.dtype, None]:
        """The expected data type of this parameter"""
        return self._dtype

    # .........................................................................

    def validate(self, value: Any, *, raise_exc: bool=True) -> bool:
        """Checks whether the given value would be a valid parameter.

        The checks for the corresponding arguments are carried out in the
        following order:

            1. ``is_any_of``
            2. ``dtype``
            3. ``limits``

        The data type is checked according to the numpy type hierarchy, see
        `docs <https://numpy.org/doc/stable/reference/arrays.scalars.html>`__.
        To reduce strictness, the following additional compatibilities are
        taken into account:

            - for unsigned integer ``dtype``, a signed integer-type
              ``value`` is compatible if ``value >= 0``
            - for floating-point ``dtype``, integer-type ``value`` are always
              considered compatible
            - for floating-point ``dtype``, ``value`` of all floating-point-
              types are considered compatible, even if they have a lower
              precision (note the coercion test below, though)

        Additionally, it is checked whether ``value`` is representable as the
        target data type. This is done by coercing ``value`` to ``dtype`` and
        then checking for equality (using np.isclose).

        Args:
            value (Any): The value to test.
            raise_exc (bool, optional): Whether to raise an exception or not.

        Returns:
            bool: Whether or not the given value is a valid parameter.

        Raises:
            ValidationError: If validation failed or is impossible (for
                instance due to ambiguous validity parameters). This error
                message contains further information on why validation failed.
        """
        def was_invalid(msg: str) -> bool:
            """Handles the case where the given value was not valid.
            This helper function raises a ``ValidationError`` if ``raise_exc``
            flag is set, returns False otherwise.
            """
            if raise_exc:
                raise ValidationError(msg)
            return False

        # Check explicitly given permissible values first
        if self.is_any_of and value not in self.is_any_of:
            _valid_opts = ", ".join([repr(e) for e in self.is_any_of])
            return was_invalid(f"value {repr(value)} is not permissible. "
                               f"Valid options are:  {_valid_opts}")

        # Check the type
        if self.dtype:
            _is_subtype = np.issubdtype(type(value), self.dtype)

            _float_compatible = (
                np.issubdtype(self.dtype, np.floating) and
                (np.issubdtype(type(value), np.integer) or
                 np.issubdtype(type(value), np.floating)))
            _uint_compatible = (
                np.issubdtype(self.dtype, np.unsignedinteger) and
                np.issubdtype(type(value), np.integer) and value >= 0)

            if not (_is_subtype or _float_compatible or _uint_compatible):
                return was_invalid(
                    f"required {self.dtype}-compatible type, but got "
                    f"{type(value).__name__} with value {repr(value)}."
                )

            # For numerical values, check that type coercion would not lead to
            # value changes, e.g. due to values not being representable
            if (np.issubdtype(self.dtype, np.number) and
                np.issubdtype(type(value), np.number) and
                not np.isnan(value)
            ):
                _coerced_value = self.dtype.type(value)
                _is_equal = (_coerced_value == value)
                _is_close = np.isclose(_coerced_value, value)
                # NOTE This is done instead of np.can_cast, which would look
                #      only at the types and thus be too strict. By taking
                #      into account the actual value, we are more compatible.

                if not (_is_equal or _is_close):
                    return was_invalid(
                        f"the {type(value).__name__} value {repr(value)} "
                        "cannot be correctly represented as the required "
                        f"type, {self.dtype}, but would be coerced to "
                        f"{repr(_coerced_value)} != {repr(value)}"
                    )


        # Check value is within the given limits
        if self.limits:
            # Type should be numerical
            if not isinstance(value, Number):
                return was_invalid(
                    f"required numerical value in order to validate limits, "
                    f"but got {type(value).__name__} with value {repr(value)}."
                )

            # Can now assume that it is numerical. Change Nones to ±inf
            lims = list(self.limits)
            if lims[0] is None:
                lims[0] = -float('inf')
            if lims[1] is None:
                lims[1] = float('inf')

            # Get comparison operators according to limits mode
            comp_l = self.LIMIT_COMPS[self.limits_mode[0]]
            comp_r = self.LIMIT_COMPS[self.limits_mode[1]]

            if not (comp_l(value, lims[0]) and comp_r(value, lims[1])):
                # For ±inf bounds, adjust the shown interval to "open"
                return was_invalid(
                    "required value in interval "
                    f"{self.limits_mode[0] if not _isinf(lims[0]) else '('}"
                    f"{lims[0]}, {lims[1]}"
                    f"{self.limits_mode[1] if not _isinf(lims[1]) else ')'}, "
                    f"but got {repr(value)}."
                )

        # If this point is reached, all checks passed: Parameter is valid.
        return True

    # .. Class methods ........................................................

    @classmethod
    def from_shorthand(cls, value, *, mode, **kwargs):
        """Constructs a Parameter object from a given shorthand mode.

        Args:
            value: A given value, typically the ``default`` argument.
            mode: A valid shorthand mode, see
                :py:attr:`~utopya.parameter.Parameter.SHORTHAND_MODES`
            \**kwargs: any further arguments for Parameter ininitialization,
                see :py:meth:`~utopya.parameter.Parameter.__init__`.

        Returns:
            a Parameter object
        """
        if mode == 'is-probability':
            d = dict(default=value, limits=[0, 1], dtype=float, **kwargs)

        elif mode == 'is-positive':
            d = dict(default=value, limits=[0, None], limits_mode='(]',
                     **kwargs)

        elif mode == 'is-negative':
            d = dict(default=value, limits=[None, 0], limits_mode='[)',
                     **kwargs)

        elif mode == 'is-int':
            d = dict(default=value, dtype=int, **kwargs)

        elif mode == 'is-positive-int':
            d = dict(default=value, limits=[0, None], limits_mode = '()',
                     dtype=int, **kwargs)

        elif mode == 'is-negative-int':
            d = dict(default=value, limits=[None, 0], limits_mode = '()',
                     dtype=int, **kwargs)

        elif mode == 'is-bool':
            d = dict(default=value, dtype=bool, **kwargs)

        elif mode == 'is-string':
            d = dict(default=value, dtype=str, **kwargs)

        elif mode == 'is-unsigned':
            d = dict(default=value, limits=[0, None], limits_mode = '[)',
                     dtype='uint', **kwargs)
        else:
            raise ValueError(f"Unrecognized shorthand mode '{mode}'! Needs be "
                             f"one of: {', '.join(cls.SHORTHAND_MODES)}")

        return cls(**d)

    @classmethod
    def to_yaml(cls, representer, node):
        """Represent this Parameter object as a YAML mapping.

        Args:
            representer (ruamel.yaml.representer): The representer module
            node (type(self)): The node, i.e. an instance of this class

        Returns:
            a yaml mapping that is able to recreate this object
        """
        d = {}
        d['default'] = node.default
        if node.limits:
            d['limits'] = node.limits
            d['limits_mode'] = node.limits_mode
        elif node.is_any_of:
            d['is_any_of'] = node.is_any_of
        if node.dtype:
            d['dtype'] = str(node.dtype)
        return representer.represent_mapping(cls.yaml_tag, d)

    @classmethod
    def from_yaml(cls, constructor, node):
         """The default constructor for Parameter objects, expecting a YAML
         node that is mapping-like.
         """
         return cls(**constructor.construct_mapping(node, deep=True))


# -----------------------------------------------------------------------------


def extract_validation_objects(model_cfg: dict, *,
                               model_name: str) -> Tuple[dict, dict]:
    """Extracts all :py:class:`~utopya.parameter.Parameter` objects from a
    model configuration (a nested dict), replacing them with their default
    values.
    Returns both the modified model configuration well as the Parameter
    objects (keyed by the key sequence necessary to reach them within the
    model configuration).

    Args:
        model_cfg (dict): the model configuration to inspect
        model_name (str): the name of the model

    Returns:
        Tuple[dict, dict]: a tuple of (model config, parameters to validate).
            The model config contains the passed config dict in which all
            Parameter class elements have been replaced by their default
            entries.
            The second entry is a dictionary consisting of the Parameter class
            objects (requiring validation) with keys being key sequences to
            those Parameter objects. Note that the key sequence is relative to
            the level *above* the model configuration, with ``model_name`` as
            a common entry for all returned values.
    """
    # Collect Parameter objects
    is_Parameter_obj = lambda e: isinstance(e, Parameter)
    to_validate = psp.tools.recursive_collect(model_cfg,
                                              select_func=is_Parameter_obj,
                                              prepend_info=("keys",))

    # Replace by default values
    set_default_val = lambda e: e.default
    model_cfg = psp.tools.recursive_replace(model_cfg,
                                            select_func=is_Parameter_obj,
                                            replace_func=set_default_val)

    # Prepend the model name to the key sequence such that the key sequences
    # attach *one level above* the model configuration, i.e. at the level of
    # the `parameter_space`.
    to_validate = {(model_name,) + ks : v
                   for ks, v in dict(to_validate).items()}

    return model_cfg, to_validate
