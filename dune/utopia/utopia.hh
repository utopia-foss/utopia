#ifndef UTOPIA_HH
#define UTOPIA_HH

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
#include "utopia_dune.hh"

// UTOPIA
#include "types.hh"
#include "neighborhoods.hh"
#include "state.hh"
#include "tags.hh"
#include "entity.hh"
// #include "cell.hh"
 #include "agent.hh"
#include "grid.hh"
#include "data.hh"
#include "data_vtk.hh"
#ifdef HAVE_PSGRAF
    #include "data_eps.hh"
#endif // HAVE_PSGRAF
#include "simulation.hh"
#include "setup.hh"

#endif // UTOPIA_HH
