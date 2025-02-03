/**
 * @file icarusalg/Geometry/GeoObjectSorterPMTasTPC.cxx
 * @brief  Geometry object sorter with PMT following TPC wire order.
 * @date   April 26, 2020
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see    icarusalg/Geometry/GeoObjectSorterPMTasTPC.h
 */

// library header
#include "icarusalg/Geometry/GeoObjectSorterPMTasTPC.h"

// LArSoft header
#include "larcorealg/Geometry/OpDetGeo.h"

bool icarus::GeoObjectSorterPMTasTPC::compareOpDets(geo::OpDetGeo const& od1,
                                                    geo::OpDetGeo const& od2) const
{
  auto const [c1, c2] = std::pair{od1.GetCenter(), od2.GetCenter()};
  if (fCmpX(c1, c2)) return c1.X() < c2.X();
  if (fCmpZ(c1, c2)) return c1.Z() < c2.Z();
  return c1.Y() < c2.Y();
}
