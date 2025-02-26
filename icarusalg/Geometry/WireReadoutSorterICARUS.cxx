/**
 * @file  icarusalg/Geometry/WireReadoutSorterICARUS.cxx
 * @brief Interface to algorithm class for sorting standard geo::XXXGeo objects
 */

#include "icarusalg/Geometry/WireReadoutSorterICARUS.h"

#include "larcorealg/Geometry/WireGeo.h"

namespace {
  constexpr double EPSILON = 0.000001;
}

namespace geo {

  WireReadoutSorterICARUS::WireReadoutSorterICARUS() = default;
  WireReadoutSorterICARUS::WireReadoutSorterICARUS(fhicl::ParameterSet const&) {}

  //----------------------------------------------------------------------------
  bool WireReadoutSorterICARUS::compareWires(WireGeo const& w1, WireGeo const& w2) const {
    auto const& xyz1 = w1.GetCenter();
    auto const& xyz2 = w2.GetCenter();

    //we have horizontal wires...
    if( std::abs(xyz1.Z()-xyz2.Z()) < EPSILON)
      return xyz1.Y() < xyz2.Y();

    //in the other cases...
    return xyz1.Z() < xyz2.Z();
  }

}
