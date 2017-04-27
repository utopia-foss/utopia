#ifndef CITCAT_DUNE_HH
#define CITCAT_DUNE_HH

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
#include <dune/grid/common/universalmapper.hh>
#include <dune/grid/io/file/vtk/vtksequencewriter.hh>

#endif // CITCAT_DUNE_HH