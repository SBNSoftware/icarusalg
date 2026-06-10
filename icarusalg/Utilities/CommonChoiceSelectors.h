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
#include "lardataobj/Simulation/BeamTypes.h"

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

  /// Helper for generic deecoding of `TimeScale` enumerator in FHiCL.
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
// --- BEGIN --- sbn::BeamType -------------------------------------------------
// -----------------------------------------------------------------------------
/// @name sbn::BeamType
/// @{

namespace sbn {
  
  /**
   * @brief Expresses the choice of a beam type.
   * 
   * The values of the standard beams are directly taken from `sim::BeamType_t`.
   * 
   * This enumerator lists possible time scales for use in the code and in
   * configuration (see `sbn::BeamTypeSelector`).
   * 
   */
  enum class BeamType { // we need to translate enum into a strong type
      kUnknown = sim::kUnknown
    , kBNB     = sim::kBNB
    , kNuMI    = sim::kNuMI
  }; // BeamType
  
  
  // ---------------------------------------------------------------------------
  /// Helper for generic encoding of `sbn::BeamType` enumerator in FHiCL.
  ::fhicl::detail::ps_atom_t encode(BeamType const& value);

  /// Helper for generic deecoding of `sbn::BeamType` enumerator in FHiCL.
  void decode(std::any const& src, BeamType& value);
  
} // namespace sbn

namespace util {
  // ---------------------------------------------------------------------------
  /// Selector for `sbn::BeamType` enumerator (template specialization).
  template <>
  struct StandardSelectorFor<sbn::BeamType> // standard: `Tag` == `0`
    : public MultipleChoiceSelection<sbn::BeamType>
  {
    /// Constructor: initializes with some appropriate strings.
    StandardSelectorFor();
  }; // StandardSelectorFor<sbn::BeamType>
  
  // ---------------------------------------------------------------------------
  
} // util

namespace fhicl {
  
  /// Specialization of fhicl::Atom<>
  template <>
  struct Atom<::sbn::BeamType>: SelectorAtom<::sbn::BeamType> {
    using SelectorAtom<::sbn::BeamType>::SelectorAtom;
  };
  
}


/// @}

// --- END ----- sbn::BeamType -----------------------------------------------
// -----------------------------------------------------------------------------

#endif // ICARUSCODE_UTILITIES_COMMONCHOICESELECTORS_H
