/**
 * @file   icarusalg/Utilities/CommonChoiceSelectors.cxx
 * @brief  Selector implementations for some enumerator data types.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 18, 2023
 * @see    icarusalg/Utilities/CommonChoiceSelectors.h
 */


// library header
#include "icarusalg/Utilities/CommonChoiceSelectors.h"

// framework libraries
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Atom.h"

// C/C++ standard libraries
#include <string>
#include <set>


//==============================================================================
//===  util::TimeScale  ========================================================
//==============================================================================
//------------------------------------------------------------------------------
//--- util::StandardSelectorFor<TimeScale>
//------------------------------------------------------------------------------
namespace util {
  
  StandardSelectorFor<util::TimeScale>::StandardSelectorFor()
    : MultipleChoiceSelection<util::TimeScale>{
        { TimeScale::Electronics, "Electronics", "ElectronicsTime" }
      , { TimeScale::Trigger,     "Trigger",     "TriggerTime" }
      , { TimeScale::BeamGate,    "BeamGate",    "Beam", "BeamGateTime" }
      , { TimeScale::Simulation,  "Simulation",  "SimulationTime" }
      }
    {}
}

//------------------------------------------------------------------------------
::fhicl::detail::ps_atom_t util::encode(TimeScale const& value) {
  return util::details::encodeEnumClassToFHiCL(value);
}

//------------------------------------------------------------------------------
void util::decode(std::any const& src, TimeScale& value) {
  util::details::decodeEnumClassToFHiCL(src, value);
}

//==============================================================================
//===  sbn::BeamType ===========================================================
//==============================================================================
//------------------------------------------------------------------------------
//--- util::StandardSelectorFor<BeamType>
//------------------------------------------------------------------------------
namespace util {
  
  StandardSelectorFor<sbn::BeamType>::StandardSelectorFor()
    : MultipleChoiceSelection<sbn::BeamType>{
        { sbn::BeamType::kUnknown, "unknown" }
      , { sbn::BeamType::kBNB,     "BNB" }
      , { sbn::BeamType::kNuMI,    "NuMI" }
      }
    {}
  
}

//------------------------------------------------------------------------------
::fhicl::detail::ps_atom_t sbn::encode(BeamType const& value) {
  return ::util::details::encodeEnumClassToFHiCL(value);
}

//------------------------------------------------------------------------------
void sbn::decode(std::any const& src, BeamType& value) {
  ::util::details::decodeEnumClassToFHiCL(src, value);
}

//------------------------------------------------------------------------------
