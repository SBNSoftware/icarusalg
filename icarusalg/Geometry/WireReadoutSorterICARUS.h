/**
 * @file  icarusalg/Geometry/WireReadoutSorterICARUS.h
 * @brief Interface to algorithm class for standard sorting of geo::XXXGeo objects
 */
#ifndef ICARUSALG_GEOMETRY_WIREREADOUTSORTERICARUS_H
#define ICARUSALG_GEOMETRY_WIREREADOUTSORTERICARUS_H

#include "fhiclcpp/fwd.h"

#include "larcorealg/Geometry/WireReadoutSorter.h"

namespace geo {

  class WireReadoutSorterICARUS : public WireReadoutSorter {
  public:
    WireReadoutSorterICARUS();
    explicit WireReadoutSorterICARUS(fhicl::ParameterSet const&);

  private:
    bool compareWires(WireGeo const& a, WireGeo const& b) const override;
  };

}

#endif // ICARUSALG_GEOMETRY_WIREREADOUTSORTERICARUS_H
