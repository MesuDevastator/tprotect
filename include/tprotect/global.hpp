// global.hpp: Global Constants And Definitions

#pragma once

#include <expected>
#include <optional>
#include <string>

namespace tprotect
{
constexpr const char *const initial_mapping{"IDvCtWclkuPZOgXshNwoBjSJzdxVMUpQGRrqEfLmbnTiFyaHeKAY"};
constexpr const int initial_key{42};

template <typename T> using eresult = std::expected<T, std::string>; // the `std::string` holds error messages
template <typename T> using oresult = std::optional<T>;
} // namespace tprotect
