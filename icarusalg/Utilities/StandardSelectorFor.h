/**
 * @file   icarusalg/Utilities/StandardSelectorFor.h
 * @brief  Selector infrastructure for some enumerator data types.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 18, 2023
 * @see    icarusalg/Utilities/CommonChoiceSelectors.h
 */


#ifndef ICARUSALG_UTILITIES_STANDARDSELECTORFOR_H
#define ICARUSALG_UTILITIES_STANDARDSELECTORFOR_H


/// LArSoft libraries
#include "lardataalg/Utilities/MultipleChoiceSelection.h"

/// framework libraries
#include "fhiclcpp/types/Atom.h"

// C/C++ standard library
#include <any>
#include <string>
#include <type_traits> // std::enable_if_t
#include <cstddef> // std::size_t


namespace util {
  
  // --- BEGIN ---  Enumerator selector implementation helpers -----------------
  /**
   * @name Enumerator selector implementation helpers
   * 
   * These helpers streamline some aspects of the implementation of FHiCL
   * objects to read enumerators.
   * 
   * The enumerator, commonly labelled as `EnumClass`, must be _scoped_
   * (`enum class`), so that its values carry a specific type (unscoped `enum`
   * values are immediately converted to integral values).
   * 
   * The idea is to be able to just write things like:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * struct Config {
   * 
   *   fhicl::Atom<ns::ValueTypeEnum> ValueType {
   *     fhicl::Name{ "ValueType" },
   *     fhicl::Comment{ "type of value" },
   *     ns::ValueTypeEnum::Integral
   *     };
   * 
   * };
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * and then read it as:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * ns::ValueTypeEnum valueType = config.ValueType();
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * This requires the code to know how to convert a string into a enumerator
   * value and vice versa. This is usually done via `encode()` and `decode()`
   * free functions that the FHiCL library looks for. In fact, the library
   * deliberately does not specify which namespace to find them in: the
   * expectation is that they are available in the same namespace as the objects
   * they code (the alternatives would be in the `::fhicl` namespace, which we
   * consider reserved to the library, and the global namespace, which we don't
   * want to populate). Since we don't require the enumerators to be in any
   * specific namespace, the two functions must be provided by the user.
   * This library provides an implementation that can be directly used:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * ::fhicl::detail::ps_atom_t ns::encode(ValueTypeEnum const& value) {
   *   return ::util::encodeEnumClassToFHiCL(value);
   * }
   * 
   * void ns::decode(std::any const& src, ValueTypeEnum& value) {
   *   ::util::decodeEnumClassToFHiCL(src, value);
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * The two helpers here utilize indirectly objects of type
   * `util::MultipleChoiceSelection<EnumClass>` to implement.
   * These objects require the names of the enumerator values at initialization
   * time, so they can't be written in generic code.
   * The helpers above rely on an object of type `util::StandardSelectorFor`
   * (in namespace `util`) which acts like a fully-initialized
   * `util::MultipleChoiceSelection` object. For example:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * namespace util {
   *   template <>
   *   struct StandardSelectorFor<ns::ValueTypeEnum>
   *     : public util::MultipleChoiceSelection<ns::ValueTypeEnum>
   *   {
   *     StandardSelectorFor()
   *       : MultipleChoiceSelection<ns::ValueTypeEnum>{
   *           { ns::ValueTypeEnum::Integer,    "Integer",    "int" }
   *         , { ns::ValueTypeEnum::NonInteger, "NonInteger", "other" }
   *         }
   *       {}
   *   };
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * Different versions of `util::StandardSelectorFor` can be specified for the
   * same enumerator by changing the template parameter `Tag` (which is not
   * shown in the example and defaults to `0`) for both the class and the
   * `encode()`/`decode()` functions.
   * 
   * Note that the type of override of `util::StandardSelectorFor` required here
   * is a template specialization. The specialization needs to be visible to the
   * code before the point where it's needed, or else a less specialized version
   * of the class will be used; in this case, the generic
   * `util::StandardSelectorFor` is written to generate a compilation error,
   * although the error message will be not so straightforward to interpret.
   * 
   */
  /// @{
  
  /**
   * @brief A selector specific for `EnumClass`.
   * @tparam EnumClass enumerator type the selector wraps around
   * @tparam Tag (default: `0`) a value to distinguish different selectors
   * 
   * A `StandardSelectorFor` is a class derived from
   * `util::MultipleChoiceSelection` which contains a construction specific for
   * `EnumClass` enumerator type.
   * 
   * The idea is that generic code can initialize a `StandardSelectorFor<>`
   * object and expect it correctly set up.
   * 
   * @note Because the initialization is specific to `EnumClass`, the generic
   *       template can't work and it is provided here only to document the
   *       "interface". Specializations are _required_ for each `EnumClass`
   *       that needs support, and this specialization may reside in their
   *       own header which won't be automatically included together with
   *       this class. If no suitable specialization is found, this template
   *       will be tried in `encodeEnumClassToFHiCL()` etc., which will cause
   *       a compilation error about failing to match the template arguments
   *       of `StandardSelectorFor` or having `hasStandardSelector_v` false.
   *       In that case, make sure that the header file with the declaration of
   *       the specialization is included before it's needed.
   * 
   * An additional template parameter, `Tag`, is provided in case generic code
   * wants to support different types of selectors for the same type (for
   * example, some excluding part of the available enumerator values).
   * The "standard" `Tag` value `0` is supposed to be... well, "standard".
   */
  template <typename EnumClass, std::size_t Tag = 0>
  struct StandardSelectorFor { static constexpr bool isEnumSupported = false; };
  
  template <typename EnumClass, std::size_t Tag = 0, typename Enable = void>
  struct hasStandardSelector_t: std::true_type {};
  
  /// Whether `EnumType` has a specialised standard selector class.
  template <typename EnumType, std::size_t Tag = 0>
  constexpr bool hasStandardSelector_v = hasStandardSelector_t<EnumType, Tag>();
  
  
  namespace details {
    
    /**
     * @brief Encodes a enumerator value for a FHiCL atom.
     * 
     * This is the recommended hook/implementation of an actual `encode()`
     * function:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * namespace myns {
     *   auto encode(myns::MyEnumClass const& value)
     *     { return util::details::encodeEnumClassToFHiCL(value); }
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * This indirection is needed so that FHiCL machinery can find the
     * `encode()` function in the same namespace where the enumerator is defined
     * (in the example, `myns`).
     */
    template <std::size_t Tag = 0, typename EnumClass>
    std::enable_if_t
      <util::hasStandardSelector_v<EnumClass, Tag>, ::fhicl::detail::ps_atom_t>
    encodeEnumClassToFHiCL(EnumClass const& value);
    
    template <std::size_t Tag = 0, typename EnumClass>
    std::enable_if_t<util::hasStandardSelector_v<EnumClass, Tag>>
    decodeEnumClassToFHiCL(std::any const& src, EnumClass& value);
    
  } // namespace details
  
  
  /// @}
  // --- END -----  Enumerator selector implementation helpers -----------------
} // namespace util


// -----------------------------------------------------------------------------


namespace fhicl {
  
  // ---------------------------------------------------------------------------
  /*
   * Because `fhiclcpp` classes are declared `final`, we can't customize them
   * by inheritance; overloading `encode()`/`decode()` helps but does not do
   * the full work (in particular the default value management is not covered).
   * Containment would require making sure that the containing class support
   * all the FHiCL validation machinery and then replicates all the needed
   * user interface.
   * 
   * So I end up specializing a class and inheriting from implementation details
   * which may change at any time.
   */
  template <typename T>
  class SelectorAtom
    : public detail::AtomBase
    , private detail::RegisterIfTableMember
  {
    static_assert(util::hasStandardSelector_v<T>);
      public:
    static_assert(!tt::is_sequence_type_v<T>, NO_STD_CONTAINERS);
    static_assert(!tt::is_fhicl_type_v<T>, NO_NESTED_FHICL_TYPES_IN_ATOM);
    static_assert(!tt::is_table_fragment_v<T>, NO_NESTED_TABLE_FRAGMENTS);
    static_assert(!tt::is_delegated_parameter_v<T>, NO_DELEGATED_PARAMETERS);
    
    //=====================================================
    // User-friendly
    // ... c'tors
    explicit SelectorAtom(Name&& name);
    explicit SelectorAtom(Name&& name, Comment&& comment);
    explicit SelectorAtom(Name&& name, Comment&& comment, std::function<bool()> useIf);

    // ... c'tors supporting defaults
    explicit SelectorAtom(Name&& name, T const& dflt_value);
    explicit SelectorAtom(Name&& name, Comment&& comment, T const& dflt_value);
    explicit SelectorAtom(
      Name&& name, Comment&& comment,
      std::function<bool()> useIf, T const& dflt_value
      );

    // ... Accessors
    auto const&
    operator()() const
    {
      return value_;
    }

    // Expert-only
    using default_type = T;
    using value_type = T;

  private:
    value_type value_{};

    static ::util::StandardSelectorFor<T> const selector;
    
    SelectorAtom(
      Name&& name, Comment&& comment, par_style const vt,
      std::function<bool()> maybeUse, T dflt_value = T{}
      );
    
    std::string get_stringified_value() const override;
    void do_set_value(fhicl::ParameterSet const& pset) override;
    
  }; // class SelectorAtom
  
} // fhicl


// =============================================================================
// ===  Template implementation
// =============================================================================
// -----------------------------------------------------------------------------
// --- FHiCL encoding/decoding implementation
// -----------------------------------------------------------------------------
/*
 * Somehow unusual, and I hope it works.
 * By default, all types are considered to have a standard selector.
 * The ones that can instantiate the following specialization are the exception.
 * All the ones which have `EnumClass` not an `enum class` fall in that category
 * (unfortunately until C++23 we won't know directly whether they are a scoped
 * or unscoped enumerator, so I am checking that it is an enumerator and that it
 * can't be converted into an integer, as in
 * https://stackoverflow.com/questions/15586163/c11-type-trait-to-differentiate-between-enum-class-and-regular-enum).
 * In addition, all the ones that are `enum class` and do have a specialization
 * of `util::StandardSelectorFor` which tells the enum is not supported also
 * fall in that category. Now, the main template does exactly that: says that
 * the argument is not supported (it also means that the check whether
 * `EnumClass` is an enumerator or not is redundant).
 * 
 * If on the other end there is a specialization of `util::StandardSelectorFor`
 * for a scoped enumerator `EnumClass`, and it does not specify
 * `isEnumSupported`, then that enumerator does not fall in this category of
 * unsupported enumerators. And if it does specify `isEnumSupported`, and
 * it converts to `false`... well, I don't know what to say.
 * Implementer's choice.
 */
template <typename EnumClass, std::size_t Tag>
struct util::hasStandardSelector_t<EnumClass, Tag,
  std::enable_if_t<
    !std::is_enum_v<EnumClass> || std::is_convertible_v<EnumClass, int>
    || !util::StandardSelectorFor<EnumClass>::isEnumSupported
  >>
  : std::false_type
{};


// -----------------------------------------------------------------------------
template <std::size_t Tag /* = 0 */, typename EnumClass>
std::enable_if_t
  <util::hasStandardSelector_v<EnumClass, Tag>, ::fhicl::detail::ps_atom_t>
util::details::encodeEnumClassToFHiCL(EnumClass const& value) {
  static util::StandardSelectorFor<EnumClass, Tag> const selector;
  return ::fhicl::detail::encode(selector.get(value).name());
} // util::details::encodeEnumClassToFHiCL()


// -----------------------------------------------------------------------------
template <std::size_t Tag /* = 0 */, typename EnumClass>
std::enable_if_t<util::hasStandardSelector_v<EnumClass, Tag>>
util::details::decodeEnumClassToFHiCL(std::any const& src, EnumClass& value) {
  std::string s;
  ::fhicl::detail::decode(src, s);
  static util::StandardSelectorFor<EnumClass, Tag> const selector;
  value = selector.parse(s).value();
} // util::details::decodeEnumClassToFHiCL()


// -----------------------------------------------------------------------------
// ---  fhicl::SelectorAtom<>
// -----------------------------------------------------------------------------
template <typename T>
util::StandardSelectorFor<T> const fhicl::SelectorAtom<T>::selector;

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom(Name&& name, Comment&& comment)
  : SelectorAtom{
    std::move(name), std::move(comment),
    par_style::REQUIRED, detail::AlwaysUse()
  }
  {}

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom
  (Name&& name, Comment&& comment, std::function<bool()> maybeUse)
  : SelectorAtom{
    std::move(name), std::move(comment),
    par_style::REQUIRED_CONDITIONAL, maybeUse
  }
  {}

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom
  (Name&& name, Comment&& comment, T const& dflt_value)
  : SelectorAtom{
    std::move(name), std::move(comment),
    par_style::DEFAULT, detail::AlwaysUse(), dflt_value
  }
  {}

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom(
  Name&& name, Comment&& comment,
  std::function<bool()> maybeUse, T const& dflt_value
)
  : SelectorAtom{
    std::move(name), Comment{ "" },
    par_style::DEFAULT_CONDITIONAL, maybeUse, dflt_value
  }
  {}

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom(Name&& name)
  : SelectorAtom{ std::move(name), Comment("") }
  {}

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom(Name&& name, T const& dflt_value)
  : SelectorAtom{ std::move(name), Comment(""), dflt_value }
  {}

// -----------------------------------------------------------------------------
template <typename T>
std::string fhicl::SelectorAtom<T>::get_stringified_value() const {
  return has_default()
    ? selector.get(value_).name()
    : detail::no_defaults::expected_types<T>{}.value
    ;
}

// -----------------------------------------------------------------------------
template <typename T>
fhicl::SelectorAtom<T>::SelectorAtom(
  Name&& name, Comment&& comment, par_style const vt,
  std::function<bool()> maybeUse, T dflt_value /* = T{} */
)
  : AtomBase{ std::move(name), std::move(comment), vt, maybeUse }
  , RegisterIfTableMember{this}
  , value_{ std::move(dflt_value) }
{
  NameStackRegistry::end_of_ctor();
}

// -----------------------------------------------------------------------------
template <typename T>
void fhicl::SelectorAtom<T>::do_set_value(fhicl::ParameterSet const& pset)
{
  auto const trimmed_key = detail::strip_first_containing_name(key());
  if (has_default()) {
    // Override default value if the key is present
    pset.get_if_present<T>(trimmed_key, value_);
  }
  else
    value_ = pset.get<T>(trimmed_key);
}


// -----------------------------------------------------------------------------

#endif // ICARUSALG_UTILITIES_STANDARDSELECTORFOR_H

// =============================================================================
