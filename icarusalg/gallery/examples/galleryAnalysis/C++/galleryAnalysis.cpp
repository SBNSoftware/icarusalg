/**
 * @file    galleryAnalysis.cpp
 * @brief   Template analysis program based on gallery.
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    October 21, 2017
 * 
 * The script is an adaptation of the official gallery demo script demo.cc at
 * https://github.com/marcpaterno/gallery-demo .
 * 
 * To jump into the action, look for `SERVICE PROVIDER SETUP` and
 * `SINGLE EVENT PROCESSING` tags in the source code.
 * 
 * The approach for loading services is the lowest level LArSoft provides.
 * An higher level one is to use `testing::TesterEnvironment` as in some service
 * provider unit tests (e.g., `geo::GeometryCore` and `detinfo::LArProperties`).
 * 
 */

// our additional code
#include "TrackAnalysis.h"
#include "HitAnalysisAlg.h"
#include "MCAssociations.h"

// ICARUS code
#include "icarusalg/Geometry/ICARUSstandaloneGeometrySetup.h"
#include "icarusalg/Geometry/WireReadoutGeomICARUS.h"
#include "icarusalg/Geometry/GeoObjectSorterPMTasTPC.h"
#include "icarusalg/gallery/helpers/C++/expandInputFiles.h"

// LArSoft
// - data products
#include "lardataobj/RecoBase/Track.h"
// - DetectorProperties
#include "lardataalg/DetectorInfo/DetectorPropertiesStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/DetectorPropertiesStandard.h"
// - DetectorClocks
#include "lardataalg/DetectorInfo/DetectorClocksStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/DetectorClocksStandard.h"
// - LArProperties
#include "lardataalg/DetectorInfo/LArPropertiesStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/LArPropertiesStandard.h"
// - Geometry
#include "larcorealg/Geometry/StandaloneGeometrySetup.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcorealg/Geometry/WireReadoutGeom.h"
#include "icarusalg/Geometry/WireReadoutGeomICARUS.h"
// - configuration
#include "larcorealg/Geometry/WireReadoutSorterStandard.h"
#include "larcorealg/Geometry/StandaloneBasicSetup.h"

// gallery/canvas
#include "gallery/Event.h"
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"

// ROOT
#include "TFile.h"

// C/C++ standard libraries
#include <string>
#include <vector>
#include <memory> // std::make_unique()
#include <iostream> // std::cerr


#if !defined(__CLING__)
// these are needed in the main() function
#include <algorithm> // std::copy()
#include <iterator> // std::back_inserter()
#include <iostream> // std::cerr
#endif // !__CLING__

/**
 * @brief Runs the analysis macro.
 * @param configFile path to the FHiCL configuration to be used for the services
 * @param inputFiles vector of path of file names
 * @return an integer as exit code (0 means success)
 */
int galleryAnalysis(std::string const& configFile, std::vector<std::string> const& inputFiles)
{
    /*
     * the "test" environment configuration
     */
    // read FHiCL configuration from a configuration file:
    fhicl::ParameterSet config = lar::standalone::ParseConfiguration(configFile);
  
    // set up message facility (always picked from "services.message")
    lar::standalone::SetupMessageFacility(config, "galleryAnalysis");
  
    // configuration from the "analysis" table of the FHiCL configuration file:
    auto const& analysisConfig = config.get<fhicl::ParameterSet>("analysis");
  
    // ***************************************************************************
    // ***  SERVICE PROVIDER SETUP BEGIN  ****************************************
    // ***************************************************************************
    //
    // Uncomment the things you need
    // (and make sure the corresponding headers are also uncommented)
    //
  
    // geometry setup (it's special)
    auto geom = lar::standalone::SetupGeometry<icarus::GeoObjectSorterPMTasTPC>(config.get<fhicl::ParameterSet>("services.Geometry"));
    
  // wire readout (it's even more special)
    auto wireReadout = lar::standalone::SetupReadout<geo::WireReadoutSorterStandard, icarus::WireReadoutGeomICARUS>
      (config.get<fhicl::ParameterSet>("services.WireReadout"), geom.get());
  
    // LArProperties setup
    auto larp = testing::setupProvider<detinfo::LArPropertiesStandard>(config.get<fhicl::ParameterSet>("services.LArPropertiesService"));
  
    // DetectorClocks setup
    auto detclk = testing::setupProvider<detinfo::DetectorClocksStandard>(config.get<fhicl::ParameterSet>("services.DetectorClocksService"));
  
    // DetectorProperties setup
    auto detp = testing::setupProvider<detinfo::DetectorPropertiesStandard>(config.get<fhicl::ParameterSet>("services.DetectorPropertiesService"),
                                                                            detinfo::DetectorPropertiesStandard::providers_type{geom.get(), wireReadout.get(), larp.get() });
  
    // ***************************************************************************
    // ***  SERVICE PROVIDER SETUP END    ****************************************
    // ***************************************************************************
  
    /*
     * the preparation of input file list
     */
    std::vector<std::string> const allInputFiles = expandInputFiles(inputFiles);
  
    /*
     * other parameters
     */
    art::InputTag trackTag = analysisConfig.get<art::InputTag>("tracks");
    art::InputTag hitsTag  = analysisConfig.get<art::InputTag>("hits");
  
    /*
     * preparation of histogram output file
     */
    std::unique_ptr<TFile> pHistFile;
    if (analysisConfig.has_key("histogramFile"))
    {
        std::string fileName = analysisConfig.get<std::string>("histogramFile");
        std::cout << "Creating output file: '" << fileName << "'" << std::endl;
        pHistFile = std::make_unique<TFile>(fileName.c_str(), "RECREATE");
    }
  
    /*
     * preparation of the algorithm class
     */
    TrackAnalysis trackAnalysis(analysisConfig.get<fhicl::ParameterSet>("trackAnalysis"));
  
    trackAnalysis.setup(*geom, pHistFile.get());
    trackAnalysis.prepare();
    
    HitAnalysis::HitAnalysisAlg hitAnalysisAlg(analysisConfig.get<fhicl::ParameterSet>("hitAnalysisAlg"));
    
    hitAnalysisAlg.setup(*wireReadout, pHistFile.get());
    
    MCAssociations mcAssociations(analysisConfig.get<fhicl::ParameterSet>("mcAssociations"));
    auto const detProp = detp->DataFor(detclk->DataForJob());
    mcAssociations.setup(*geom, *wireReadout, detProp, pHistFile.get());
    mcAssociations.prepare();
    
    int numEvents(0);
  
    /*
     * the event loop
     */
    for (gallery::Event event(allInputFiles); !event.atEnd(); event.next())
    {
        // *************************************************************************
        // ***  SINGLE EVENT PROCESSING BEGIN  *************************************
        // *************************************************************************
    
        mf::LogVerbatim("galleryAnalysis") << "This is event " << event.fileEntry() << "-" << event.eventEntry();
    
        trackAnalysis.processTracks(*(event.getValidHandle<std::vector<recob::Track>>(trackTag)));
        
        hitAnalysisAlg.fillHistograms(*(event.getValidHandle<std::vector<recob::Hit>>(hitsTag)));
        
        mcAssociations.doTrackHitMCAssociations(event);

        numEvents++;
    
        // *************************************************************************
        // ***  SINGLE EVENT PROCESSING END    *************************************
        // *************************************************************************
    
    } // for
  
    trackAnalysis.finish();
    mcAssociations.finish();
    
    hitAnalysisAlg.endJob(numEvents);
  
    return 0;
} // galleryAnalysis()


/// Version with a single input file.
int galleryAnalysis(std::string const& configFile, std::string filename)
  { return galleryAnalysis(configFile, std::vector<std::string>{ filename }); }

#if !defined(__CLING__)
int main(int argc, char** argv) {
  
  char **pParam = argv + 1, **pend = argv + argc;
  if (pParam == pend) {
    std::cerr << "Usage: " << argv[0] << "  configFile [inputFile ...]"
      << std::endl;
    return 1;
  }
  std::string const configFile = *(pParam++);
  std::vector<std::string> fileNames;
  std::copy(pParam, pend, std::back_inserter(fileNames));
  
  return galleryAnalysis(configFile, fileNames);
} // main()

#endif // !__CLING__
