////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterICARUS.cxx
/// \brief Interface to algorithm class for sorting standard geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "icarusalg/Geometry/GeoObjectSorterICARUS.h"

#include "larcorealg/Geometry/CryostatGeo.h"
#include "larcorealg/Geometry/TPCGeo.h"

namespace geo{

  //----------------------------------------------------------------------------
  GeoObjectSorterICARUS::GeoObjectSorterICARUS(fhicl::ParameterSet const&)
  {
  }

  //----------------------------------------------------------------------------
  bool GeoObjectSorterICARUS::compareCryostats(CryostatGeo const& c1, CryostatGeo const& c2) const
  {
    auto const xyz1 = c1.GetCenter();
    auto const xyz2 = c2.GetCenter();

    return xyz1.X() < xyz2.X();
  }

  //----------------------------------------------------------------------------
  bool GeoObjectSorterICARUS::compareTPCs(TPCGeo const& t1, TPCGeo const& t2) const
  {
    auto const xyz1 = t1.GetCenter();
    auto const xyz2 = t2.GetCenter();

    // sort TPCs according to x
    return xyz1.X() < xyz2.X();
  }

}
