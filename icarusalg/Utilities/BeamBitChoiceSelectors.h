/**
 * @file   icarusalg/Utilities/BeamBitChoiceSelectors.h
 * @brief  Selector implementations for `sbn::bits` enumerator types.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   December 11, 2025
 * 
 * This library is header only.
 */


#ifndef ICARUSALG_UTILITIES_BEAMBITCHOICESELECTORS_H
#define ICARUSALG_UTILITIES_BEAMBITCHOICESELECTORS_H

// ICARUS/SBN libraries
#include "icarusalg/Utilities/StandardSelectorFor.h"
#include "sbnobj/Common/Trigger/BeamBits.h"

/// LArSoft libraries
#include "lardataalg/Utilities/MultipleChoiceSelection.h"
#include "larcorealg/CoreUtils/counter.h"

/// framework libraries
#include "fhiclcpp/types/Atom.h"

// C/C++ standard library
#include <type_traits> // std::enable_if_t, ...
#include <utility> // std::move()


// -----------------------------------------------------------------------------
namespace sbn::bits::details {
  
  /// Type trait: T is an enum class type.
  template <typename T, typename = void>
  struct is_enum_class: std::false_type {};
  
  template<typename T>
  struct is_enum_class<T, std::enable_if_t<std::is_enum_v<T>>>
    : std::negation<std::is_convertible<T, std::underlying_type_t<T>>>
    {};

  template<typename T>
  inline constexpr bool is_enum_class_v = is_enum_class<T>();
  
  
  /// Type trait: the result of `bitName(T::NBits)` is like `std::string`.
  template <typename T, typename = void>
  struct has_bitName: std::false_type {};
  
  template <typename T>
  struct has_bitName<T, std::void_t<decltype(bitName(std::declval<T>()))>>
    : std::is_convertible<decltype(bitName(T::NBits)), std::string>
    {};
  
  template<typename T>
  inline constexpr bool has_bitName_v = has_bitName<T>();
  
} // sbn::bits::details


// -----------------------------------------------------------------------------
/**
 * @name Beam bits (from `sbn::bits`)
 * 
 * The customization for `util::MultipleChoiceSelection` includes three parts:
 * 1. a class helper associating enumerator values to their names;
 * 2. the two low level FHiCL conversion functions using that helper;
 * 3. the `fhicl::Atom` specialization for the enumerator.
 * 
 * This set of utilities is meant to provide this customization for all
 * enumerator classes in `sbn::bits`.
 * 
 * The first, the class helper, is a specialization of `StandardSelectorFor`
 * and it is written so that it works for any enumerator class type as long as
 * it has a `NBits` value and a `bitName()` function
 * (that is the enumerators that satisfy `sbn::bits::is_beambit_v`). Great.
 * 
 * @note The special name "empty string" is associated to `NBits` (unless is
 *       the actual name of another enumerator value).
 * 
 * The conversion functions are written so that they work for those enumerators
 * too. However, the functions themselves are defined in `sbn::bits`, so they
 * will be visible via argument-dependent lookup only when their enumerator
 * argument is defined in that same namespace, and they can't be defined as
 * templates because they conflict with existing ones. Not that great.
 * 
 * Finally, the `fhicl::Atom` class can't be partially specialized (that is,
 * specialized for a subset of classes rather than for a single class) without
 * the concept feature of C++20 since it has a single template parameter.
 * So for that we still need to define one specialization for each supported
 * enumerator class. Not great at all.
 */
/// @{

namespace sbn::bits {
  
  /// Type trait: true if `T` is a beam bit or similar
  /// (an enum class with `NBits` which is known by a `bitName()` function).
  template<typename T>
  struct is_beambit
    : std::conjunction<details::is_enum_class<T>, details::has_bitName<T>>
    {};
  
  template<typename T>
  inline constexpr bool is_beambit_v = is_beambit<T>::value;
  
  //------------------------------------------------------------------------------

} // sbn::bits


namespace util {
  
  // ---------------------------------------------------------------------------
  /// Selector for any enumerator class with `is_beambit_v` (template specialization).
  template <typename BeamBitEnum, std::size_t Tag>
  struct StandardSelectorFor
    <BeamBitEnum, Tag, std::enable_if_t<sbn::bits::is_beambit_v<BeamBitEnum>>>
    : public MultipleChoiceSelection<BeamBitEnum>
  {
    /// Constructor: initializes with bits from `0` to `NBits`.
    StandardSelectorFor();
  }; // StandardSelectorFor<sbn::bits::triggerLogic>
  
  // ---------------------------------------------------------------------------
  
} // util


// --- BEGIN --- sbn::bits::triggerSource  -------------------------------------

namespace sbn::bits {
  
  inline fhicl::detail::ps_atom_t encode(triggerSource const& value)
    { return ::util::details::encodeEnumClassToFHiCL(value); }
  
  inline void decode(std::any const& src, triggerSource& value)
    { ::util::details::decodeEnumClassToFHiCL(src, value); }
  
} // sbn::bits

namespace fhicl {
  
  /// Specialization of `fhicl::Atom` for `sbn::bits::triggerSource`.
  template <>
  struct Atom<::sbn::bits::triggerSource>
    : SelectorAtom<::sbn::bits::triggerSource>
  {
    using SelectorAtom<::sbn::bits::triggerSource>::SelectorAtom;
  };
  
} // fhicl

// ---- END ---- sbn::bits::triggerSource  -------------------------------------


// --- BEGIN --- sbn::bits::triggerLogic  --------------------------------------

namespace sbn::bits {
  
  inline fhicl::detail::ps_atom_t encode(triggerLogic const& value)
    { return ::util::details::encodeEnumClassToFHiCL(value); }
  
  inline void decode(std::any const& src, triggerLogic& value)
    { ::util::details::decodeEnumClassToFHiCL(src, value); }
  
} // sbn::bits
  
namespace fhicl {
  
  /// Specialization of `fhicl::Atom` for `sbn::bits::triggerLogic`.
  template <>
  struct Atom<::sbn::bits::triggerLogic>
    : SelectorAtom<::sbn::bits::triggerLogic>
  {
    using SelectorAtom<::sbn::bits::triggerLogic>::SelectorAtom;
  };
  
  
} // fhicl

// ---- END ---- sbn::bits::triggerLogic  --------------------------------------


// --- BEGIN --- sbn::bits::triggerLocation  -----------------------------------

namespace sbn::bits {
  
  inline fhicl::detail::ps_atom_t encode(triggerLocation const& value)
    { return ::util::details::encodeEnumClassToFHiCL(value); }
  
  inline void decode(std::any const& src, triggerLocation& value)
    { ::util::details::decodeEnumClassToFHiCL(src, value); }
  
} // sbn::bits

namespace fhicl {
  
  /// Specialization of `fhicl::Atom` for `sbn::bits::triggerLocation`.
  template <>
  struct Atom<::sbn::bits::triggerLocation>
    : SelectorAtom<::sbn::bits::triggerLocation>
  {
    using SelectorAtom<::sbn::bits::triggerLocation>::SelectorAtom;
  };
  
} // fhicl

// ---- END ---- sbn::bits::triggerLocation  -----------------------------------


// --- BEGIN --- sbn::bits::triggerType  ---------------------------------------

namespace sbn::bits {
  
  inline fhicl::detail::ps_atom_t encode(triggerType const& value)
    { return ::util::details::encodeEnumClassToFHiCL(value); }
  
  inline void decode(std::any const& src, triggerType& value)
    { ::util::details::decodeEnumClassToFHiCL(src, value); }
  
} // sbn::bits

namespace fhicl {
  
  /// Specialization of `fhicl::Atom` for `sbn::bits::triggerType`.
  template <>
  struct Atom<::sbn::bits::triggerType>
    : SelectorAtom<::sbn::bits::triggerType>
  {
    using SelectorAtom<::sbn::bits::triggerType>::SelectorAtom;
  };
  
} // fhicl

// ---- END ---- sbn::bits::triggerType  ---------------------------------------


// --- BEGIN --- sbn::bits::gateSelection  -------------------------------------

namespace sbn::bits {
  
  inline fhicl::detail::ps_atom_t encode(gateSelection const& value)
    { return ::util::details::encodeEnumClassToFHiCL(value); }
  
  inline void decode(std::any const& src, gateSelection& value)
    { ::util::details::decodeEnumClassToFHiCL(src, value); }
  
} // sbn::bits

namespace fhicl {
  
  /// Specialization of `fhicl::Atom` for `sbn::bits::gateSelection`.
  template <>
  struct Atom<::sbn::bits::gateSelection>
    : SelectorAtom<::sbn::bits::gateSelection>
  {
    using SelectorAtom<::sbn::bits::gateSelection>::SelectorAtom;
  };
  
} // namespace fhicl

// ---- END ---- sbn::bits::gateSelection  -------------------------------------


// -----------------------------------------------------------------------------
// --- Template implementation
// -----------------------------------------------------------------------------
template <typename BeamBitEnum, std::size_t Tag>
util::StandardSelectorFor<BeamBitEnum, Tag,
  std::enable_if_t<sbn::bits::is_beambit_v<BeamBitEnum>>
  >::StandardSelectorFor()
{
  constexpr auto NBits
    = static_cast<std::underlying_type_t<BeamBitEnum>>(BeamBitEnum::NBits);
  bool hasEmpty = false;
  for (auto const bitValue: util::counter(NBits)) {
    BeamBitEnum bit = BeamBitEnum{ bitValue };
    std::string name = bitName(bit);
    if (name.empty()) hasEmpty = true;
    MultipleChoiceSelection<BeamBitEnum>::addOption(bit, std::move(name));
  } // for
  if (!hasEmpty)
    MultipleChoiceSelection<BeamBitEnum>::addOption(BeamBitEnum::NBits, "");
} // util::StandardSelectorFor<BeamBitEnum>::StandardSelectorFor()


//------------------------------------------------------------------------------
template <typename BeamBitEnum>
std::enable_if_t<sbn::bits::is_beambit_v<BeamBitEnum>>
sbn::bits::decode(std::any const& src, BeamBitEnum& value) {
  ::util::details::decodeEnumClassToFHiCL(src, value);
}


/// @}
//------------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_BEAMBITCHOICESELECTORS_H
