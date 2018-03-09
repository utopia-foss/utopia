#ifndef UTOPIA_HH
#define UTOPIA_BASE_HH

// STL containers
#include <vector>
#include <array>
#include <initializer_list>
#include <map>
#include <bitset>

// I/O
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>

// Utility
#include <memory>
#include <chrono>
#include <type_traits>
#include <random>
#include <algorithm>
#include <functional>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

// DUNE HEADERS
#pragma GCC system_header // disable DUNE internal warnings

// DUNE-COMMON
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>
#include <dune/common/fmatrix.hh>
#include <dune/common/timer.hh>

// DUNE-GRID
#include <dune/grid/yaspgrid.hh>
#include <dune/grid/uggrid.hh>
#include <dune/grid/io/file/gmshreader.hh>
#include <dune/grid/utility/structuredgridfactory.hh>
#include <dune/grid/common/mcmgmapper.hh>
#include <dune/grid/io/file/vtk/vtksequencewriter.hh>

// // UTOPIA I/O
// #include "data_io/config.hh"

#endif // UTOPIA_BASE_HH
