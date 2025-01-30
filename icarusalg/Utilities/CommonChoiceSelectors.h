/**
 * @file   icarusalg/Utilities/CommonChoiceSelectors.h
 * @brief  Selector implementations for some enumerator data types.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 18, 2023
 * @see    icarusalg/Utilities/CommonChoiceSelectors.cxx
 */


#ifndef ICARUSALG_UTILITIES_COMMONCHOICESELECTORS_H
#define ICARUSALG_UTILITIES_COMMONCHOICESELECTORS_H

// SBN libraries
#include "icarusalg/Utilities/StandardSelectorFor.h"

/// LArSoft libraries
#include "lardataalg/Utilities/MultipleChoiceSelection.h"

/// framework libraries
#include "fhiclcpp/types/Atom.h"

// C/C++ standard library
#include <type_traits> // std::enable_if_t


// -----------------------------------------------------------------------------
// --- BEGIN --- util::TimeScale -----------------------------------------------
// -----------------------------------------------------------------------------
/// @name util::TimeScale
/// @{

namespace util {
  
  /**
   * @brief Expresses the choice of a time scale.
   * 
   * Time scales are documented in @ref DetectorClocksTimeDefinitions "LArSoft".
   * The more friendly way to manipulate them is arguably via the
   * `detinfo::DetectorTimings` helper.
   * 
   * This enumerator lists possible time scales for use in the code and in
   * configuration (see `util::TimeScaleSelector`).
   * 
   */
  enum class TimeScale: unsigned int {
    Electronics, ///< @ref DetectorClocksElectronicsTime "Electronics time".
    Trigger,     ///< @ref DetectorClocksTriggerTime "Hardware trigger time".
    BeamGate,    ///< @ref DetectorClocksBeamGateTime "Beam gate opening time".
    Simulation,  ///< @ref DetectorClocksSimulationTime "Simulation time".
    
    NTimes,      ///< Number of supported reference times.
    Default = Electronics ///< LArSoft "default".
  }; // TimeScale
  
  
  // ---------------------------------------------------------------------------
  /// Helper for generic encoding of `TimeScale` enumerator in FHiCL.
  ::fhicl::detail::ps_atom_t encode(TimeScale const& value);

  /// Helper for generic decoding of `TimeScale` enumerator in FHiCL.
  void decode(std::any const& src, TimeScale& value);
  
  // ---------------------------------------------------------------------------
  /// Selector for `util::TimeScale` enumerator (template specialization).
  template <>
  struct StandardSelectorFor<TimeScale> // standard: `Tag` == `0`
    : public MultipleChoiceSelection<util::TimeScale>
  {
    /// Constructor: initializes with some appropriate strings.
    StandardSelectorFor();
  }; // StandardSelectorFor<TimeScale>
  
  // ---------------------------------------------------------------------------
  
} // util

namespace fhicl {
  
  /// Specialization of fhicl::Atom<>
  template <>
  struct Atom<::util::TimeScale>: SelectorAtom<::util::TimeScale> {
    using SelectorAtom<::util::TimeScale>::SelectorAtom;
  };
  
}


/// @}

// --- END ----- util::TimeScale -----------------------------------------------
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --- BEGIN --- util::SignalPolarity ------------------------------------------
// -----------------------------------------------------------------------------
/// @name util::SignalPolarity
/// @{

namespace util {
  
  /**
   * @brief Expresses the polarity of an analog signal.
   * 
   * This enumerator lists possible signal polarities for use in the code and in
   * configuration (see `util::SignalPolaritySelector`).
   * 
   */
  enum class SignalPolarity: int {
    Negative = -1, ///< Stronger signals develop with more negative voltages.
    Positive = +1, ///< Stronger signals develop with more positive voltages.
    
    UnknownPolarity ///< Unknown or unspecified polarity.
  }; // SignalPolarity
  
  
  // ---------------------------------------------------------------------------
  /// Helper for generic encoding of `SignalPolarity` enumerator in FHiCL.
  ::fhicl::detail::ps_atom_t encode(SignalPolarity const& value);

  /// Helper for generic decoding of `SignalPolarity` enumerator in FHiCL.
  void decode(std::any const& src, SignalPolarity& value);
  
  // ---------------------------------------------------------------------------
  /// Selector for `util::SignalPolarity` enumerator (template specialization).
  template <>
  struct StandardSelectorFor<SignalPolarity> // standard: `Tag` == `0`
    : public MultipleChoiceSelection<util::SignalPolarity>
  {
    /// Constructor: initializes with some appropriate strings.
    StandardSelectorFor();
  }; // StandardSelectorFor<SignalPolarity>
  
  // ---------------------------------------------------------------------------
  
} // util

namespace fhicl {
  
  /// Specialization of fhicl::Atom<>
  template <>
  struct Atom<::util::SignalPolarity>: SelectorAtom<::util::SignalPolarity> {
    using SelectorAtom<::util::SignalPolarity>::SelectorAtom;
  };
  
}


/// @}

// --- END ----- util::SignalPolarity  -----------------------------------------
// -----------------------------------------------------------------------------

#endif // ICARUSCODE_UTILITIES_COMMONCHOICESELECTORS_H
