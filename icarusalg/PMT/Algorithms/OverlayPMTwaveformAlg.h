/**
 * @file   icarusalg/PMT/Algorithms/OverlayPMTwaveformAlg.h
 * @brief  Algorithm overlaying simulated PMT waveforms on data PMT waveforms.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   March 23, 2026
 * @see    icarusalg/PMT/Algorithms/OverlayPMTwaveformAlg.cxx
 */

#ifndef ICARUSALG_PMT_ALGORITHMS_OVERLAYPMTWAVEFORMALG_H
#define ICARUSALG_PMT_ALGORITHMS_OVERLAYPMTWAVEFORMALG_H

// ICARUS libraries
#include "icarusalg/Utilities/TimeInterval.h"

// SBN/ICARUS objects
#include "sbnobj/ICARUS/PMT/Data/WaveformBaseline.h"

// LArSoft and framework libraries
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h" // detinfo::timescales::electronics_time
#include "lardataalg/Utilities/quantities/spacetime.h" // nanoseconds, ...
#include "lardataobj/RawData/OpDetWaveform.h"
#include "cetlib_except/exception.h"

// C/C++ standard libraries
#include <cstddef>
#include <cstdint> // std::size_t, std::ptrdiff_t
#include <optional>
#include <string>
#include <vector>


namespace sbn::opdet { class OverlayPMTwaveformAlg; }


//------------------------------------------------------------------------------
/**
 * @brief Overlays simulated PMT waveforms on data PMT waveforms, time-matched.
 *
 * This algorithm overlays the content of a collection of "simulation" PMT
 * waveforms on top of a collection of "data" PMT waveforms.
 *
 * The output waveform segmentation (timestamp, length, channel) matches the
 * input data segmentation exactly.
 *
 * Overlay is time-based: for each data waveform, only simulated waveforms on the
 * same channel and whose time interval overlaps the data waveform time interval
 * contribute. The overlap is computed assuming uniform sampling for both data
 * and simulated waveforms and using the configured optical sampling period.
 *
 * Each channel is merged independently of any other.
 * 
 * Effectively, all the samples from the simulated waveforms that do not fall on
 * the time of any data sample are discarded. In other words, simulation
 * waveforms are truncated in time not to exceed the data ones.
 * In the time ticks where there is no simulation, data samples are also left
 * untouched.
 *
 *
 * Timestamp snapping
 * ------------------
 *
 * The timestamp of each waveform is expected to represent the time of its first
 * sample (sample #0) on the sampling grid defined by the optical tick.
 *
 * To mitigate rounding/representation artifacts in the input (e.g. with a 1 us
 * tick, a timestamp reported as 1499.999 us), the algorithm snaps each waveform
 * start time to the closest tick boundary before computing overlaps and sample
 * indices.
 *
 * If the input timestamps are not aligned to the sampling grid, the snapping
 * will still be applied (by rounding to the closest tick boundary); users are
 * responsible for ensuring this produces the intended behavior.
 *
 *
 * Baseline handling
 * -----------------
 *
 * The baseline associated to the output waveform is intended to be the same
 * baseline as the corresponding input data waveform.
 *
 * The baseline associated to each simulated waveform is used to compute the
 * contribution to add to the data waveform sample-by-sample as:
 *
 *     contribution = (simSample - simBaseline)
 *
 * For clipping purposes, the baseline is subtracted first, then the
 * contribution is added to the data waveform.
 * 
 * Note that while the baseline can be specified as a real number, the input
 * waveforms always store integral-type ADC counts. For that reason
 * **it is strongly advised to have integral baseline values**.
 *
 *
 * Assumptions
 * -----------
 *
 * This algorithm assumes that:
 *
 * 1. The simulated waveform is generated without electronic noise: noise is
 *    already present in the data waveform, so adding simulation noise would
 *    double-count it.
 *
 * 2. The simulated waveform signal is always at or below its baseline:
 *
 *       simSample - simBaseline <= 0
 *
 *    Therefore, the overlay contribution is never positive and the operation can
 *    only decrease the data ADC value.
 *    This may be strictly not true in case of rounding in the template responses
 *    used to compose the waveform, and also with actual overshooting of the
 *    pulses. It is however assumed that the baseline is low enough that the
 *    range boundary of the waveform (`16384`) is never reached.
 *
 * 3. Waveforms are not overlapping in each of the data and sim sequences.
 *
 *
 * Saturation
 * ----------
 *
 * The PMT ADC dynamic range is 14 bits, i.e. `[0, 16383]`. Under the assumptions
 * above, the overlay cannot increase the data sample and the only expected
 * saturation is on the lower bound: values below `0` are clamped to `0`.
 *
 *
 * Configuration
 * -------------
 * 
 * * `opticalTick`: PMT digitiser sampling period. Must be specified.
 * * `baselineCheck`:
 *     * `length`: expected initial flat-baseline duration in simulated
 *       waveforms. `0` (default) disables the check.
 *     * `tolerance`: fraction of tolerated failures in an event.
 * * `logCategory`: name of the output stream for messages (unused so far).
 * 
 */
class sbn::opdet::OverlayPMTwaveformAlg {

  public:

  // aliases
  using microseconds = util::quantities::intervals::microseconds;
  using nanoseconds = util::quantities::intervals::nanoseconds;
  using electronics_time = detinfo::timescales::electronics_time;
  using Interval_t = icarus::ns::util::TimeInterval<electronics_time>;

  /// Input waveform with its baseline.
  struct InputWaveform_t {
    raw::OpDetWaveform const* waveform = nullptr; ///< Waveform (required).
    icarus::WaveformBaseline const* baseline = nullptr; ///< Baseline (required).
  };

  /// Result of the overlay operation.
  struct OverlaidWaveforms {
    std::vector<raw::OpDetWaveform> waveforms; ///< Output waveforms (data segmentation).
  };

  /// Configuration parameters for the algorithm.
  struct Config {
    
    struct BaselineCheck {
      
      /// Expected initial flat-baseline duration in simulated waveforms.
      microseconds length = microseconds{ 0.0 };
      
      /// Fraction of tolerated waveform check failures.
      float tolerance = 0.0;
      
    }; // BaselineCheck

    nanoseconds opticalTick; ///< PMT digitiser sampling period.
    
    BaselineCheck baselineCheck; ///< Baseline check configuration.

    /// Name of the console output stream for messages.
    std::string logCategory = "OverlayPMTwaveformAlg";
    
  }; // struct Config

  
  /// Constructor: configures the algorithm.
  explicit OverlayPMTwaveformAlg(Config const& config);

  /**
   * @brief Performs the overlay.
   * @param data set of base waveforms (supposedly from data)
   * @param sim set of additional waveforms to be superposed to `data`
   * @param limitTo interval to select which data waveforms to superpose
   * @return a complete set of waveforms with superposition
   * 
   * The returned set of waveforms corresponds one-to-one to the waveforms in
   * `data`. Each one will have the same time range and the same baseline as
   * its corresponding `data` waveform.
   * 
   * The waveforms in data are expected to be non-overlapping (they _can_ be
   * adjacent). The same requirement is enforced for the simulated waveforms.
   * 
   * The `limitTo` interval, if specified, selects which `data` waveforms will
   * undergo superposition. The `data` waveforms that do not overlap that
   * interval at all will be copied verbatim into the output. The others
   * will undergo full superposition over all their covered interval, including
   * the part outside of `limitTo`. All `sim` waveforms are considered for the
   * superposition.
   * 
   * See the description of the algorithm for more details on the
   * superposition.
   */
  OverlaidWaveforms overlay(
    std::vector<InputWaveform_t> const& data,
    std::vector<InputWaveform_t> const& sim,
    std::optional<Interval_t> limitTo = std::nullopt
    ) const;

  /// @brief Dumps the algorithm configuration into the specified output stream.
  /// 
  /// Takes over the current output line and does not terminate the last line.
  template <typename Stream>
  void dumpConfiguration(
    Stream& out, std::string const& indent, std::string const& firstIndent
    ) const;
  
  /// Dumps the algorithm configuration into the specified output stream.
  template <typename Stream>
  void dumpConfiguration(Stream& out, std::string const& indent = "") const
    { dumpConfiguration(out, indent, indent); }


  private:

  /// Helper for quick time lookup of time-sorted waveforms.
  struct ChannelSimEntries_t;
  
  /// Type for optimised lookups, first by channel and then by time range.
  using SimEntryLookup_t = std::map<raw::Channel_t, ChannelSimEntries_t>;
  
  /// Parameters of the baseline check.
  struct BaselineCheckParams_t: Config::BaselineCheck {
    
    /// Constructor: copies the configuration and fills the cache.
    BaselineCheckParams_t(Config::BaselineCheck config, nanoseconds tick);
    
    /// Length of tested waveform (0 disables).
    std::size_t ticks = 0;
    
    /// Returns whether the check is effectively enabled (i.e. it could fail).
    constexpr bool enabled() const noexcept
      { return (ticks > 0) && (tolerance < 1.0); }
    
  }; // BaselineCheckParams_t
  
  
  // --- BEGIN ---  Configuration  ---------------------------------------------
  
  /// Optical sampling period (PMT digitiser tick).
  nanoseconds const fOpticalTick;
  
  /// Parameters for the test for noiseless baseline.
  BaselineCheckParams_t const fBaselineCheckParams;
  
  /// Name of the console output stream for messages.
  std::string const fLogCategory;
  
  // ---  END  ---  Configuration  ---------------------------------------------

  /**
   * @brief Overlays a single `data` waveform with all simulated waveforms.
   * @param data the data waveform to be overlaid
   * @param simEntriesByChannel lookup object with all the simulated waveforms
   * @param limitTo perform actual overlay only if data overlap this interval
   * @return the data waveform overlaid with simulated waveforms
   * 
   * The algorithm selects the simulated waveforms from `simEntriesByChannel`
   * to be overlaid to the `data` waveform, and then performs the overlay.
   * 
   * If `limitTo` is specified, overlay is performed only if `data` intersects
   * that interval; otherwise, a copy of `data` waveform is returned unchanged.
   * 
   * All the simulated waveforms from the channel of `data` which overlap its
   * time interval are overlaid, one by one, by invoking `overlayWaveform()`
   * in sequence.
   * 
   * The object `simEntriesByChannel` may be created with `makeSimEntryLookup()`.
   */
  raw::OpDetWaveform overlayDataWaveform(
    InputWaveform_t const& data,
    SimEntryLookup_t const& simEntriesByChannel,
    std::optional<Interval_t> limitTo
    ) const;

  /// Adds `contrib` waveform, `baseline` subtracted, to `dest` in place.
  void overlayWaveform(
    raw::OpDetWaveform& dest,
    raw::OpDetWaveform const& contrib, icarus::WaveformBaseline const& baseline
    ) const;

  /// Performs the baseline check on a full waveform set.
  /// @throw cet::exception on the first waveform failing the test
  /// @see checkNoiselessBaseline(raw::OpDetWaveform const&, float) const
  void checkNoiselessBaseline(std::vector<InputWaveform_t> const& sim) const;

  /// Performs the baseline check on a single waveform. The baseline is rounded.
  /// @return an error message, or empty if check passed
  std::string checkNoiselessBaseline
    (raw::OpDetWaveform const& waveform, float baseline) const;

  /// Rearranges the input waveforms into a time-ordered map by channel.
  SimEntryLookup_t makeSimEntryLookup
    (std::vector<InputWaveform_t> const& waveforms) const;
  
  /// Returns the number of ticks contained in a time interval (rounded).
  std::ptrdiff_t ticksInInterval(nanoseconds interval) const;
  
  /// Returns the number of ticks contained in a time interval (rounded).
  static std::ptrdiff_t ticksInInterval(nanoseconds interval, nanoseconds tick);
  
  /// Returns the index of a sample at time `t` given the waveform `start` time.
  std::ptrdiff_t timeToSample
    (electronics_time t, electronics_time start = electronics_time{}) const;
  
  /// Returns the start time of the waveform, snapped to the tick.
  electronics_time snappedStartTime(raw::OpDetWaveform const& waveform) const;

  /// Returns the time interval covered by the waveform.
  Interval_t waveformInterval(raw::OpDetWaveform const& wv) const;

  /// Clamping of a sample value to the expected ADC range.
  static raw::ADC_Count_t clampToADC(raw::ADC_Count_t value);

}; // class sbn::opdet::OverlayPMTwaveformAlg


// -----------------------------------------------------------------------------
// ---  template implementation
// -----------------------------------------------------------------------------
template <typename Stream>
void sbn::opdet::OverlayPMTwaveformAlg::dumpConfiguration
  (Stream& out, std::string const& indent, std::string const& firstIndent) const
{
  out << firstIndent << "OverlayPMTwaveformAlg configuration:"
    << "\n" << indent << " - optical sampling period: " << fOpticalTick;

  if (fBaselineCheckParams.enabled()) {
    out << "\n" << indent << " - checking that";
    if (fBaselineCheckParams.tolerance <= 0) out << " all";
    else {
      out << " at least " << (fBaselineCheckParams.tolerance * 100) << "% of";
    }
    out << " the waveforms in an event start with at least "
      << (fBaselineCheckParams.ticks * fOpticalTick)
      << " (" << fBaselineCheckParams.ticks << " samples) of noiseless baseline";
  }
  else {
    out << "\n" << indent << " - check of absence of noise in waveforms: disabled";
  }
} // sbn::opdet::OverlayPMTwaveformAlg::dumpConfiguration()


// -----------------------------------------------------------------------------

#endif // ICARUSALG_PMT_ALGORITHMS_OVERLAYPMTWAVEFORMALG_H
