"""Sets up the utopya package, test dependencies, and command line scripts"""

from setuptools import setup, find_packages

# Dependency lists
INSTALL_DEPS = ['numpy>=1.13',
                'PyYAML>=3.12,<4.0',
                # From private repositories:
                'paramspace>=1.0.1',
                'dantro>=0.1.0-rc.5'  # TODO use 0.1.0 once released
                ]
TEST_DEPS    = ['pytest>=3.4.0', 'pytest-cov>=2.5.1']

setup(name='utopya',
      #
      # Package information
      version='0.1.0-pre.0',
      # NOTE This needs to correspond to utopya.__init__.__version__
      description='The Utopia frontend package.',
      url='https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia',
      classifiers=[
          'Programming Language :: Python :: 3.6',
          'Topic :: Utilities'
      ],
      #
      # Package content and dependencies
      packages=find_packages(exclude=["*.test", "*.test.*", "test.*", "test"]),
      package_data=dict(utopya=["cfg/*.yml"]),
      install_requires=INSTALL_DEPS,
      tests_require=TEST_DEPS,
      test_suite='py.test',
      extras_require=dict(test_deps=TEST_DEPS),
      #
      # Command line scripts, installed into the virtual environment
      scripts=['bin/utopia']
      )
