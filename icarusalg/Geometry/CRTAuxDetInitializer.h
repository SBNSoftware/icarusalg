#ifndef ICARUS_GEOMETRY_CRTAUXDETINITIALIZER_H
#define ICARUS_GEOMETRY_CRTAUXDETINITIALIZER_H

#include "larcorealg/Geometry/AuxDetGeo.h"
#include "larcorealg/Geometry/AuxDetSensitiveGeo.h"
#include "larcorealg/Geometry/AuxDetGeometryCore.h"

namespace icarus::crt {

  class CRTAuxDetInitializer : public geo::AuxDetInitializer {
  public:
    explicit CRTAuxDetInitializer(fhicl::ParameterSet const&);

  private:
    geo::AuxDetReadoutInitializers
    initialize(std::vector<geo::AuxDetGeo> const& adgeo) const override;
  };

} // namespace icarus::crt

#endif // ICARUS_GEOMETRY_CRTAUXDETINITIALIZER_H
