"""Sets up the utopya package, test dependencies, and command line scripts"""

from setuptools import setup, find_packages

# Dependency lists
INSTALL_DEPS = ['numpy>=1.13',
                'coloredlogs>=10.0',
                'ruamel.yaml',
                # only required for testing
                'pytest>=3.4.0',
                'pytest-cov>=2.5.1',
                # From private repositories:
                # NOTE Versions need also be set in python/CMakeLists.txt
                'paramspace>=2.0.0rc6', # TODO use release version once ready
                'dantro>=0.4.0rc3'      # TODO use release version once ready
                ]

setup(name='utopya',
      #
      # Package information
      version='0.1.0-pre.2',
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
      test_suite='py.test',
      #
      # Command line scripts, installed into the virtual environment
      scripts=['bin/utopia']
      )
