/**
 * @file libimp/system.h
 * @author mutouyun (orz@orzz.org)
 * @brief Isolation and encapsulation of system APIs
 * @date 2022-08-07
 */
#pragma once

#include <string>
#include <cstdint>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/result.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

/**
 * @brief Get/Set the system error code
 */
LIBIMP_EXPORT result_code error_code() noexcept;
LIBIMP_EXPORT void error_code(result_code) noexcept;

/**
 * @brief Gets a text description of the system error
 */
LIBIMP_EXPORT std::string error_str(result_code) noexcept;

/**
 * @brief A text description string with an error number attached
 */
LIBIMP_EXPORT std::string error_msg(result_code) noexcept;

/**
 * @brief Get system configuration information at run time
 */
enum class info : std::int32_t {
  page_size,
};
LIBIMP_EXPORT std::int64_t conf(info) noexcept;

} // namespace sys
LIBIMP_NAMESPACE_END_
