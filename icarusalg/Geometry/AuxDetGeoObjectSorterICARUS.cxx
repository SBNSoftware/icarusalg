////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterICARUS.cxx
/// \brief Interface to algorithm class for sorting standard geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "icarusalg/Geometry/AuxDetGeoObjectSorterICARUS.h"

#include "larcorealg/Geometry/AuxDetGeo.h"
#include "larcorealg/Geometry/AuxDetSensitiveGeo.h"

#include <string>

namespace geo{

  //----------------------------------------------------------------------------
  AuxDetGeoObjectSorterICARUS::AuxDetGeoObjectSorterICARUS(fhicl::ParameterSet const&)
  {
  }

  //----------------------------------------------------------------------------
  bool AuxDetGeoObjectSorterICARUS::compareAuxDets(AuxDetGeo const& ad1, AuxDetGeo const& ad2) const
  {
    std::string type1 = "", type2 = "";
    switch (ad1.NSensitiveVolume()) {
        case 20 : type1 = "MINOS"; break;
        case 16 : type1 = "CERN"; break;
        case 64 : type1 = "DC"; break;
    }
    switch (ad2.NSensitiveVolume()) {
        case 20 : type2 = "MINOS"; break;
        case 16 : type2 = "CERN"; break;
        case 64 : type2 = "DC"; break;
    }

    // sort based off of GDML name, module number
    std::string ad1name = (ad1.TotalVolume())->GetName();
    std::string ad2name = (ad2.TotalVolume())->GetName();
    // assume volume name is "volAuxDet<type>module###<region>"
    std::string base1 = "volAuxDet"+type1+"module";
    std::string base2 = "volAuxDet"+type2+"module";

    //keep compatibility with legacy g4
    ad1name.erase(std::remove(ad1name.begin(), ad1name.end(), '_'), ad1name.end());
    ad2name.erase(std::remove(ad2name.begin(), ad2name.end(), '_'), ad2name.end());

    int ad1Num = std::atoi( ad1name.substr( base1.size(), 3).c_str() );
    int ad2Num = std::atoi( ad2name.substr( base2.size(), 3).c_str() );

    return ad1Num < ad2Num;
  }

  //----------------------------------------------------------------------------
  bool AuxDetGeoObjectSorterICARUS::compareAuxDetSensitives(AuxDetSensitiveGeo const& ad1,
                                                            AuxDetSensitiveGeo const& ad2) const
  {
    std::string type1 = "", type2 = "";

    // sort based off of GDML name, assuming ordering is encoded
    std::string ad1name = (ad1.TotalVolume())->GetName();
    std::string ad2name = (ad2.TotalVolume())->GetName();

    if ( ad1name.find("MINOS") != std::string::npos ) type1 = "MINOS";
    if ( ad1name.find("CERN") != std::string::npos ) type1 = "CERN";
    if ( ad1name.find("DC") != std::string::npos ) type1 = "DC";
    if ( ad2name.find("MINOS") != std::string::npos ) type2 = "MINOS";
    if ( ad2name.find("CERN") != std::string::npos ) type2 = "CERN";
    if ( ad2name.find("DC") != std::string::npos ) type2 = "DC";

    // assume volume name is "volAuxDetSensitive<type>module###strip##"
    std::string baseMod1 = "volAuxDetSensitive"+type1+"module";
    std::string baseStr1 = "volAuxDetSensitive"+type1+"module###strip";
    std::string baseMod2 = "volAuxDetSensitive"+type2+"module";
    std::string baseStr2 = "volAuxDetSensitive"+type2+"module###strip";

    //keep compatibility with legacy g4
    ad1name.erase(std::remove(ad1name.begin(), ad1name.end(), '_'), ad1name.end());
    ad2name.erase(std::remove(ad2name.begin(), ad2name.end(), '_'), ad2name.end());

    int ad1Num = atoi( ad1name.substr( baseMod1.size(), 3).c_str() );
    int ad2Num = atoi( ad2name.substr( baseMod2.size(), 3).c_str() );

    if(ad1Num!=ad2Num) return ad1Num < ad2Num;

    ad1Num = std::atoi( ad1name.substr( baseStr1.size(), 2).c_str() );
    ad2Num = std::atoi( ad2name.substr( baseStr2.size(), 2).c_str() );


    return ad1Num < ad2Num;
  }

}
