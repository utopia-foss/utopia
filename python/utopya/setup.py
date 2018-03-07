#!/usr/bin/env python3
"""Setups the utopya package, its test dependencies, and a script."""

from setuptools import setup

# Dependency lists
INSTALL_DEPS = ['numpy>=1.13', 'PyYAML>=3.12', 'paramspace>=1.0b']
TEST_DEPS    = ['pytest>=3.4.0', 'pytest-cov>=2.5.1']

setup(name='utopya',
      description='The Utopia frontend package.',
      url='https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia',
      classifiers=[
          'Programming Language :: Python :: 3.6',
          'Topic :: Utilities'
      ],
      packages=['utopya'],
      include_package_data=True,
      install_requires=INSTALL_DEPS,
      tests_require=TEST_DEPS,
      test_suite='py.test',
      extras_require=dict(test_deps=TEST_DEPS),
      scripts=['bin/utopia'] # installs this into the virtual env!
      )