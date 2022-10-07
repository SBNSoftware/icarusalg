////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterICARUS.cxx
/// \brief Interface to algorithm class for sorting standard geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "icarusalg/Geometry/GeoObjectSorterICARUS.h"
#include "icarusalg/Geometry/details/AuxDetSorting.h"

#include "larcorealg/Geometry/AuxDetGeo.h"
#include "larcorealg/Geometry/AuxDetSensitiveGeo.h"
#include "larcorealg/Geometry/CryostatGeo.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"

namespace geo{

  //----------------------------------------------------------------------------
  // Define sort order for cryostats in standard configuration
  static bool sortCryoStandard(const CryostatGeo& c1, const CryostatGeo& c2)
  {
    double xyz1[3] = {0.}, xyz2[3] = {0.};
    double local[3] = {0.};
    c1.LocalToWorld(local, xyz1);
    c2.LocalToWorld(local, xyz2);

    return xyz1[0] < xyz2[0];
  }


  //----------------------------------------------------------------------------
  // Define sort order for tpcs in standard configuration.
  static bool sortTPCStandard(const TPCGeo& t1, const TPCGeo& t2)
  {
    double xyz1[3] = {0.};
    double xyz2[3] = {0.};
    double local[3] = {0.};
    t1.LocalToWorld(local, xyz1);
    t2.LocalToWorld(local, xyz2);

    // sort TPCs according to x
    if(xyz1[0] < xyz2[0]) return true;

    return false;
  }

  const double EPSILON = 0.000001;

  //----------------------------------------------------------------------------
  // Define sort order for planes in standard configuration
  static bool sortPlaneStandard(const PlaneGeo& p1, const PlaneGeo& p2)
  {
    double xyz1[3] = {0.};
    double xyz2[3] = {0.};
    double local[3] = {0.};
    p1.LocalToWorld(local, xyz1);
    p2.LocalToWorld(local, xyz2);

    //if the planes are in the same drift coordinate, lower Z is first plane
    if( std::abs(xyz1[0] - xyz2[0]) < EPSILON)
      return xyz1[2] < xyz2[2];

    //else
    // drift direction is negative, plane number increases in drift direction
    return xyz1[0] > xyz2[0];
  }


  //----------------------------------------------------------------------------
  static bool sortWireStandard(WireGeo const& w1, WireGeo const& w2){
    auto const [xyz1, xyz2] = std::pair{w1.GetCenter(), w2.GetCenter()};

    //we have horizontal wires...
    if( std::abs(xyz1[2]-xyz2[2]) < EPSILON)
      return xyz1[1] < xyz2[1];

    //in the other cases...
    return xyz1[2] < xyz2[2];
  }

  //----------------------------------------------------------------------------
  GeoObjectSorterICARUS::GeoObjectSorterICARUS(fhicl::ParameterSet const& p)
  {
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterICARUS::SortAuxDets(std::vector<geo::AuxDetGeo> & adgeo) const
  {
    icarus::SortAuxDetsStandard(adgeo);
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterICARUS::SortAuxDetSensitive(std::vector<geo::AuxDetSensitiveGeo> & adsgeo) const
  {
    icarus::SortAuxDetSensitiveStandard(adsgeo);
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterICARUS::SortCryostats(std::vector<geo::CryostatGeo> & cgeo) const
  {
    std::sort(cgeo.begin(), cgeo.end(), sortCryoStandard);
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterICARUS::SortTPCs(std::vector<geo::TPCGeo>  & tgeo) const
  {
    std::sort(tgeo.begin(), tgeo.end(), sortTPCStandard);
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterICARUS::SortPlanes(std::vector<geo::PlaneGeo> & pgeo,
                                           geo::DriftDirection_t  const driftDir) const
  {
    // sort the planes to increase in drift direction
    // The drift direction has to be set before this method is called.  It is set when
    // the CryostatGeo objects are sorted by the CryostatGeo::SortSubVolumes method
    if     (driftDir == geo::kPosX) std::sort(pgeo.rbegin(), pgeo.rend(), sortPlaneStandard);
    else if(driftDir == geo::kNegX) std::sort(pgeo.begin(),  pgeo.end(),  sortPlaneStandard);
    else if(driftDir == geo::kUnknownDrift)
      throw cet::exception("TPCGeo") << "Drift direction is unknown, can't sort the planes\n";
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterICARUS::SortWires(std::vector<geo::WireGeo> & wgeo) const
  {
    std::sort(wgeo.begin(), wgeo.end(), sortWireStandard);
  }

}
