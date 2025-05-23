/**
 * @file   icarusalg/PMT/Algorithms/SharedWaveformBaseline.h
 * @brief  Extracts and writes PMT waveform baselines.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   May 5, 2022
 * @see    icarusalG/pMT/Algorithms/SharedWaveformBaseline.cxx
 */

#ifndef ICARUSALG_PMT_ALGORITHMS_SHAREDWAVEFORMBASELINE_H
#define ICARUSALG_PMT_ALGORITHMS_SHAREDWAVEFORMBASELINE_H


// LArSoft libraries
#include <cstdint>  // uint16_t in OpDetWaveform.h
#include "lardataobj/RawData/OpDetWaveform.h"

// C/C++ standard libraries
#include <vector>
#include <utility> // std::move()
#include <string>
#include <ostream>
#include <cstdint> // std::size_t


// -----------------------------------------------------------------------------
namespace opdet { class SharedWaveformBaseline; }
/**
 * @class opdet::SharedWaveformBaseline
 * @brief Extracts a common baseline from waveforms.
 * 
 * This algorithm processes a group of waveforms at a time, and returns a common
 * baseline for them.
 * The baseline is learned by looking at a fixed size of the beginning of each
 * of the waveforms, as follows:
 * 
 * 1. the RMS of the first portion of each baseline is computed;
 * 2. the median of the samples on the same portion of data is also computed;
 * 3. an acceptance range is constructed, using the median of the sample as the
 *    center and `nRMS` times the median of the RMS as maximum distance from
 *    that center in either direction;
 * 4. as a second pass, if in the first portion of a waveform there are at least
 *    `nExcessSamples` samples in a row that are outside of the acceptance
 *    range, that waveform is excluded;
 * 5. all the samples in the first portion of the remaining waveforms are
 *    averaged to obtain the final estimation of the baseline; this last step
 *    should increase the resolution of the baseline beyond the median that was
 *    obtained at step 2;
 * 6. if no waveform passed the check on step 4, then the baseline is defined as
 *    the median of the set of medians from each waveform, in an attempt to
 *    suppress the contribution of outliers. In this case, the number of used
 *    samples is conventionally returned to be `0`.
 * 
 * The parameters are specified at algorithm construction time and are contained
 * in the `Params_t` object.
 * 
 */
class opdet::SharedWaveformBaseline {
    public:
  
  /// Algorithm configuration parameters.
  struct Params_t {
    
    std::size_t nSample; ///< Number of samples to use from each waveform.
    
    double nRMS; ///< Number of RMS from the baseline to discard a waveform.
    
    /// Number of samples out of range to discard a waveform.
    unsigned int nExcessSamples;
    
    /// Dumps this configuration into the output stream `out`.
    template <typename Stream>
    void dump(
      Stream& out,
      std::string const& indent, std::string const& firstIndent
      ) const;
    template <typename Stream>
    void dump(Stream& out, std::string const& indent = "") const
      { dump(out, indent, indent); }
    
  }; // Params_t
  
  
  /// Type for algorithm result.
  struct BaselineInfo_t {
    
    /// Magic value used to denote the lack of a (`double`) data item.
    static constexpr double NoInfo = std::numeric_limits<double>::max();
    
    /// Value of the baseline [ADC#]
    double baseline = NoInfo;
    
    /// The RMS found during the extraction.
    double RMS = NoInfo;
    
    /// Number of waveforms used for the extraction.
    unsigned int nWaveforms = 0U;
    
    /// Number of samples used for the extraction.
    unsigned int nSamples = 0U;
    
  }; // BaselineInfo_t
  
  
  SharedWaveformBaseline(Params_t params, std::string logCategory):
      fParams{ std::move(params) }
    , fLogCategory{ std::move(logCategory) }
    {}
  
  /// Returns a common baseline from all the specified waveforms.
  BaselineInfo_t operator()
    (std::vector<raw::OpDetWaveform const*> const& waveforms) const;
  
  /// Returns the set of configuration parameters of this algorithm.
  Params_t const& parameters() const { return fParams; }
  
    private:
  Params_t fParams; ///< Algorithm parameters.
  
  std::string fLogCategory; ///< Name of stream category for console messages.
  
  
  /// Returns central value and radius for the accepted sample range.
  std::pair<double, double> acceptanceRange
    (std::vector<raw::OpDetWaveform const*> const& waveforms) const;
  
  /// Returns the list of medians of all the specified `waveforms`.
  std::vector<raw::ADC_Count_t> waveformMedians
    (std::vector<raw::OpDetWaveform const*> const& waveforms) const;
    
  /// Returns the median of the maxima of each waveform.
  raw::ADC_Count_t maximaMedian
    (std::vector<raw::OpDetWaveform const*> const& waveforms) const;
  
  /// Returns the median of the medians of the specified `waveforms`.
  raw::ADC_Count_t medianOfMedians
    (std::vector<raw::OpDetWaveform const*> const& waveforms) const;
  
  /// Returns the maximum among the medians of the specified `waveforms`.
  raw::ADC_Count_t maximumOfMedians
    (std::vector<raw::OpDetWaveform const*> const& waveforms) const;
  
}; // opdet::SharedWaveformBaseline


//------------------------------------------------------------------------------
namespace opdet {
  
  inline std::ostream& operator<<
    (std::ostream& out, SharedWaveformBaseline::Params_t const& params)
    { params.dump(out); return out; }

} // namespace opdet


//------------------------------------------------------------------------------
//---  Template implementation
//------------------------------------------------------------------------------
template <typename Stream>
void opdet::SharedWaveformBaseline::Params_t::dump(
  Stream& out,
  std::string const& indent, std::string const& firstIndent
  ) const
{
  out << firstIndent << "samples from each waveforms: " << nSample
    << "\n" << indent << "pedestal range: +/- " << nRMS << " x RMS"
    << "\n" << indent << "use only waveforms with less than "
      << nExcessSamples << " samples out of pedestal range"
    ;
} // opdet::SharedWaveformBaseline::Params_t::dump()


//------------------------------------------------------------------------------

#endif // ICARUSALG_PMT_ALGORITHMS_SHAREDWAVEFORMBASELINE_H
