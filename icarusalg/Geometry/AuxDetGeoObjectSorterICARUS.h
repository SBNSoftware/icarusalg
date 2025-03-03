/**
 * @file  icarusalg/Geometry/AuxDetGeoObjectSorterICARUS.h
 * @brief Interface to algorithm class for standard sorting of geo::XXXGeo objects
 *
 * @author Chris Hilgenberg
 */
#ifndef ICARUSALG_GEOMETRY_AUXDETGEOOBJECTSORTERICARUS_H
#define ICARUSALG_GEOMETRY_AUXDETGEOOBJECTSORTERICARUS_H

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

#endif // ICARUSALG_GEOMETRY_AUXDETGEOOBJECTSORTERICARUS_H
