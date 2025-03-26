/**
 * @file    DetectorActivityRatePlots.cpp
 * @brief   Draws some plots of detector activity with time. For simulation.
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    October 13, 2020
 * 
 * The configuration requires:
 * 
 * * `services` table with `Geometry`, `LArPropertiesService`,
 *   `DetectorClocksService` and `DetectorPropertiesService`
 * * `analysis` table configuring the job
 * 
 */

// local libraries
#include "Binner.h"

// SBN code
#include "icarusalg/Geometry/ICARUSstandaloneGeometrySetup.h"
#include "icarusalg/Geometry/WireReadoutGeomICARUS.h"
#include "icarusalg/Geometry/GeoObjectSorterPMTasTPC.h"
#include "icarusalg/Utilities/ROOTutils.h" // util::ROOT::TDirectoryChanger
#include "icarusalg/gallery/helpers/C++/expandInputFiles.h"

// LArSoft
// - data products
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"

// - DetectorProperties
#include "lardataalg/DetectorInfo/DetectorPropertiesStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/DetectorPropertiesStandard.h"
#include "lardataalg/DetectorInfo/DetectorPropertiesData.h"
// - DetectorClocks
#include "lardataalg/DetectorInfo/DetectorClocksStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/DetectorClocksStandard.h"
#include "lardataalg/DetectorInfo/DetectorClocksStandardDataFor.h"
#include "lardataalg/DetectorInfo/DetectorClocksData.h"
// - LArProperties
#include "lardataalg/DetectorInfo/LArPropertiesStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/LArPropertiesStandard.h"
// - Geometry
#include "larcorealg/Geometry/StandaloneGeometrySetup.h"
#include "larcorealg/Geometry/GeometryCore.h"

// - configuration
#include "larcorealg/Geometry/WireReadoutSorterStandard.h"
#include "larcorealg/Geometry/StandaloneBasicSetup.h"
// - utilities
#include "lardataalg/DetectorInfo/DetectorTimings.h"
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h" // simulation_time, ...
#include "lardataalg/Utilities/StatCollector.h"
#include "lardataalg/Utilities/intervals_fhicl.h" // microseconds from FHiCL
#include "lardataalg/Utilities/quantities/energy.h" // megaelectronvolt
#include "lardataalg/Utilities/quantities/electronics.h" // tick, ticks
#include "lardataalg/Utilities/quantities/spacetime.h" // microseconds, ...
#include "larcorealg/CoreUtils/enumerate.h"

// gallery/canvas
#include "gallery/Event.h"
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/ParameterSet.h"

// ROOT
#include "TFile.h"
#include "TDirectory.h"
#include "TProfile.h"

// C/C++ standard libraries
// #include <string>
#include <numeric> // std::accumulate()
#include <vector>
#include <array>
#include <memory> // std::make_unique()
#include <iostream> // std::cerr, std::endl
#include <limits>
#include <cmath> // std::ceil(), std::floor()


// #if !defined(__CLING__)
// // these are needed in the main() function
#include <algorithm> // std::copy()
#include <iterator> // std::back_inserter()
// #include <iostream> // std::cerr
// #endif // !__CLING__

// -----------------------------------------------------------------------------
// ---  Utilities
// -----------------------------------------------------------------------------
namespace {

  template <typename Time>
  using timetraitsOf_t
    = typename detinfo::timescales::timescale_traits<typename Time::category_t>;

  template <typename Time>
  constexpr std::string timeScaleName() { return timetraitsOf_t<Time>::name(); }

  template <typename DestTime, typename Stream, typename Time>
  void printConvertedTimeRange(
    Stream&& out, Time start, Time stop,
    detinfo::DetectorTimings const& detTimings
  ) {
    out << timeScaleName<DestTime>() << ": "
      << detTimings.toTimeScale<DestTime>(start) << " -- " 
      << detTimings.toTimeScale<DestTime>(stop)
      ;
  } // printConvertedTimeRange()

  template <typename DestTick, typename Stream, typename Time>
  void printConvertedTickRange(
    Stream&& out, Time start, Time stop,
    detinfo::DetectorTimings const& detTimings
  ) {
    out << timeScaleName<DestTick>()
      << " ticks: " << detTimings.toTick<DestTick>(start)
      << " -- " << detTimings.toTick<DestTick>(stop)
      ;
  } // printConvertedTickRange()

} // local namespace


// -----------------------------------------------------------------------------
// ---  algorithm classes
// -----------------------------------------------------------------------------

/**
 * @brief Produces plots. Aaah!
 * 
 * Algorithm workflow:
 * 
 * 1. construction
 * 2. `setup()`
 * 3. `prepare()`
 * 4. for each event:
 *     1. `setupEvent()`
 *     2. `plotEvent()`
 * 5. `finish()`
 * 
 */
class PlotDetectorActivityRates {
  
  using nanoseconds = util::quantities::intervals::nanoseconds;
  
  /// Time scale used for plotting of generation and particle level simulation.
  using simulation_time_scale
    = detinfo::timescales::timescale_traits<detinfo::timescales::SimulationTimeCategory>;
  
  using simulation_time = simulation_time_scale::time_point_t;
  using trigger_time = detinfo::timescales::simulation_time;
  using electronics_tick = detinfo::timescales::electronics_tick;
  
  /// Unit the energy depositions are stored in (LArSoft convention).
  using EDepUnit_t = util::quantities::megaelectronvolt;
  
  
  /// Data structure with all the configuration.
  struct AlgorithmConfiguration {
    template <typename Point>
    struct BinConfig {
      using Point_t = Point;
      using Interval_t = typename util::Binner<Point_t>::Step_t;
      Point_t start;
      Point_t stop;
      Interval_t step;
    };
    
    art::InputTag edepTag { "largeant", "TPCActive" };
    art::InputTag chanTag { "largeant" };
    art::InputTag photTag { "largeant" };
    
    BinConfig<simulation_time> simBinning;
    BinConfig<electronics_tick> TPCBinning;
    BinConfig<trigger_time> opDetBinning;
    
  }; // AlgorithmConfiguration
  
  
  // --- BEGIN -- Data members -------------------------------------------------
  
  // ----- BEGIN -- Configuration ----------------------------------------------
  
  /// Complete configuration of the algorithm.
  AlgorithmConfiguration const fConfig;
  
  util::Binner<simulation_time> fSimBinner;
  util::Binner<electronics_tick> fTPCBinner;
  util::Binner<trigger_time> fOpDetBinner;
  
  // ----- END -- Configuration ------------------------------------------------
  
  // ----- BEGIN -- Setup ------------------------------------------------------
  /// ROOT directory where to write the plots.
  TDirectory* fDestDir = nullptr;
  
  std::optional<detinfo::DetectorTimings> fDetTimings;
  std::optional<detinfo::DetectorPropertiesData> fDetPropsData;
  // ----- END -- Setup --------------------------------------------------------

  
  // ----- BEGIN -- Plots ------------------------------------------------------
  /// Average amount of deposited energy per time [GeV].
  std::unique_ptr<TProfile> fEDepDistrib;
  
  /// Average amount of ionization collected per time.
  std::unique_ptr<TProfile> fTPCchargeDistrib;
  
  /// Average number of photon per time.
  std::unique_ptr<TProfile> fPhotonDistrib;
  // ----- END -- Plots --------------------------------------------------------
  
  /// Statistics of the total energy per event.
  lar::util::StatCollector<double> fEDepStats;
  
  /// Statistics of the total charge (electron count) per event.
  lar::util::StatCollector<double> fTPCchargeStats;
  
  /// Statistics of the total light (photoelectron count) per event.
  lar::util::StatCollector<unsigned int> fPhotonStats;
  

  // --- END -- Data members ---------------------------------------------------
  
    public:
  
  /// Name of the recommended configuration table for this algorithm.
  static std::string const ConfigurationKey;
  
  struct FHiCLconfig {
    
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    
    using millisecond = util::quantities::points::millisecond;
    using milliseconds = util::quantities::intervals::milliseconds;
    using tick = util::quantities::points::tick;
    using ticks = util::quantities::intervals::ticks;
    
    template <typename Point, typename Interval>
    struct BinningConfig {
      fhicl::Atom<Point> Start {
        Name{ "Start" },
        Comment{ "start of the range" }
        };
      fhicl::Atom<Point> Stop {
        Name{ "Stop" },
        Comment{ "end of the range" }
        };
      fhicl::Atom<Interval> Step {
        Name{ "Step" },
        Comment{ "duration of steps in the range" }
        };
    }; // BinningConfig
    
    
    fhicl::Atom<art::InputTag> Deposits {
      Name{ "Deposits" },
      Comment{ "input tag for energy deposit data product" },
      art::InputTag{ "largeant", "TPCActive" }
      };
    fhicl::Atom<art::InputTag> TPCchannels {
      Name{ "TPCchannels" },
      Comment{ "input tag for simulated TPC channel data product" },
      "largeant"
      };
    fhicl::Atom<art::InputTag> OpDetChannels {
      Name{ "OpDetChannels" },
      Comment{ "input tag for simulated optical detector channel data product" },
      "largeant"
      };
    
    fhicl::Table<BinningConfig<millisecond, milliseconds>> SimBinning {
      Name{ "SimulationBinning" },
      Comment{ "range and binning for simulation times (simulation time) [ms]" }
      };
    
    fhicl::Table<BinningConfig<tick, ticks>> TPCBinning {
      Name{ "TPCBinning" },
      Comment{ "range and binning for TPC readout [electronics ticks]" }
      };
    
    fhicl::Table<BinningConfig<millisecond, milliseconds>> OpDetBinning {
      Name{ "OpDetBinning" },
      Comment{ "range and binning for optical detector simulation (beam gate time)" }
      };
    
  }; // FHiCLconfig
  
  using Parameters = fhicl::Table<FHiCLconfig>;
  
  
  /// Constructor: reads the configuration from the specified parameters set.
  PlotDetectorActivityRates(Parameters const& config);
  
//   /// Constructor: reads the configuration from the specified parameters set.
//   PlotDetectorActivityRates(fhicl::ParameterSet const& pset);
  
  
  /// Prints on `out` screen a configuration summary.
  static void printConfigurationHelp(std::ostream& out);
  
  
  /**
   * @brief Sets the algorithm up.
   * @param pDestDir ROOT output directory for the plots
   * @param clocksData detector clocks information (event-independent)
   * @param propsData detector properties information (event-independent)
   */
  void setup(
    TDirectory* pDestDir,
    detinfo::DetectorClocksData&& clocksData,
    detinfo::DetectorPropertiesData&& propsData
    );
  
  /// Performs the initialization of the algorithm.
  void prepare();
  
  /// Set up for a specific event.
  void setupEvent(
    detinfo::DetectorClocksData&& clocksData,
    detinfo::DetectorPropertiesData&& propsData
    );
  
  /// Processes a single event.
  template <typename Event>
  void plotEvent(Event const& event);
  
  /// Completes and saves the plots.
  void finish();
  
  
  /// Prints the current configuration to the specified output stream.
  template <typename Stream>
  void printConfig(Stream&& out) const;
  
  /// Prints some information about configured timing.
  template <typename Stream>
  void printTimingSummary(Stream&& out) const;
  
    private:
  
  
  // --- BEGIN -- Configuration ------------------------------------------------
  
  AlgorithmConfiguration parseAlgorithmConfiguration
    (fhicl::ParameterSet const& pset);
  
  AlgorithmConfiguration parseValidatedAlgorithmConfiguration
    (FHiCLconfig const& config);
  
  AlgorithmConfiguration parseValidatedAlgorithmConfiguration
    (fhicl::ParameterSet const& pset)
    { return parseValidatedAlgorithmConfiguration(Parameters{pset}()); }
  
  template <typename ConfigOut>
  static void parseBinning
    (ConfigOut& binConfig, fhicl::ParameterSet const& pset);

  template <typename ConfigOut, typename ConfigIn>
  static void parseAndValidateBinning
    (ConfigOut& binConfig, ConfigIn const& config);
  
  template <typename T, typename BinConfig>
  static util::Binner<T> makeBinning(BinConfig const& config)
    { return { config.start, config.stop, config.step }; }
  
  template <typename T>
  static void readParam
    (fhicl::ParameterSet const& pset, std::string const& key, T& var)
    { var = pset.get<T>(key); }
  
  // --- END -- Configuration --------------------------------------------------


  // --- BEGIN -- Plot management ----------------------------------------------
  
  /// Performs the initialization of the plots filled by the algorithm.
  void initializePlots();
  
  /// Performs the initialization of plots pertaining energy deposits.
  void initializeEnergyDepositPlots();
  
  /// Performs the initialization of plots pertaining collected TPC charge.
  void initializeTPCionizationPlots();
  
  /// Performs the initialization of plots pertaining collected photoelectrons.
  void initializePhotonPlots();
  
  /// Writes all plots into the current ROOT directory (and then deletes them).
  void savePlots();
  
  /// Prints on screen some collected statistics.
  void printStats() const;
  
  // --- END -- Plot initialization --------------------------------------------
  
  
  // --- BEGIN -- Plots --------------------------------------------------------
  
  /// Plots data from energy deposits in liquid argon.
  void plotEnergyDeposits(std::vector<sim::SimEnergyDeposit> const& energyDeps);
  
  /// Plots data from photoelectron collection.
  void plotTPCionization(std::vector<sim::SimChannel> const& TPCchannels);
  
  /// Plots data from photoelectron collection.
  void plotPhotons(std::vector<sim::SimPhotons> const& photonChannels);
  
  // --- END -- Plots ----------------------------------------------------------
  
  
  /// Writes `plot` into the current ROOT directory, then deletes it.
  template <typename Plot>
  static void Serialize(std::unique_ptr<Plot>& plot);
  
}; // PlotDetectorActivityRates


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string const PlotDetectorActivityRates::ConfigurationKey { "plot" };



PlotDetectorActivityRates::PlotDetectorActivityRates
  (Parameters const& config)
  : fConfig{ parseValidatedAlgorithmConfiguration(config()) }
  , fSimBinner{ makeBinning<simulation_time>(fConfig.simBinning) }
  , fTPCBinner{ makeBinning<electronics_tick>(fConfig.TPCBinning) }
  , fOpDetBinner{ makeBinning<trigger_time>(fConfig.opDetBinning) }
  {}


auto PlotDetectorActivityRates::parseAlgorithmConfiguration
  (fhicl::ParameterSet const& pset) -> AlgorithmConfiguration
{
  AlgorithmConfiguration algConfig;
  algConfig.edepTag = pset.get<art::InputTag>("Deposits", "largeant");
  algConfig.chanTag = pset.get<art::InputTag>("TPCchannels", "largeant");
  algConfig.photTag = pset.get<art::InputTag>("OpDetChannels", "largeant");
  
  parseBinning
    (algConfig.simBinning, pset.get<fhicl::ParameterSet>("SimBinning"));
  parseBinning
    (algConfig.TPCBinning, pset.get<fhicl::ParameterSet>("TPCBinning"));
  parseBinning
    (algConfig.opDetBinning, pset.get<fhicl::ParameterSet>("OpDetBinning"));
  
  return algConfig;
} // PlotDetectorActivityRates::parseAlgorithmConfiguration()


auto PlotDetectorActivityRates::parseValidatedAlgorithmConfiguration
  (FHiCLconfig const& config) -> AlgorithmConfiguration
{
  AlgorithmConfiguration algConfig;
  algConfig.edepTag = config.Deposits();
  algConfig.chanTag = config.TPCchannels();
  algConfig.photTag = config.OpDetChannels();
  parseAndValidateBinning(algConfig.simBinning, config.SimBinning());
  parseAndValidateBinning(algConfig.TPCBinning, config.TPCBinning());
  parseAndValidateBinning(algConfig.opDetBinning, config.OpDetBinning());
  return algConfig;
} // PlotDetectorActivityRates::parseValidatedAlgorithmConfiguration()




template <typename ConfigOut>
void PlotDetectorActivityRates::parseBinning
  (ConfigOut& binConfig, fhicl::ParameterSet const& pset)
{
  readParam(pset, "Start", binConfig.start);
  readParam(pset, "Stop", binConfig.stop);
  readParam(pset, "Step", binConfig.step);
} // PlotDetectorActivityRates::parseBinning()


template <typename ConfigOut, typename ConfigIn>
void PlotDetectorActivityRates::parseAndValidateBinning
  (ConfigOut& binConfig, ConfigIn const& config)
{
  binConfig.start = config.Start();
  binConfig.stop = config.Stop();
  binConfig.step = config.Step();
} // PlotDetectorActivityRates::parseAndValidateBinning()


void PlotDetectorActivityRates::printConfigurationHelp(std::ostream& out) {
  
  out << "Configuration for the analysis algorithm:\n";
  Parameters const table
    { fhicl::Name{ PlotDetectorActivityRates::ConfigurationKey } };
  table.print_allowed_configuration(out);
  out << std::endl;
  
} // PlotDetectorActivityRates::printConfigurationHelp()


void PlotDetectorActivityRates::setup(
  TDirectory* pDestDir,
  detinfo::DetectorClocksData&& clocksData,
  detinfo::DetectorPropertiesData&& propsData
) {
  
  fDestDir = pDestDir;
  
  fDetTimings.emplace(std::move(clocksData));
  fDetPropsData.emplace(std::move(propsData));
  
} // PlotDetectorActivityRates::setup()


void PlotDetectorActivityRates::prepare() {
  
  initializePlots();
  
} // PlotDetectorActivityRates::prepare()


void PlotDetectorActivityRates::initializePlots() {
  
  initializeEnergyDepositPlots();
  initializeTPCionizationPlots();
  initializePhotonPlots();
  
} // PlotDetectorActivityRates::initializePlots()


void PlotDetectorActivityRates::initializeEnergyDepositPlots() {
  
  fEDepDistrib = std::make_unique<TProfile>(
    "EnergyDepositsInTime",
    (
      "Energy deposited in active volume vs. time"
      ";deposition time (simulation time scale) [ "
        + simulation_time::unit_t::symbol() + " ]"
      ";" + EDepUnit_t::unit_t::symbol() +" / event  [ /"
        + to_string(fSimBinner.step()) + " ]"
    ).c_str(),
    fSimBinner.nBins(), fSimBinner.lower().value(), fSimBinner.upper().value()
    );
  
} // PlotDetectorActivityRates::initializeEnergyDepositPlots()


void PlotDetectorActivityRates::initializeTPCionizationPlots() {
  assert(fDetTimings); // setup() should have taken care of these already
  assert(fDetPropsData);
  
  detinfo::timescales::TPCelectronics_tick const TPCstart { 0 },
    TPCstop { fDetPropsData->ReadOutWindowSize() };
  
  fTPCchargeDistrib = std::make_unique<TProfile>(
    "TPCchargeInTime",
    (
      "Electrons sensed by TPC channels vs. time (readout window: "
        + to_string(fDetTimings->toTick<electronics_tick>(TPCstart)) + " -- "
        + to_string(fDetTimings->toTick<electronics_tick>(TPCstop)) + ")"
      ";observation time (electronics time scale) [ TPC ticks, "
        + to_string(fDetTimings->ClockPeriodFor<electronics_tick>()) + " ]"
      ";ionization electrons / event  [ /" + to_string(fTPCBinner.step()) + " ]"
    ).c_str(),
    fTPCBinner.nBins(), fTPCBinner.lower().value(), fTPCBinner.upper().value()
    );
  
} // PlotDetectorActivityRates::initializeTPCionizationPlots()


void PlotDetectorActivityRates::initializePhotonPlots() {
  
  fPhotonDistrib = std::make_unique<TProfile>(
    "PhotoelectronsInTime",
    (
      "Photoelectrons detected vs. time"
      ";PMT conversion time (simulation time scale) [ "
        + simulation_time::unit_t::symbol() + " ]"
      ";photons / event  [ /" + to_string(fOpDetBinner.step()) + " ]"
    ).c_str(),
    fOpDetBinner.nBins(),
    fOpDetBinner.lower().value(), fOpDetBinner.upper().value()
    );
  
} // PlotDetectorActivityRates::initializePhotonPlots()


void PlotDetectorActivityRates::setupEvent(
  detinfo::DetectorClocksData&& clocksData,
  detinfo::DetectorPropertiesData&& propsData
) {
  // if multithreading were needed, this should be merged with `plotEvent()`
  // and made constant; ROOT histograms are not thread-safe anyway.
  fDetTimings.emplace(std::move(clocksData));
  fDetPropsData.emplace(std::move(propsData));
} // PlotDetectorActivityRates::setupEvent()


template <typename Event>
void PlotDetectorActivityRates::plotEvent(Event const& event) {
  assert(fDetTimings); // setupEvent() should have taken care of this
  
  //
  // energy depositions
  //
  auto const& energyDeps
    = *(event.template getValidHandle<std::vector<sim::SimEnergyDeposit>>
      (fConfig.edepTag));
  
  plotEnergyDeposits(energyDeps);
  
  //
  // TPC charge
  //
  auto const& TPCchannels
    = *(event.template getValidHandle<std::vector<sim::SimChannel>>
      (fConfig.chanTag));
  
  plotTPCionization(TPCchannels);
  
  //
  // photons
  //
  auto const& photonChannels
    = *(event.template getValidHandle<std::vector<sim::SimPhotons>>
      (fConfig.photTag));
  
  plotPhotons(photonChannels);
  
} // PlotDetectorActivityRates::plotEvent()


void PlotDetectorActivityRates::finish() {
  
  savePlots();
  
  printStats();
  
} // PlotDetectorActivityRates::finish()


template <typename Stream>
void PlotDetectorActivityRates::printConfig(Stream&& out) const {
  
  out << "PlotDetectorActivityRates algorithm using:"
    << "\n * simulated energy deposits: " << fConfig.edepTag.encode()
    << "\n * simulated electrons:       " << fConfig.chanTag.encode()
    << "\n * simulated photons:         " << fConfig.photTag.encode()
    << "\n * time binning for:          "
    << "\n   - simulation:              " << fSimBinner
    << "\n   - TPC:                     " << fTPCBinner
    << "\n   - optical detectors:       " << fOpDetBinner
    << "\n"
    ;
  
} // PlotDetectorActivityRates::printConfig()


template <typename Stream>
void PlotDetectorActivityRates::printTimingSummary(Stream&& out) const {
  assert(fDetTimings); // it should have been taken care by setup()
  assert(fDetPropsData); // it should have been taken care by setup()
  
  detinfo::timescales::TPCelectronics_tick const TPCstart { 0 },
    TPCstop { fDetPropsData->ReadOutWindowSize() };
  
  out << "Relevant timing settings:"
    << "\n * TPC readout window: ";
  printConvertedTickRange<detinfo::timescales::electronics_tick>
    (out << "\n   - ", TPCstart, TPCstop, *fDetTimings);
  printConvertedTimeRange<detinfo::timescales::electronics_time>
    (out << "\n   - ", TPCstart, TPCstop, *fDetTimings);
  printConvertedTimeRange<detinfo::timescales::simulation_time>
    (out << "\n   - ", TPCstart, TPCstop, *fDetTimings);
  out << "\n";
  
} // PlotDetectorActivityRates::printConfig()


void PlotDetectorActivityRates::plotEnergyDeposits
  (std::vector<sim::SimEnergyDeposit> const& energyDeps)
{
  // funny fact: this method modifies the plot content but does not modify the
  // plot object pointer, so we can declare it const.
  
  // shifted by 1 ([0] is underflow, ROOT standard)
  std::vector<EDepUnit_t> counters(fSimBinner.nBins() + 2U, EDepUnit_t{ 0.0 });
  
  // all channels are aggregated together
  for (sim::SimEnergyDeposit const& edep: energyDeps) {
    
    simulation_time const time { edep.Time() };
    
    int const timeBin = fSimBinner.cappedBinWithOverflows(time);
    assert(timeBin + 1 < counters.size());
    
    EDepUnit_t const energy { edep.Energy() }; // explicit with the unit
    
    counters[timeBin + 1] += energy;
    
  } // for channels
  
  for (auto const [ iCount, count ]: util::enumerate(counters))
    fEDepDistrib->Fill(fSimBinner.binCenter(iCount - 1).value(), count.value());
  
  
  EDepUnit_t const totalE
    = std::accumulate(counters.cbegin(), counters.cend(), EDepUnit_t{ 0.0 });
  fEDepStats.add(totalE.value());
  
  mf::LogVerbatim("PlotDetectorActivityRates")
    << "Collected " << totalE << " in " << energyDeps.size() << " deposits.";

} // PlotDetectorActivityRates::plotEnergyDeposits()


void PlotDetectorActivityRates::plotTPCionization
  (std::vector<sim::SimChannel> const& TPCchannels)
{
  // funny fact: this method modifies the plot content but does not modify the
  // plot object pointer, so we can declare it const.

  // shifted by 1 ([0] is underflow, ROOT standard)
  std::vector<double> counters(fTPCBinner.nBins() + 2U, 0U);
  
  // all channels are aggregated together
  for (sim::SimChannel const& channel: TPCchannels) {
    
    for (auto const& [ TDC, IDEs ]: channel.TDCIDEMap()) {
      
      // the TDC is filled by `LArVoxelReadout` with
      // clockData.TPCClock().Ticks(clockData.G4ToElecTime(time))
      electronics_tick const tick { TDC };
      
      int const tickBin = fTPCBinner.cappedBinWithOverflows(tick);
      assert(tickBin + 1 < counters.size());
      
      counters[tickBin + 1] += channel.Charge(TDC);
      
    } // for all TDC
    
  } // for channels
  
  for (auto const [ iCount, count ]: util::enumerate(counters))
    fTPCchargeDistrib->Fill(fTPCBinner.binCenter(iCount - 1).value(), count);
  
  double const totalElectrons
    = std::accumulate(counters.cbegin(), counters.cend(), 0.0);
  fTPCchargeStats.add(totalElectrons);
  mf::LogVerbatim("PlotDetectorActivityRates")
    << "Detected " << totalElectrons
    << " electrons in " << TPCchannels.size() << " channels (all planes)."
    ;

} // PlotDetectorActivityRates::plotTPCionization()


void PlotDetectorActivityRates::plotPhotons
  (std::vector<sim::SimPhotons> const& photonChannels)
{
  // funny fact: this method modifies the plot content but does not modify the
  // plot object pointer, so we can declare it const.
  
  // shifted by 1 ([0] is underflow, ROOT standard)
  std::vector<unsigned int> counters(fOpDetBinner.nBins() + 2U, 0U);
  
  // all channels are aggregated together
  for (sim::SimPhotons const& photons: photonChannels) {
    
    for (sim::OnePhoton const& photon: photons) {
      
      simulation_time const time { photon.Time };
      
      int const timeBin
        = fOpDetBinner.cappedBinWithOverflows(fDetTimings->toTriggerTime(time));
      assert(timeBin + 1 < counters.size());
      
      ++counters[timeBin + 1];
      
    } // for photons in channel
    
  } // for channels
  
  for (auto const [ iCount, count ]: util::enumerate(counters))
    fPhotonDistrib->Fill(fOpDetBinner.binCenter(iCount - 1).value(), count);
  
  unsigned int const totalPhotons
    = std::accumulate(counters.cbegin(), counters.cend(), 0U);
  fPhotonStats.add(totalPhotons);
  
  mf::LogVerbatim("PlotDetectorActivityRates")
    << "Collected " << totalPhotons
    << " photoelectrons in " << photonChannels.size() << " channels."
    ;
  
} // PlotDetectorActivityRates::plotPhotons()


void PlotDetectorActivityRates::savePlots() {
  
  if (!fDestDir) return;
    
  util::ROOT::TDirectoryChanger DirGuard(fDestDir);
  
  Serialize(fEDepDistrib);
  Serialize(fTPCchargeDistrib);
  Serialize(fPhotonDistrib);
  
  
} // PlotDetectorActivityRates::savePlots()


void PlotDetectorActivityRates::printStats() const {
  
  mf::LogVerbatim("PlotDetectorActivityRates")
    <<   "Statistics, on average per event (" << fEDepStats.N() << " events):"
    << "\n * )" << fEDepStats.Average() << " +/- " << fEDepStats.RMS()
      << ") " << EDepUnit_t::unitSymbol() << " of deposited energy"
    << "\n * (" << fTPCchargeStats.Average() << " +/- " << fTPCchargeStats.RMS()
      << ") ionization electrons (all planes)"
    << "\n * " << fPhotonStats.Average() << " +/- " << fPhotonStats.RMS()
      << ") photoelectrons"
    ;
  
} // PlotDetectorActivityRates::printStats()


template <typename Plot>
void PlotDetectorActivityRates::Serialize(std::unique_ptr<Plot>& plot) {
  if (!plot) return;
  plot->Write();
  plot.reset();
} // PlotDetectorActivityRates::Serialize()


// -----------------------------------------------------------------------------


/**
 * @brief Runs the analysis macro.
 * @param configFile path to the FHiCL configuration to be used for the services
 * @param inputFiles vector of path of file names
 * @return an integer as exit code (0 means success)
 */
int makePlots
  (std::string const& configFile, std::vector<std::string> const& inputFiles)
{
  /*
   * the "test" environment configuration
   */
  // read FHiCL configuration from a configuration file:
  fhicl::ParameterSet config = lar::standalone::ParseConfiguration(configFile);

  // set up message facility (always picked from "services.message")
  lar::standalone::SetupMessageFacility(config, "");

  // ***************************************************************************
  // ***  SERVICE PROVIDER SETUP BEGIN  ****************************************
  // ***************************************************************************
  //
  // Uncomment the things you need
  // (and make sure the corresponding headers are also uncommented)
  //
  // geometry setup (it's special)
  auto geom = lar::standalone::SetupGeometry<icarus::GeoObjectSorterPMTasTPC>
    (config.get<fhicl::ParameterSet>("services.Geometry"));
    
  // wire readout (it's even more special)
  auto wireReadout = lar::standalone::SetupReadout
    <geo::WireReadoutSorterStandard, icarus::WireReadoutGeomICARUS>
    (config.get<fhicl::ParameterSet>("services.WireReadout"), geom.get())
    ;
  
  // LArProperties setup
  auto larProp = testing::setupProvider<detinfo::LArPropertiesStandard>
    (config.get<fhicl::ParameterSet>("services.LArPropertiesService"));

  // DetectorClocks setup
  auto detClocks = testing::setupProvider<detinfo::DetectorClocksStandard>
    (config.get<fhicl::ParameterSet>("services.DetectorClocksService"));

  // DetectorProperties setup
  auto detProps = testing::setupProvider<detinfo::DetectorPropertiesStandard>(
    config.get<fhicl::ParameterSet>("services.DetectorPropertiesService"),
    detinfo::DetectorPropertiesStandard::providers_type{geom.get(),wireReadout.get(), larProp.get()}
//     { geom.get(), larProp.get() } // FIXME try this simpler version
    );

  // ***************************************************************************
  // ***  SERVICE PROVIDER SETUP END    ****************************************
  // ***************************************************************************

  auto const& analysisConfig = config.get<fhicl::ParameterSet>("analysis");
  
  // event loop options
  constexpr auto NoLimits = std::numeric_limits<unsigned int>::max();
  unsigned int nSkip = analysisConfig.get("skipEvents", 0U);
  unsigned int const maxEvents = analysisConfig.get("maxEvents", NoLimits);

  /*
   * the preparation of input file list
   */
  if (inputFiles.size() != 1) {
    throw std::runtime_error("Support for multiple input files not implemented yet!");
  }
  std::vector<std::string> const allInputFiles = expandInputFiles(inputFiles);

  /*
   * other parameters
   */

  /*
   * preparation of histogram output file
   */
  std::unique_ptr<TFile> pHistFile;
  if (analysisConfig.has_key("histogramFile")) {
    std::string fileName = analysisConfig.get<std::string>("histogramFile");
    mf::LogVerbatim("makePlots")
      << "Creating output file: '" << fileName << "'" << std::endl;
    pHistFile = std::make_unique<TFile>(fileName.c_str(), "RECREATE");
  }

  /*
   * preparation of the algorithm class
   */
  
  // configuration from `ConfigurationKey` table of configuration file:
  PlotDetectorActivityRates plotAlg {
    analysisConfig.get<fhicl::ParameterSet>
      (PlotDetectorActivityRates::ConfigurationKey)
    };
  
  plotAlg.printConfig(mf::LogVerbatim{"makePlots"});
  
  {
    auto clocksData = detClocks->DataForJob();
    plotAlg.setup
      (pHistFile.get(), std::move(clocksData), detProps->DataFor(clocksData));
  }
  plotAlg.printTimingSummary(mf::LogVerbatim{"makePlots"});
  
  plotAlg.prepare();
  
  /*
  auto const clock_data = detclk->DataForJob();
  auto const det_prop_data = detp->DataFor(clock_data);
  */
  
  unsigned int numEvents { 0U };

  /*
   * the event loop
   */
  for (gallery::Event event(allInputFiles); !event.atEnd(); event.next()) {
    
    if (nSkip > 0) { --nSkip; continue; }
    if (numEvents >= maxEvents) {
      mf::LogVerbatim("makePlots") << "Maximum number of events reached.";
      break;
    }
    
    // *************************************************************************
    // ***  SINGLE EVENT PROCESSING BEGIN  *************************************
    // *************************************************************************

    ++numEvents;
    {
      mf::LogVerbatim log("makePlots");
      log << "This is event " << event.fileEntry() << "-" << event.eventEntry()
        << " (" << numEvents;
      if (maxEvents < NoLimits) log << "/" << maxEvents;
      log << ")";
    }
    
    {
      auto clocksData
        = detinfo::detectorClocksStandardDataFor(*detClocks, event);
      plotAlg.setupEvent(std::move(clocksData), detProps->DataFor(clocksData));
    }
    
    plotAlg.plotEvent(event);
    

    // *************************************************************************
    // ***  SINGLE EVENT PROCESSING END    *************************************
    // *************************************************************************

  } // for

  plotAlg.finish();
  plotAlg.printTimingSummary(mf::LogVerbatim{"makePlots"} << "Once again:\n");
  
  return 0;
} // makePlots()


/// Version with a single input file.
int makePlots(std::string const& configFile, std::string filename)
  { return makePlots(configFile, std::vector<std::string>{ filename }); }

#if !defined(__CLING__)
int main(int argc, char** argv) {
  
  char **pParam = argv + 1, **pend = argv + argc;
  if (pParam == pend) {
    std::cerr << "Usage: " << argv[0] << "  configFile [inputFile ...]"
      << std::endl;
    PlotDetectorActivityRates::printConfigurationHelp(std::cerr);
    return 1;
  }
  std::string const configFile = *(pParam++);
  std::vector<std::string> fileNames;
  std::copy(pParam, pend, std::back_inserter(fileNames));
  
  return makePlots(configFile, fileNames);
} // main()

#endif // !__CLING__
