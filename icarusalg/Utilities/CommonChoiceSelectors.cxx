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
//===  util::SignalPolarity  ===================================================
//==============================================================================
//------------------------------------------------------------------------------
//--- util::StandardSelectorFor<SignalPolarity>
//------------------------------------------------------------------------------
namespace util {
  
  StandardSelectorFor<util::SignalPolarity>::StandardSelectorFor()
    : MultipleChoiceSelection<util::SignalPolarity>{
        { SignalPolarity::Negative, "Negative", "neg" }
      , { SignalPolarity::Positive, "Positive", "pos" }
      }
    {}
}

//------------------------------------------------------------------------------
::fhicl::detail::ps_atom_t util::encode(SignalPolarity const& value) {
  return util::details::encodeEnumClassToFHiCL(value);
}

//------------------------------------------------------------------------------
void util::decode(std::any const& src, SignalPolarity& value) {
  util::details::decodeEnumClassToFHiCL(src, value);
}

//------------------------------------------------------------------------------
