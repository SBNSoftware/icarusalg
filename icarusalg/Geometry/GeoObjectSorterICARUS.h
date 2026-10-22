////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterICARUS.h
/// \brief Interface to algorithm class for standard sorting of geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  wketchum@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_GEOOBJECTSORTERICARUS_H
#define GEO_GEOOBJECTSORTERICARUS_H

#include "larcorealg/Geometry/GeoObjectSorter.h"

#include "fhiclcpp/fwd.h"

namespace geo{

  class GeoObjectSorterICARUS : public GeoObjectSorter {
  public:
    explicit GeoObjectSorterICARUS(fhicl::ParameterSet const&);

  private:
    bool compareCryostats(CryostatGeo const& c1, CryostatGeo const& c2) const override;
    bool compareTPCs(TPCGeo const& t1, TPCGeo const& t2) const override;
  };

}

#endif // GEO_GEOOBJECTSORTERICARUS_H
