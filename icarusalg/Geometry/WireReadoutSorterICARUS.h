////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterICARUS.h
/// \brief Interface to algorithm class for standard sorting of geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  wketchum@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_WIREREADOUTSORTERICARUS_H
#define GEO_WIREREADOUTSORTERICARUS_H

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

#endif // GEO_GEOOBJECTSORTERICARUS_H
