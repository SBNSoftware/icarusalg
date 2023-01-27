/**
 * @file    DrawPMTwaveforms.cpp
 * @brief   Draws all waveforms in the events.
 * @author  Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date    March 8, 2022
 * 
 * The configuration requires:
 *  * `analysis` table configuring the job
 * 
 * Run the executable with no parameters for FHiCL configuration description
 * 
 */

// SBN code
#include "icarusalg/Utilities/ROOTutils.h" // util::ROOT::TDirectoryChanger
#include "icarusalg/gallery/helpers/C++/expandInputFiles.h"

// LArSoft
// - data products
#include "lardataobj/RawData/TriggerData.h" // raw::Trigger
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/Simulation/BeamGateInfo.h"
// - configuration
#include "larcorealg/Geometry/StandaloneBasicSetup.h"
// - utilities
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h" // simulation_time, ...
#include "lardataalg/Utilities/intervals_fhicl.h" // nanoseconds from FHiCL
#include "lardataalg/Utilities/quantities/spacetime.h" // nanoseconds, ...
#include "lardataalg/Utilities/StatCollector.h"
#include "larcorealg/CoreUtils/enumerate.h"
#include "larcorealg/CoreUtils/counter.h"

// gallery/canvas
#include "gallery/Event.h"
#include "canvas/Persistency/Provenance/EventID.h"
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/types/OptionalDelegatedParameter.h"
#include "fhiclcpp/types/OptionalSequence.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/TableAs.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/ParameterSet.h"

// ROOT
#include "TCanvas.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TDirectoryFile.h"
#include "TH1.h"
#include "TGraph.h"
#include "TLine.h"
#include "TROOT.h" // gROOT

// C/C++ standard libraries
#include <string>
#include <numeric> // std::reduce(), std::transform_reduce()
#include <vector>
#include <map>
#include <optional>
#include <memory> // std::make_unique()
#include <iostream> // std::cerr, std::endl
#include <limits>
#include <type_traits> // std::void_t
#include <cmath> // std::round()


#if !defined(__CLING__)
// // these are needed in the main() function
#include <algorithm> // std::copy()
#include <iterator> // std::back_inserter()
#endif // !__CLING__


using namespace util::quantities::time_literals;

using optical_time = detinfo::timescales::optical_time;


// -----------------------------------------------------------------------------
// ---  Utilities
// -----------------------------------------------------------------------------

// --- BEGIN -- Simple numeric algorithms --------------------------------------
namespace {
  
  /// Collects statistics, minimum and maximum, with reduced interface.
  template <typename T, typename W = T>
  class StatCollectorWithMinMaxAndMedian
    : public lar::util::StatCollector<T,W>, public lar::util::MinMaxCollector<T>
  {
    std::vector<T> fData; // weights are ignored
    
      public:
    
    using Data_t = T;
    using Weight_t = W;
    using StatCollector_t = lar::util::StatCollector<Data_t, Weight_t>;
    using MinMaxCollector_t = lar::util::MinMaxCollector<Data_t>;
    
    void add(Data_t value, Weight_t weight = { 1 })
      {
        StatCollector_t::add(value, weight);
        MinMaxCollector_t::add(value);
        fData.push_back(value);
      }
    
    void clear()
      { StatCollector_t::clear(); MinMaxCollector_t::clear(); fData.clear(); }
    
    Data_t median() { return extractMedian(fData); }
    
    Data_t median() const
      { std::vector<Data_t> data{ fData }; return extractMedian(data); }
    
      private:
    Data_t extractMedian(std::vector<Data_t>& data) const
      {
        std::sort(data.begin(), data.end());
        std::size_t const middle = data.size()/2;
        return (data.size() % 2 == 0) // deliberately throws if no data
          ? (data.at(middle - 1) + data.at(middle)) / 2: data.at(middle);
      }
    
  }; // StatCollectorWithMinMaxAndMedian()
  
  
  template <typename Iter>
  auto median(Iter begin, Iter end) {
    
    using value_type = typename Iter::value_type;
    
    // need a copy of the data to be sorted:
    std::vector<value_type> data { begin, end };
    auto const middle = data.begin() + data.size() / 2;
    std::nth_element(data.begin(), middle, data.end());
    return *middle;
    
  } // median


  template <typename T, typename Iter>
  auto sum(Iter begin, Iter end)
    { return std::reduce(begin, end, T{}); }

  template <typename Iter>
  auto sum(Iter begin, Iter end)
    { return sum<typename Iter::value_type>(begin, end); }


  template <typename T, typename Iter>
  auto sumsq(Iter begin, Iter end)
    { return std::transform_reduce(begin, end, begin, T{}); }

  template <typename Iter>
  auto sumsq(Iter begin, Iter end)
    { return sumsq<typename Iter::value_type>(begin, end); }


  template <typename T, typename Iter>
  std::pair<T, T> averageAndVariance(Iter begin, Iter end) {
    T const N = std::distance(begin, end);
    T const average = sum<T>(begin, end)/N;
    return { average, sumsq<T>(begin, end)/N - average * average };
  } // averageAndVariance()
  
  template <typename Iter>
  auto averageAndVariance(Iter begin, Iter end)
    { return averageAndVariance<typename Iter::value_type>(begin, end); }
  
} // local namespace


// -----------------------------------------------------------------------------
template <typename T>
class ValueRange {
    public:
  using value_type = T;
  using boundary_type = std::optional<value_type>;
  
  struct NoLimit_t {};
  static constexpr NoLimit_t no_limit {};
  
  ValueRange(
    std::optional<value_type> lower = std::nullopt,
    std::optional<value_type> upper = std::nullopt
    )
    : fLower(lower), fUpper{ upper } {}
  template <typename U>
  ValueRange(ValueRange<U> const& from)
    : ValueRange{ boundary_type{ from.lower() }, boundary_type{ from.upper() }}
    {}
  
  std::optional<value_type> lower() const { return fLower; }
  std::optional<value_type> upper() const { return fUpper; }
  
  /// Returns whether `v` is contained in this range.
  bool contains(value_type v) const
    { return (!fLower || (v >= *fLower)) && (!fUpper || (v < *fUpper)); }
  
    private:
  boundary_type fLower, fUpper;
  
}; // ValueRange

template <typename T>
std::ostream& operator<< (std::ostream& out, ValueRange<T> const& range) {
  if (range.lower() && range.upper())
    out << "[ " << *(range.lower()) << " ; " << *(range.upper()) << " [";
  else if (range.lower())
    out << " [ " << *(range.lower()) << " ... ]";
  else if (range.upper())
    out << " [ ... " << *(range.upper()) << " [";
  else out << "{ any }";
  return out;
} // operator<< (ValueRange)


/// Lookup of settings by channel number.
template <typename Setting>
class HWSettingMap {
    public:
  using Setting_t = Setting; ///< Type held by this setting map.
  
  /// Returns whether a value is available.
  bool contains(raw::Channel_t channel) const
    { return fValues.find(channel) != fValues.end(); } // C++20: `contains()`
  
  /// Returns the value for the specified `channel`.
  /// @throw std::out_of_range if not found
  Setting_t const& operator() (raw::Channel_t channel) const
    { return fValues.at(channel); }
  
  /// Returns a copy of the value for the specified `channel`, or `defVal`.
  Setting_t operator() (raw::Channel_t channel, Setting_t defVal) const
    {
      auto const it = fValues.find(channel);
      return (it == fValues.end())? defVal: it->second;
    }
  
  /// Returns a pointer to the value for the specified `channel` or `nullptr`.
  Setting_t const* get(raw::Channel_t channel) const
    {
      auto const it = fValues.find(channel);
      return (it == fValues.end())? nullptr: &(it->second);
    }
  
  /// Sets or replaces a `value` for `channel`.
  void set(raw::Channel_t channel, Setting_t value)
    { fValues[channel] = std::move(value); }
  
    private:
  
  std::map<raw::Channel_t, Setting_t> fValues; ///< All settings.
  
}; // HWSettingMap<>



// --- END ---- Simple numeric algorithms --------------------------------------


// --- BEGIN -- FHiCL interfaces -----------------------------------------------
template <typename T>
struct ValueRangeFHiCL {
  using value_type = T;
  
  fhicl::OptionalAtom<value_type> Lower
    { fhicl::Name{ "Lower" }, fhicl::Comment{ "lower limit (included)" } };
  fhicl::OptionalAtom<value_type> Upper
    { fhicl::Name{ "Upper" }, fhicl::Comment{ "upper limit (excluded)" } };
    
}; // ValueRangeFHiCL

template <typename Q>
std::enable_if_t< // make sure that Q is a time point
  (
    util::quantities::concepts::is_point<Q>::value
    && Q::template category_compatible_with<util::quantities::points::microsecond>()
  ),
  ValueRange<optical_time>
  >
convert(ValueRangeFHiCL<Q> const& config)
  { return ValueRange<optical_time>{ config.Lower(), config.Upper() }; }

ValueRange<double> convert(ValueRangeFHiCL<double> const& config)
  { return ValueRange<double>{ config.Lower(), config.Upper() }; }


// --- END ---- FHiCL interfaces -----------------------------------------------


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
 *     1. `analyze()`
 * 5. `finish()`
 * 
 */
class DrawPMTwaveforms {
  
  using microsecond = util::quantities::points::microsecond;
  
  using microseconds = util::quantities::intervals::microseconds;
  using nanoseconds = util::quantities::intervals::nanoseconds;
  
  /// Data structure with all the configuration.
  struct AlgorithmConfiguration {
    
    struct BaselineConfig {
      bool subtract = false;
      
      unsigned int nSamples = 0;
      
      bool doPrint = false; ///< Whether to print the value on screen.
    }; // BaselineConfig
    
    
    
    art::InputTag waveformTag { "daqPMT" };
    
    art::InputTag triggerTag { "daqTrigger" };
    
    unsigned int nChannels { 360U };
    
    BaselineConfig baseline;
    
    /// Override whether to have all plots on the same ADC scale or not.
    std::optional<bool> sharedADCrange;
    
    std::vector<ValueRange<optical_time>> plotTimes;
    
    /// Configured baselines per channel.
    HWSettingMap<raw::ADC_Count_t> readoutBaselines;
    /// Configured thresholds per channel.
    HWSettingMap<raw::ADC_Count_t> readoutThresholds;
    
    float staggerFraction = 0.0;
    
    nanoseconds tickDuration;
    
  }; // AlgorithmConfiguration
  
  
  // --- BEGIN -- Data members -------------------------------------------------
  
  // ----- BEGIN -- Configuration ----------------------------------------------
  
  /// Complete configuration of the algorithm.
  AlgorithmConfiguration const fConfig;
  
  // ----- END -- Configuration ------------------------------------------------
  
  // ----- BEGIN -- Setup ------------------------------------------------------
  TDirectory* fDestDir = nullptr; ///< ROOT directory where to write the plots.
  
  // ----- END -- Setup --------------------------------------------------------

  
  // ----- BEGIN -- Plots ------------------------------------------------------
  
  // ----- END -- Plots --------------------------------------------------------
  
  /// Total number of processed events.
  unsigned int fNEvents { 0U };
  
  // --- END -- Data members ---------------------------------------------------
  
    public:
  
  /// Name of the recommended configuration table for this algorithm.
  static std::string const ConfigurationKey;
  
  struct FHiCLconfig {
    
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    
    using microseconds = util::quantities::intervals::microseconds;
    using nanosecond = util::quantities::points::nanosecond;
    
    
    struct BaselineOptions_t {
      fhicl::Atom<bool> SubtractBaseline {
        Name{ "SubtractBaseline" },
        Comment{ "estimate and subtract the baseline from the waveforms" },
        false // default
        };
      
      fhicl::Atom<unsigned int> EstimationSamples {
        Name{ "EstimationSamples" },
        Comment{ "number of samples at the beginning of the waveform for baseline estimation" }
        // mandatory
        };
      
      fhicl::Atom<bool> PrintBaseline {
        Name{ "PrintBaseline" },
        Comment{ "prints the baseline on screen for each channel and plot." },
        false
        };
    }; // BaselineOptions_t
    
    
    struct ReadoutSettings_t {
      fhicl::Atom<raw::Channel_t> Channel {
        Name{ "Channel" },
        Comment{ "ID of the channel these settings are applied to" }
        };
      fhicl::OptionalAtom<raw::ADC_Count_t> Baseline {
        Name{ "Baseline" },
        Comment{ "readout waveform baseline, in ADC counts" }
        };
      fhicl::OptionalAtom<raw::ADC_Count_t> Threshold {
        Name{ "Threshold" },
        Comment{ "LVDS discrimination threshold, in ADC counts" }
        };
    }; // ReadoutSettings_t
    
    
    fhicl::Atom<art::InputTag> WaveformTag {
      Name{ "WaveformTag" },
      Comment{ "input tag for PMT waveforms" },
      art::InputTag{ "daqPMT" }
      };
    
    fhicl::OptionalAtom<art::InputTag> TriggerTag {
      Name{ "TriggerTag" },
      Comment{ "input tag for global trigger" }
      };
    
    fhicl::Atom<unsigned int> Channels {
      Name{ "Channels" },
      Comment
        { "number of PMT channel to be analyzer (from 0 to this one excluded)" },
      360U
      };
    
    fhicl::Sequence<fhicl::TableAs<ValueRange<optical_time>, ValueRangeFHiCL<microsecond>>>
    TimeSlices {
      Name{ "TimeSlices" },
      Comment{ "include only waveforms with timestamp within this interval" },
      std::vector<ValueRange<optical_time>>{}
      };
    
    fhicl::OptionalTable<BaselineOptions_t> Baseline {
      Name{ "Baseline" },
      Comment{ "Options about the baseline estimation and subtraction" }
      };
    
    fhicl::OptionalAtom<bool> SharedADCrange {
      Name{ "SharedADCrange" },
      Comment{ "whether all plots in a screen will share the same ADC range" }
      };
    
    fhicl::OptionalSequence<fhicl::Table<ReadoutSettings_t>> ReadoutSettings {
      Name{ "ReadoutSettings" },
      Comment{ "Configured readout settings, per channel" },
      };
    
    fhicl::OptionalDelegatedParameter ReadoutThresholds {
      Name{ "ReadoutThresholds" },
      Comment
        { "Configured discrimination thresholds, in `channel: baselineADC` form" }
      };
    
    fhicl::Atom<float> StaggerPlots {
      Name{ "StaggerPlots" },
      Comment
        { "displace plots from the grid by this fraction to resemble ICARUS PMT disposition" },
      0.0f // default
      };
    
    fhicl::Atom<nanoseconds> TickDuration {
      Name{ "TickDuration" },
      Comment{ "optical readout digitization tick duration" },
      2_ns
      };
    
  }; // FHiCLconfig
  
  using Parameters = fhicl::Table<FHiCLconfig>;
  
  
  /// Constructor: reads the configuration from the specified parameters set.
  DrawPMTwaveforms(Parameters const& config);
  
  
  /// Prints on `out` screen a configuration summary.
  static void printConfigurationHelp(std::ostream& out);
  
  
  /**
   * @brief Sets the algorithm up.
   * @param pDestDir ROOT output directory for the plots
   * @param clocksData detector clocks information (event-independent)
   * @param propsData detector properties information (event-independent)
   */
  void setup(TDirectory* pDestDir);
  
  /// Performs the initialization of the algorithm.
  void prepare();
  
  /// Processes a single event.
  template <typename Event>
  void analyze(Event const& event, art::EventID const& id);
  
  /// Completes and saves the plots.
  void finish();
  
  
  /// Prints the current configuration to the specified output stream.
  template <typename Stream>
  void printConfig(Stream&& out) const;
  
    private:
  
  /// Data associated to a waveform.
  struct WaveformInfo_t {
    static constexpr float NoHWSetting
      = std::numeric_limits<raw::ADC_Count_t>::lowest();
    static constexpr float NoBaseline = std::numeric_limits<float>::lowest();
    static constexpr float NoThreshold = std::numeric_limits<float>::lowest();
    static constexpr auto NoTime = std::numeric_limits<optical_time>::lowest();
    
    raw::OpDetWaveform const* waveform = nullptr;
    
    optical_time triggerTime = NoTime;
    optical_time beamGateTime = NoTime;
    nanoseconds beamGateWidth = 0.0_ns;
    
    float baseline = NoBaseline;
    float threshold = NoThreshold;
    
    raw::ADC_Count_t hwBaseline = NoHWSetting;
    raw::ADC_Count_t hwThreshold = NoHWSetting;
    
    operator bool() const { return bool(waveform); }
    bool operator!() const { return !waveform; }
    raw::OpDetWaveform const* operator->() const { return waveform; }
    static bool byChannel(WaveformInfo_t const& a, WaveformInfo_t const& b)
      { return a->ChannelNumber() < b->ChannelNumber(); }
    static bool byTime(WaveformInfo_t const& a, WaveformInfo_t const& b)
      { return a->TimeStamp() < b->TimeStamp(); }
  }; // WaveformInfo_t
  
  using Cluster_t = std::vector<WaveformInfo_t>;
  
  // --- BEGIN -- Configuration ------------------------------------------------
  
  AlgorithmConfiguration parseValidatedAlgorithmConfiguration
    (FHiCLconfig const& config);
  
  AlgorithmConfiguration parseValidatedAlgorithmConfiguration
    (fhicl::ParameterSet const& pset)
    { return parseValidatedAlgorithmConfiguration(Parameters{pset}()); }
  
  template <typename T>
  static void readParam
    (fhicl::ParameterSet const& pset, std::string const& key, T& var)
    { var = pset.get<T>(key); }
  
  // --- END -- Configuration --------------------------------------------------


  // --- BEGIN -- Analysis -----------------------------------------------------
  struct BaselineInfo_t {
    struct RangeStats_t {
      double baseline; ///< The value chosen as representative of baseline.
      unsigned int nSamples = 0; ///< Number of samples in the range.
      double average; ///< Average of samples in the range.
      double variance; ///< Variance of samples in the range.
      double median; ///< Median of samples in the range.
    };
    
    static constexpr double NoBaseline = std::numeric_limits<double>::lowest();
    RangeStats_t estimate; ///< Our best estimate.
    RangeStats_t regionA; ///< First partial of the most trusted region.
    RangeStats_t regionB; ///< Second partial of the most trusted region.
    RangeStats_t regionE; ///< Contol region.
    double const& baseline = estimate.baseline; ///< Best baseline estimate.
    double baselineAverage; ///< Average baseline of trusted and control region.
    double RMS; ///< Baseline RMS.
  }; // BaselineInfo_t
  
  using BaselineEstimates_t = std::vector
    <std::pair<art::EventID, StatCollectorWithMinMaxAndMedian<double>>>;
  
  /// The best estimation for each channel so far.
  BaselineEstimates_t fBestBaselineEstimates;
  
  /// Extracts (and plots) baseline information from the specified `waveform`.
  BaselineInfo_t extractBaseline(raw::OpDetWaveform const& waveform) const;
  
  // --- END ---- Analysis -----------------------------------------------------

  /// Produces a graph with the full content of the waveform.
  std::unique_ptr<TGraph> drawWaveform
    (WaveformInfo_t const& wf, art::EventID const& id) const;
  
  std::vector<Cluster_t> clusterWaveforms
    (std::vector<WaveformInfo_t> waveforms, microseconds duration) const;
  
  /// Returns the representative time of the cluster.
  optical_time clusterTime(Cluster_t const& waveforms) const;
  
  std::unique_ptr<TDirectory> plotWaveformCluster(
    Cluster_t const& cluster, art::EventID const& id, TDirectory& eventOutputDir
    ) const;
  
  /// Prints the average baselines of an event on screen.
  void printBaselines(BaselineEstimates_t const& baselines) const;
  

  /// Returns the waveforms grouped by channel range (30 each group).
  std::vector<Cluster_t> groupWaveformCluster(Cluster_t const& waveforms) const;

  /// Plots the full group of waveforms in a single canvas.
  std::unique_ptr<TCanvas> plotWaveformGroup(
    Cluster_t const& group, art::EventID const& id, optical_time time,
    TDirectory& clusterOutputDir
    ) const;

  /// Returns the lowest and highest channel number among the `waveforms`.
  static std::pair<raw::Channel_t, raw::Channel_t> channelRange
    (Cluster_t const& waveforms);
  
  
}; // DrawPMTwaveforms


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string const DrawPMTwaveforms::ConfigurationKey { "analysis" };

// --- BEGIN -- Code to update -------------------------------------------------

DrawPMTwaveforms::DrawPMTwaveforms
  (Parameters const& config)
  : fConfig{ parseValidatedAlgorithmConfiguration(config()) }
  , fBestBaselineEstimates{ fConfig.nChannels }
  {}


auto DrawPMTwaveforms::parseValidatedAlgorithmConfiguration
  (FHiCLconfig const& config) -> AlgorithmConfiguration
{
  AlgorithmConfiguration algConfig;
  algConfig.waveformTag = config.WaveformTag();
  algConfig.triggerTag = config.TriggerTag().value_or(art::InputTag{});
  algConfig.nChannels = config.Channels();
  algConfig.plotTimes = config.TimeSlices();
  algConfig.staggerFraction = config.StaggerPlots();
  if (std::optional<FHiCLconfig::BaselineOptions_t> baselineOpts = config.Baseline()) {
    algConfig.baseline.subtract = baselineOpts->SubtractBaseline();
    algConfig.baseline.nSamples = baselineOpts->EstimationSamples();
    algConfig.baseline.doPrint = baselineOpts->PrintBaseline();
  } // if baseline specs
  
  if (config.ReadoutSettings()) {
    for (FHiCLconfig::ReadoutSettings_t const& settings
        : *(config.ReadoutSettings())
    ) {
      
      raw::Channel_t const channel = settings.Channel();
      
      if (settings.Baseline()) {
        raw::ADC_Count_t const value = *(settings.Baseline());
        if (algConfig.readoutBaselines.contains(channel)) {
          throw std::runtime_error{
            "Duplicate baseline setting for channel " + std::to_string(channel)
            + " (" + std::to_string(algConfig.readoutBaselines(channel))
            + ", then " + std::to_string(value) + ")."
            };
        }
        algConfig.readoutBaselines.set(channel, value);
      }
      
      if (settings.Threshold()) {
        raw::ADC_Count_t const value = *(settings.Threshold());
        if (algConfig.readoutThresholds.contains(channel)) {
          throw std::runtime_error{
            "Duplicate threshold setting for channel " + std::to_string(channel)
            + " (" + std::to_string(algConfig.readoutThresholds(channel))
            + ", then " + std::to_string(value) + ")."
            };
        }
        algConfig.readoutThresholds.set(channel, value);
      }
      
    } // for
  } // if
  
  algConfig.sharedADCrange = config.SharedADCrange();
  
  algConfig.tickDuration = config.TickDuration();
  return algConfig;
} // DrawPMTwaveforms::parseValidatedAlgorithmConfiguration()


void DrawPMTwaveforms::printConfigurationHelp(std::ostream& out) {
  
  out << "Configuration for the analysis algorithm:\n";
  Parameters const table
    { fhicl::Name{ DrawPMTwaveforms::ConfigurationKey } };
  table.print_allowed_configuration(out);
  out << std::endl;
  
} // DrawPMTwaveforms::printConfigurationHelp()


void DrawPMTwaveforms::setup(TDirectory* pDestDir) {
  
  fDestDir = pDestDir;
  
} // DrawPMTwaveforms::setup()


void DrawPMTwaveforms::prepare() {
  
  fNEvents = 0U;
  
} // DrawPMTwaveforms::prepare()


template <typename Event>
void DrawPMTwaveforms::analyze(Event const& event, art::EventID const& id) {
  
  ++fNEvents;
  
  //
  // read the data
  //
  auto const& waveforms
//     = event.template getProduct<std::vector<raw::OpDetWaveform>> // art 3.9; gallery...?
//     (fConfig.waveformTag);
    = *(event.template getValidHandle<std::vector<raw::OpDetWaveform>>
    (fConfig.waveformTag));
  
  optical_time triggerTime, beamGateTime; // 0 by default
  microseconds beamGateWidth { 0.0 };
  if (!fConfig.triggerTag.empty()) {
    auto const& triggers
//     = event.template getProduct<std::vector<raw::Trigger>> // art 3.9; gallery...?
//     (fConfig.triggerTag);
      = *(event.template getValidHandle<std::vector<raw::Trigger>>
        (fConfig.triggerTag))
      ;
    if (!triggers.empty()) {
      raw::Trigger const& trigger = triggers.front();
      triggerTime
        = optical_time::castFrom(microsecond{ trigger.TriggerTime() });
      beamGateTime
        = optical_time::castFrom(microsecond{ trigger.BeamGateTime() });
    } // if have trigger
    auto const& beamGates
//     = event.template getProduct<std::vector<sim::BeamGateInfo>> // art 3.9; gallery...?
//     (fConfig.triggerTag);
      = *(event.template getValidHandle<std::vector<sim::BeamGateInfo>>
        (fConfig.triggerTag))
      ;
    if (!beamGates.empty()) {
      sim::BeamGateInfo const& beamGate = beamGates.front();
      beamGateWidth = nanoseconds{ beamGate.Width() }; // stored in simulation time
    } // if have trigger

    mf::LogVerbatim{ "DrawPMTwaveforms" } << "Trigger time: " << triggerTime
      << "; beam gate: [ " << beamGateTime << " ; " << (beamGateTime + beamGateWidth)
      << " ] (" << beamGateWidth << ")";
  } // if specified trigger
  
  //
  // preselect the waveforms
  //
  std::vector<StatCollectorWithMinMaxAndMedian<double>> Baselines
    { fConfig.nChannels };
  std::vector<WaveformInfo_t> selectedWaveforms;
  for (raw::OpDetWaveform const& waveform: waveforms) {
    optical_time const time { microsecond { waveform.TimeStamp() } };
    raw::Channel_t const channel = waveform.ChannelNumber();
    
    bool selected = fConfig.plotTimes.empty();
    for (ValueRange<optical_time> const& goodTime: fConfig.plotTimes) {
      if (!goodTime.contains(time)) continue;
      selected = true;
      break;
    } // for time ranges
    if (!selected) continue;
    
    float baseline = 0.0; // no baseline
    if (fConfig.baseline.subtract || fConfig.baseline.doPrint) {
      BaselineInfo_t baselineInfo = extractBaseline(waveform);
      baseline = baselineInfo.baseline;
      Baselines.at(channel).add(baseline);
    }
    
    selectedWaveforms.push_back(WaveformInfo_t{
        &waveform
      , triggerTime
      , beamGateTime
      , beamGateWidth
      , baseline
      , WaveformInfo_t::NoThreshold
      , fConfig.readoutBaselines(channel, WaveformInfo_t::NoHWSetting)
      , fConfig.readoutThresholds(channel, WaveformInfo_t::NoHWSetting)
      });
    
  } // for all waveforms
  
  if (fConfig.baseline.doPrint) {
    
    for (auto const& [ channel, stats ]: util::enumerate(Baselines)) {
      if (!stats.has_data()) continue;
      auto& bestStats = fBestBaselineEstimates.at(channel);
      if (bestStats.second.has_data() && bestStats.second.RMS() <= stats.RMS())
        continue;
      bestStats = { event.eventAuxiliary().id(), stats };
    } // for
    
  }
  
  //
  // cluster waveforms in time
  //
  
  std::vector<Cluster_t> waveformClusters
    = clusterWaveforms(selectedWaveforms, 2.0_us);
  
  //
  // draw each cluster
  //
  TDirectory* eventOutputDir = fDestDir->mkdir(
    ("R" + std::to_string(id.run()) + "E" + std::to_string(id.event())).c_str(),
    ("Run " + std::to_string(id.run()) + " event " + std::to_string(id.event()))
      .c_str()
    );
  
  for (Cluster_t const& cluster: waveformClusters) {
    
    std::unique_ptr<TDirectory> plots
      = plotWaveformCluster(cluster, id, *eventOutputDir);
    
    util::ROOT::TDirectoryChanger dg { eventOutputDir };
    plots->Write();
    
  } // for clusters
  
  eventOutputDir->Write();
  delete eventOutputDir;
  
} // DrawPMTwaveforms::analyze()


auto DrawPMTwaveforms::clusterWaveforms
  (std::vector<WaveformInfo_t> waveforms, microseconds duration) const
  -> std::vector<Cluster_t>
{
  //
  // simple clustering in time;
  // returned waveforms in each cluster are sorted by channel number
  //
  
  std::sort(waveforms.begin(), waveforms.end(), WaveformInfo_t::byTime);
  
  std::vector<Cluster_t> clusters;
  auto itWf = waveforms.begin();
  auto const wend = waveforms.end();
  
  if (itWf == wend) return clusters;
  
  Cluster_t currentCluster { *itWf };
  util::quantities::points::microsecond currentClusterTime
    { itWf->waveform->TimeStamp() };
  while (++itWf != wend) {
    
    util::quantities::points::microsecond const waveformTime
      { itWf->waveform->TimeStamp() };
    
    if (waveformTime - currentClusterTime >= duration) { // next cluster
      // close and store the current cluster
      clusters.push_back(std::move(currentCluster));
      currentCluster.clear(); // here is the new one
      currentClusterTime = waveformTime;
    } // if
    
    currentCluster.push_back(*itWf);
    
  } // for
  
  if (!currentCluster.empty()) clusters.push_back(std::move(currentCluster));
  
  return clusters;
} // DrawPMTwaveforms::clusterWaveforms()


auto DrawPMTwaveforms::clusterTime
  (Cluster_t const& waveforms) const -> optical_time
{
  double minTime = std::numeric_limits<double>::max();
  for (WaveformInfo_t const& wf: waveforms)
    if (wf->TimeStamp() < minTime) minTime = wf->TimeStamp();
  return optical_time{ util::quantities::points::microsecond{ minTime } };
} // DrawPMTwaveforms::clusterTime()


std::unique_ptr<TDirectory> DrawPMTwaveforms::plotWaveformCluster
  (Cluster_t const& cluster, art::EventID const& id, TDirectory& eventOutputDir)
  const
{
  
  optical_time const time = clusterTime(cluster);
  using std::to_string;
  auto outDir = std::make_unique<TDirectoryFile>(
    ("R" + to_string(id.run()) + "E" + to_string(id.event())
      + "TS" + to_string
        (static_cast<int>(std::round(time.convertInto<microsecond>().value())))
    ).c_str(),
    ("Run " + to_string(id.run()) + " event " + to_string(id.event())
      + " cluster at time " + to_string(time.convertInto<microsecond>())
    ).c_str(),
    "TDirectoryFile", &eventOutputDir
    );
  
  std::vector<Cluster_t> groups = groupWaveformCluster(cluster);
  
  mf::LogVerbatim log { "DrawPMTwaveforms" };
  log
    << "Run " << id.run() << " event " << id.event() << ": " << cluster.size()
    << " waveforms at t=" << time << ": channels";
  for (Cluster_t const& group: groups) {
    if (group.empty()) continue;
    
    std::unique_ptr<TCanvas> canvas
      = plotWaveformGroup(group, id, time, *outDir);
    if (!canvas) continue;
    
    auto const [ firstChannel, lastChannel ] = channelRange(group);
    log << "  " << firstChannel;
    if (lastChannel != firstChannel) log << "-" << lastChannel;
    
    util::ROOT::TDirectoryChanger dg { outDir.get() };
    canvas->Write();
    gPad = nullptr; // just in case
    
  } // for groups
  
  return outDir;
} // DrawPMTwaveforms::plotWaveformCluster()


auto DrawPMTwaveforms::groupWaveformCluster(Cluster_t const& waveforms) const
  -> std::vector<Cluster_t>
{
  //
  // groups by continuous position
  //
  
  constexpr int NChannels = 360;
  constexpr int ChannelsInGroup = 30;
  
  // hard-coding galore!
  std::vector<Cluster_t> groups{ NChannels / ChannelsInGroup };
  
  for (WaveformInfo_t const& wf: waveforms) {
    if (!wf) continue;
    
    int const groupNumber = wf->ChannelNumber() / ChannelsInGroup;
    groups[groupNumber].push_back(wf);
    
  } // for
  
  for (Cluster_t& group: groups) {
    std::sort(group.begin(), group.end(), WaveformInfo_t::byChannel);
  } // for
  
  return groups;
} // DrawPMTwaveforms::groupWaveformCluster()


std::unique_ptr<TCanvas> DrawPMTwaveforms::plotWaveformGroup(
  Cluster_t const& group, art::EventID const& id, optical_time time,
  TDirectory& clusterOutputDir
  ) const
{
  /*
   * index of the pad:
   *   Channel                 ->  TPad->cd()
   *   04  09  14  19  24  29       01  02  03  04  05  06
   *   03  08  13  18  23  28       07  08  09  10  11  12
   *   02  07  12  17  22  27       13  14  15  16  17  18
   *   01  06  11  16  21  26       19  20  21  22  23  24
   *   00  05  10  15  20  25       25  26  27  28  29  30
   * 
   * 
   *   Channel                    -> TPad->cd()
   *   04  07    14  17    24  27    01  02  03  04  05  06
   *  01    09  11    19  21    29   07  08  09  10  11  12
   *   03  06    13  16    23  26    13  14  15  16  17  18
   *  00    08  10    18  20    28   19  20  21  22  23  24
   *   02  05    12  15    22  25    25  26  27  28  29  30
   */
  auto const padPos = [](raw::Channel_t channel) constexpr
    {
      // channel -> pos
      constexpr int map[10] = { 19,  7, 25, 13,  1, 26, 14,  2, 20,  8 };
      return map[channel % 10] + 2 * (channel % 30 / 10);
    };
  auto leftLeaning = [](raw::Channel_t channel) constexpr
    {
      // channel -> pos
      constexpr bool map[10]
        = { true, true, false, false, false, true, true, true, false, false };
      return map[channel % 10];
    };
  static_assert(padPos( 0) == 19);  static_assert(leftLeaning( 0) ==  true);
  static_assert(padPos( 1) ==  7);  static_assert(leftLeaning( 1) ==  true);
  static_assert(padPos( 2) == 25);  static_assert(leftLeaning( 2) == false);
  static_assert(padPos( 3) == 13);  static_assert(leftLeaning( 3) == false);
  static_assert(padPos( 4) ==  1);  static_assert(leftLeaning( 4) == false);
  static_assert(padPos( 5) == 26);  static_assert(leftLeaning( 5) ==  true);
  static_assert(padPos( 6) == 14);  static_assert(leftLeaning( 6) ==  true);
  static_assert(padPos( 7) ==  2);  static_assert(leftLeaning( 7) ==  true);
  static_assert(padPos( 8) == 20);  static_assert(leftLeaning( 8) == false);
  static_assert(padPos( 9) ==  8);  static_assert(leftLeaning( 9) == false);
  static_assert(padPos(10) == 21);  static_assert(leftLeaning(10) ==  true);
  static_assert(padPos(11) ==  9);  static_assert(leftLeaning(11) ==  true);
  static_assert(padPos(12) == 27);  static_assert(leftLeaning(12) == false);
  static_assert(padPos(13) == 15);  static_assert(leftLeaning(13) == false);
  static_assert(padPos(14) ==  3);  static_assert(leftLeaning(14) == false);
  static_assert(padPos(15) == 28);  static_assert(leftLeaning(15) ==  true);
  static_assert(padPos(16) == 16);  static_assert(leftLeaning(16) ==  true);
  static_assert(padPos(17) ==  4);  static_assert(leftLeaning(17) ==  true);
  static_assert(padPos(18) == 22);  static_assert(leftLeaning(18) == false);
  static_assert(padPos(19) == 10);  static_assert(leftLeaning(19) == false);
  static_assert(padPos(20) == 23);  static_assert(leftLeaning(20) ==  true);
  static_assert(padPos(21) == 11);  static_assert(leftLeaning(21) ==  true);
  static_assert(padPos(22) == 29);  static_assert(leftLeaning(22) == false);
  static_assert(padPos(23) == 17);  static_assert(leftLeaning(23) == false);
  static_assert(padPos(24) ==  5);  static_assert(leftLeaning(24) == false);
  static_assert(padPos(25) == 30);  static_assert(leftLeaning(25) ==  true);
  static_assert(padPos(26) == 18);  static_assert(leftLeaning(26) ==  true);
  static_assert(padPos(27) ==  6);  static_assert(leftLeaning(27) ==  true);
  static_assert(padPos(28) == 24);  static_assert(leftLeaning(28) == false);
  static_assert(padPos(29) == 12);  static_assert(leftLeaning(29) == false);
  static_assert(padPos(30) == 19);  static_assert(leftLeaning(30) ==  true);
  
  auto const [ firstChannel, lastChannel ] = channelRange(group);
  
  auto const opticalToUS
    = [](optical_time t){ return t.convertInto<microsecond>().value(); };
  
  using std::to_string;
  util::ROOT::TDirectoryChanger dg { &clusterOutputDir };
  auto canvas = std::make_unique<TCanvas>(
    ("R" + to_string(id.run()) + "E" + to_string(id.event())
      + "TS" + to_string(static_cast<int>(std::round(opticalToUS(time))))
      + "CH" + to_string(firstChannel)
      + "_" + to_string(lastChannel)
      ).c_str(),
    ("Run " + to_string(id.run()) + " event " + to_string(id.event())
      + " cluster at time " + to_string(time.convertInto<microsecond>())
      + " channels " + to_string(firstChannel)
      + " -- " + to_string(lastChannel)
    ).c_str()
    );
  canvas->Divide(6, 5, 0.0, 0.0); // 6 columns, 5 rows, no margins
  std::vector<TGraph*> graphs{ 30, nullptr };
  bool const bSharedADCrange
    = fConfig.sharedADCrange.value_or(fConfig.baseline.subtract);
  lar::util::MinMaxCollector<double> sampleRange;
  for (WaveformInfo_t const& wf: group) {
    
    std::unique_ptr<TGraph> graph = drawWaveform(wf, id);
    if (!graph) continue;
    
    raw::Channel_t const channel = wf->ChannelNumber();
    
    int const subpad = padPos(channel);
    bool const leanLeft = leftLeaning(channel);
    TVirtualPad* pad = canvas->cd(subpad);
    pad->SetMargin( 
      0.04 + (leanLeft? 0.00: fConfig.staggerFraction), // left
      0.01 + (leanLeft? fConfig.staggerFraction: 0.00), // right
      0.05, 0.00  // bottom, top
      ); // reduced margins
    
    // clone the graph, let TCanvas manage that one
    TGraph* clone = static_cast<TGraph*>(graph->DrawClone("AL"));
    graphs[subpad - 1] = clone;
    clone->SetLineWidth(2);
    clone->SetLineColor(kBlue - 7); // tradition demands waveforms to be blue
    
    pad->SetGrid();
    pad->SetTicks();
    
    // track the (visualized) range
    pad->Update(); // this forces ROOT to update the coordinates stored in `pad`
    double const Ymin = pad->GetUymin(), Ymax = pad->GetUymax();
    lar::util::MinMaxCollector<double> localRange;
    localRange.add({ Ymin, Ymax });
    
    for (raw::ADC_Count_t level: { wf.hwBaseline, wf.hwThreshold }) {
      if (level == WaveformInfo_t::NoHWSetting) continue;
      if (fConfig.baseline.subtract) level -= wf.baseline;
      localRange.add(level);
    }
    
    // extend to include the added "level" lines in the range, if present
    if (clone->GetYaxis()) {
      clone->GetYaxis()->SetRangeUser(localRange.min(), localRange.max());
      pad->Update();
    }
    
    // if 
    if (bSharedADCrange)
      sampleRange.add({ localRange.min(), localRange.max() });
    
  } // for waveforms
  
  //
  // equalize sample ranges
  //
  if (bSharedADCrange && sampleRange.has_data()) {
    
    for (TGraph* graph: graphs) {
      if (!graph || !graph->GetYaxis()) continue;
      graph->GetYaxis()->SetRangeUser(sampleRange.min(), sampleRange.max());
    } // for graphs
    canvas->Update();
  } // if equalize ranges
  
  //
  // draw the trigger and beam lines
  //
  for (auto const& [ iPad, wf ]: util::enumerate(group)) {
    TVirtualPad* pad = canvas->GetPad(iPad + 1);
    pad->cd();
    
    double const Xmin = pad->GetUxmin(), Xmax = pad->GetUxmax();
    
    if (wf.beamGateTime != WaveformInfo_t::NoTime) {
      TLine* beamGateLine = new TLine(
        opticalToUS(wf.beamGateTime), sampleRange.min(),
        opticalToUS(wf.beamGateTime), sampleRange.max()
        );
      beamGateLine->SetVertical();
      beamGateLine->SetLineColor(kYellow - 5);
      beamGateLine->SetLineWidth(2.0);
      beamGateLine->SetBit(kCannotPick);
      beamGateLine->Draw();
    }
    
    if (wf.beamGateWidth > 0_ns) {
      auto const endGateNS = opticalToUS(wf.beamGateTime + wf.beamGateWidth);
      TLine* beamGateEndLine = new TLine(
        endGateNS, sampleRange.min(),
        endGateNS, sampleRange.max()
        );
      beamGateEndLine->SetVertical();
      beamGateEndLine->SetLineColor(kYellow - 3);
      beamGateEndLine->SetLineWidth(1.5);
      beamGateEndLine->SetBit(kCannotPick);
      beamGateEndLine->Draw();
    }
    
    if (wf.triggerTime != WaveformInfo_t::NoTime) {
      TLine* triggerLine = new TLine(
        opticalToUS(wf.triggerTime), sampleRange.min(),
        opticalToUS(wf.triggerTime), sampleRange.max()
        );
      triggerLine->SetVertical();
      triggerLine->SetLineColor(kRed + 2);
      triggerLine->SetLineWidth(2.0);
      triggerLine->SetBit(kCannotPick);
      triggerLine->Draw();
    }
    
    if (wf.hwBaseline != WaveformInfo_t::NoHWSetting) {
      
      double level = wf.hwBaseline;
      if (fConfig.baseline.subtract) level -= wf.baseline;
      
      TLine* baseLine = new TLine(Xmin, level, Xmax, level);
      baseLine->SetHorizontal();
      baseLine->SetLineStyle(kDashed);
      baseLine->SetLineColor(kBlue + 2);
      baseLine->SetLineWidth(2.0);
      baseLine->SetBit(kCannotPick);
      baseLine->Draw();
    }
    
    if (wf.hwThreshold != WaveformInfo_t::NoHWSetting) {
      
      double level = wf.hwThreshold;
      if (fConfig.baseline.subtract) level -= wf.baseline;
      
      TLine* thresholdLine = new TLine(Xmin, level, Xmax, level);
      thresholdLine->SetHorizontal();
      thresholdLine->SetLineStyle(kDotted);
      thresholdLine->SetLineColor(kRed);
      thresholdLine->SetLineWidth(2.0);
      thresholdLine->SetBit(kCannotPick);
      thresholdLine->Draw();
    }
    
  } // for
  
  canvas->cd(0);
  
  return canvas;
} // DrawPMTwaveforms::plotWaveformGroup()



std::pair<raw::Channel_t, raw::Channel_t> DrawPMTwaveforms::channelRange
  (Cluster_t const& waveforms)
{
  lar::util::MinMaxCollector<raw::Channel_t> range;
  for (WaveformInfo_t const& wf: waveforms)
    range.add(wf->ChannelNumber());
  return { range.min(), range.max() };
} // DrawPMTwaveforms::channelRange()


void DrawPMTwaveforms::printBaselines(BaselineEstimates_t const& baselines) const
{
  
  mf::LogInfo out{ "DrawPMTwaveforms" };
  out << "Baseline statistics from " << baselines.size() << " channels:";
  for (auto const& [ channel, IDandStats ]: util::enumerate(baselines)) {
    
    auto const& [ ID, stats ] = IDandStats;
    
    out << "\nCH " << channel << " :";
    if (!stats.has_data()) {
      out << " no data";
      continue;
    }
    raw::ADC_Count_t const hwBaseline
      = fConfig.readoutBaselines(channel, WaveformInfo_t::NoHWSetting);
    auto const medianBaseline = stats.median();
    out << " baseline " << stats.Average()
      << " +/- " << stats.RMS() << " from " << stats.N()
      << " waveforms (range " << (stats.max() - stats.min()) << " : "
      << stats.min() << " - " << stats.max() << "), median " << medianBaseline
      ;
    if (hwBaseline != WaveformInfo_t::NoHWSetting)
      out << "; HW baseline: " << hwBaseline;
    out << " [from R:" << ID.run() << " E:" << ID.event() << "]";
  } // for
  
} // DrawPMTwaveforms::printBaselines()


void DrawPMTwaveforms::finish() {
  
  if (fConfig.baseline.doPrint) printBaselines(fBestBaselineEstimates);
  
} // DrawPMTwaveforms::finish()


template <typename Stream>
void DrawPMTwaveforms::printConfig(Stream&& out) const {
  
  out << "DrawPMTwaveforms using:"
    << "\n * PMT waveforms from '" << fConfig.waveformTag.encode() << '\''
    ;
  if (!fConfig.triggerTag.empty()) {
    out << "\n * trigger information from '" << fConfig.triggerTag.encode()
      << "'";
  }
  if (!fConfig.plotTimes.empty()) {
    out << "\n * only plot waveforms within these " << fConfig.plotTimes.size()
      << " time intervals:";
    for (ValueRange<optical_time> const& range: fConfig.plotTimes)
      out << " " << range;
  }
  if (fConfig.baseline.nSamples > 0) {
    out << "\n * baseline estimated from " << fConfig.baseline.nSamples
      << " samples";
  }
  if (fConfig.baseline.subtract)
    out << "\n * subtract baseline in each plot";
  
  out << "\n";
} // DrawPMTwaveforms::printConfig()


// -----------------------------------------------------------------------------
auto DrawPMTwaveforms::extractBaseline(raw::OpDetWaveform const& waveform) const
  -> BaselineInfo_t
{
  AlgorithmConfiguration::BaselineConfig const& config = fConfig.baseline;
  
  BaselineInfo_t info;
  
  auto const extractBaselineStatistics
    = [](auto begin, auto end) -> BaselineInfo_t::RangeStats_t
    {
      BaselineInfo_t::RangeStats_t stats;
      stats.nSamples = std::distance(begin, end);
      std::tie(stats.average, stats.variance)
        = averageAndVariance<double>(begin, end);
      stats.median = median(begin, end);
      return stats;
    };
  
  // we choose the average as estimation of baseline here
  double const (BaselineInfo_t::RangeStats_t::*baseline)
    = &BaselineInfo_t::RangeStats_t::average;
  
  std::vector<double> const data { waveform.begin(), waveform.end() };
  
  auto const start = data.begin();
  auto const middle = start + config.nSamples / 2;
  auto const stop = middle + config.nSamples / 2; // make it even
  
  info.regionA = extractBaselineStatistics(start, middle);
  info.regionA.baseline = info.regionA.*baseline;
  info.regionB = extractBaselineStatistics(middle, stop);
  info.regionB.baseline = info.regionB.*baseline;
  
  info.estimate.baseline = (info.regionA.baseline + info.regionB.baseline) / 2.0;
  info.estimate.nSamples = (info.regionA.nSamples + info.regionB.nSamples) / 2.0;
  info.estimate.average = (info.regionA.average + info.regionB.average) / 2.0;
  info.estimate.variance = (info.regionA.variance + info.regionB.variance) / 2.0;
  info.estimate.median = median(start, stop);
  
  info.RMS
    = (info.estimate.variance > 0.0)? std::sqrt(info.estimate.variance): 1.0;
  
  info.baselineAverage = info.baseline;

  return info;
} // DrawPMTwaveforms::extractBaseline()


// -----------------------------------------------------------------------------
std::unique_ptr<TGraph> DrawPMTwaveforms::drawWaveform
  (WaveformInfo_t const& wf, art::EventID const& id) const
{
  optical_time const startTime { microsecond{ wf->TimeStamp() } };
  auto shape = std::make_unique<TGraph>(wf->size());
  using std::to_string;
  shape->SetNameTitle(
    ("WaveformR" + to_string(id.run()) + "E" + to_string(id.event())
      + "TS" + to_string(static_cast<int>(std::round(wf->TimeStamp())))
      + "Ch" + to_string(wf->ChannelNumber())
    ).c_str(),
    ("Run " + to_string(id.run()) + " event " + to_string(id.event())
     + ": PMT waveform at T=" + to_string(wf->TimeStamp()) + " #mus"
     + " channel " + to_string(wf->ChannelNumber())
     + ";sample time  [ "
      + to_string(optical_time::unit()) + " ]"
    ).c_str()
    );
  
  for (auto [ iSample, sample ]: util::enumerate(*(wf.waveform))) {
    optical_time const time = startTime + iSample * fConfig.tickDuration;
    shape->SetPoint(iSample, time.value(), sample - wf.baseline);
  }
  MF_LOG_TRACE("test") << "Waveform for channel " << wf->ChannelNumber()
    << " plotted: '" << shape->GetName()
    << "' (\"" << shape->GetTitle() << "\")";
  
  return shape;
} // DrawPMTwaveforms::drawWaveform()


// -----------------------------------------------------------------------------


/**
 * @brief Runs the analysis macro.
 * @param configFile path to the FHiCL configuration to be used for the services
 * @param inputFiles vector of path of file names
 * @return an integer as exit code (0 means success)
 */
int runAnalysis
  (std::string const& configFile, std::vector<std::string> inputFiles)
{
  
  TH1::AddDirectory(kFALSE); // do not register histograms in the current directory
  gROOT->SetBatch(kFALSE);
  
  /*
   * the "test" environment configuration
   */
  // read FHiCL configuration from a configuration file:
  fhicl::ParameterSet config = lar::standalone::ParseConfiguration(configFile);

  // set up message facility (always picked from "services.message")
  lar::standalone::SetupMessageFacility(config, "");

  auto const& analysisConfig = config.get<fhicl::ParameterSet>("analysis");
  
  // event loop options
  constexpr auto NoLimits = std::numeric_limits<unsigned int>::max();
  unsigned int nSkip = analysisConfig.get("skipEvents", 0U);
  unsigned int const maxEvents = analysisConfig.get("maxEvents", NoLimits);
  if (analysisConfig.has_key("inputFile")) {
    inputFiles.push_back(analysisConfig.get<std::string>("inputFile"));
  }

  /*
   * the preparation of input file list
   */
  if (inputFiles.empty()) {
    throw std::runtime_error("An input file is required!");
  }
  if (inputFiles.size() != 1) {
    throw std::runtime_error("Support for multiple input parameters not implemented yet!"); // TODO
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
    mf::LogVerbatim("runAnalysis")
      << "Creating output file: '" << fileName << "'" << std::endl;
    pHistFile = std::make_unique<TFile>(fileName.c_str(), "RECREATE");
  }

  /*
   * preparation of the algorithm class
   */
  
  // configuration from `ConfigurationKey` table of configuration file:
  DrawPMTwaveforms plotAlg {
    analysisConfig.get<fhicl::ParameterSet>
      (DrawPMTwaveforms::ConfigurationKey)
    };
  
  plotAlg.printConfig(mf::LogVerbatim{"runAnalysis"});
  
  plotAlg.setup(pHistFile.get());
  
  plotAlg.prepare();
  
  unsigned int numEvents { 0U };

  /*
   * the event loop
   */
  if (maxEvents) // if user requested zero events, don't loop at all
  for (gallery::Event event(allInputFiles); !event.atEnd(); event.next()) {
    
    if (nSkip > 0) { --nSkip; continue; }
    
    // *************************************************************************
    // ***  SINGLE EVENT PROCESSING BEGIN  *************************************
    // *************************************************************************

    ++numEvents;
    art::EventID const& eventID = event.eventAuxiliary().eventID();
    {
      mf::LogVerbatim log("runAnalysis");
      log << "This is event " << event.fileEntry() << "-" << event.eventEntry()
        << ", " << eventID << " (" << numEvents;
      if (maxEvents < NoLimits) log << "/" << maxEvents;
      log << ")";
    }
    
    plotAlg.analyze(event, eventID);
    

    // *************************************************************************
    // ***  SINGLE EVENT PROCESSING END    *************************************
    // *************************************************************************

    // checking this after processing avoids pointless load of the next file
    if (numEvents >= maxEvents) {
      mf::LogVerbatim("runAnalysis") << "Maximum number of events reached.";
      break;
    }
  } // for

  plotAlg.finish();
  
  pHistFile->Write();
  
  return 0;
} // runAnalysis()


/// Version with a single input file.
int runAnalysis(std::string const& configFile, std::string filename)
  { return runAnalysis(configFile, std::vector<std::string>{ filename }); }

#if !defined(__CLING__)
int main(int argc, char** argv) {
  
  char **pParam = argv + 1, **pend = argv + argc;
  if (pParam == pend) {
    std::cerr << "Usage: " << argv[0] << "  configFile [inputFile ...]"
      << std::endl;
    DrawPMTwaveforms::printConfigurationHelp(std::cerr);
    return 1;
  }
  std::string const configFile = *(pParam++);
  std::vector<std::string> fileNames;
  std::copy(pParam, pend, std::back_inserter(fileNames));
  
  return runAnalysis(configFile, std::move(fileNames));
} // main()

#endif // !__CLING__
