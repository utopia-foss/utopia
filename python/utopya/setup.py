"""Sets up the utopya package, test dependencies, and command line scripts"""

from setuptools import setup, find_packages

# Dependency lists
INSTALL_DEPS = [
    'numpy>=1.13',
    'scipy>=1.1.0',
    'matplotlib>=3.1',
    'coloredlogs>=10.0',
    'ruamel.yaml',
    
    # Required for testing:
    'pytest>=3.4.0',
    'pytest-cov>=2.5.1',

    # From private repositories:
    'paramspace>=2.2.3',
    'dantro>=0.9.1'
    # NOTE Versions need also be set in python/CMakeLists.txt
]

setup(
    name='utopya',
    #
    # Package information
    version='0.5.0',
    # NOTE This needs to correspond to utopya.__init__.__version__
    # TODO single-source this
    description='The Utopia Frontend Package',
    url='https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia',
    author='TS-CCEES Utopia Developers',
    author_email=('Benjamin Herdeanu '
                  '<Benjamin.Herdeanu@iup.uni-heidelberg.de>, '
                  'Yunus Sevinchan '
                  '<Yunus.Sevinchan@iup.uni-heidelberg.de>'),
    classifiers=[
        'Programming Language :: Python :: 3.6',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)',
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
