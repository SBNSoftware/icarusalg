////////////////////////////////////////////////////////////////////////
/// \file  AuxDetGeoObjectSorterICARUS.h
/// \brief Interface to algorithm class for standard sorting of geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  wketchum@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_AUXDETGEOOBJECTSORTERICARUS_H
#define GEO_AUXDETGEOOBJECTSORTERICARUS_H

#include "larcorealg/Geometry/AuxDetGeoObjectSorter.h"

#include "fhiclcpp/fwd.h"

namespace geo{

  class AuxDetGeoObjectSorterICARUS : public AuxDetGeoObjectSorter {
  public:
    explicit AuxDetGeoObjectSorterICARUS(fhicl::ParameterSet const&);

  private:
    bool compareAuxDets(AuxDetGeo const& ad1, AuxDetGeo const& ad2) const override;
    bool compareAuxDetSensitives(AuxDetSensitiveGeo const& ad1,
                                 AuxDetSensitiveGeo const& ad2) const override;
  };

}

#endif // GEO_AUXDETGEOOBJECTSORTERICARUS_H
