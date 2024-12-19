#ifndef ENUM_H_INCLUDE
#define ENUM_H_INCLUDE


#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>


/**
 * C++23 gives std::to_underlying (see: https://en.cppreference.com/w/cpp/utility/to_underlying)
 * This function is the same.
 */
template <typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(e);
}


/**
 * A generic function for enum serialization.
 * This implementation is the fallback one which'll only get selected if you try
 * to serialize an enumeration which has no serialization defined, and then fail to compile.
 * Since these functions work by type matching, they are designed for use with scoped enums (enum class)
 */
template <typename Enum>
const char* const serialise_enum(const Enum e)
{
  static_assert(std::is_enum<Enum>::value, "serialise_enum is for enums");
  static_assert(std::is_same<Enum, void>::value, "No serialise_enum for enum provided");

  // throw an exception to stop the compiler complaining about no return value
  // throw std::invalid_argument("No serialise_enum for enum provided");
  return "";
}


/**
 * A generic function for enum deserialization.
 * This is the counter part of serialise_enum, see that for details.
 */
template <typename Enum>
Enum deserialise_enum(const char* const str)
{
  static_assert(std::is_enum<Enum>::value, "deserialise_enum is for enums");
  static_assert(std::is_same<Enum, void>::value, "No deserialise_enum for enum provided");

  // throw an exception to stop the compiler complaining about no return value
  // throw std::invalid_argument("No deserialise_enum for enum provided");
  return static_cast<Enum>(0);
}


/**
 * This macro is a utility helper for spitting out serialization and deserialization functions
 * for scoped enum types, later C++ editions will let us make this better.
 * Can use like `ENUM_SERIALISATION(MyEnum, {{MyEnum::ONE, "ONE"}, {MyEnum::TWO, "TWO"}})` and then use
 * `serializa_enum` and `deserialise_enum` with MyEnum
 */
#define ENUM_SERIALISATION(EnumType, ...) \
  template <> \
  inline const char* const serialise_enum(const EnumType e) \
  { \
    using Pair = std::pair<EnumType, const char* const>; \
    static const Pair m[] = __VA_ARGS__;  \
    for (const auto& p : m) \
    { \
      if (p.first == e) \
        return p.second; \
    } \
    return m[0].second; \
  } \
  template <> \
  inline EnumType deserialise_enum(const char* const str) \
  { \
    using Pair = std::pair<EnumType, const char* const>; \
    static const Pair m[] = __VA_ARGS__;  \
    for (const auto& p : m) \
    { \
      if (strcmp(p.second, str) == 0) \
        return p.first; \
    } \
    return m[0].first; \
  }

#endif
