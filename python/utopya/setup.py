"""Sets up the utopya package, test dependencies, and command line scripts"""

from setuptools import setup, find_packages

# Dependency lists
INSTALL_DEPS = [
    'numpy>=1.13',
    'scipy>=1.1.0',
    'matplotlib>=3.1',
    'coloredlogs>=10.0',
    'ruamel.yaml>=0.16.5',

    'paramspace>=2.3',
    'dantro~=0.11',  # NOTE Version need also be set in python/CMakeLists.txt
    # See https://www.python.org/dev/peps/pep-0440/#compatible-release for
    # details on the compatible release clause.

    # Required for testing:
    'pytest>=3.4.0',
    'pytest-cov>=2.5.1',
    'psutil>=5.6.7'
]

# .............................................................................

# A function to extract version number from __init__.py
def find_version(*file_paths) -> str:
    """Tries to extract a version from the given path sequence"""
    import os, re, codecs

    def read(*parts):
        """Reads a file from the given path sequence, relative to this file"""
        here = os.path.abspath(os.path.dirname(__file__))
        with codecs.open(os.path.join(here, *parts), 'r') as fp:
            return fp.read()

    # Read the file and match the __version__ string
    file = read(*file_paths)
    match = re.search(r"^__version__\s?=\s?['\"]([^'\"]*)['\"]", file, re.M)
    if match:
        return match.group(1)
    raise RuntimeError("Unable to find version string in " + str(file_paths))

# .............................................................................

setup(
    name='utopya',
    #
    # Package information
    version=find_version('utopya', '__init__.py'),
    #
    description='The Utopia Frontend Package',
    url='https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia',
    author='TS-CCEES Utopia Developers',
    author_email=('Benjamin Herdeanu '
                  '<Benjamin.Herdeanu@iup.uni-heidelberg.de>, '
                  'Yunus Sevinchan '
                  '<Yunus.Sevinchan@iup.uni-heidelberg.de>'),
    classifiers=[
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
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
