// substitution_cipher.hpp: The Substitution Cipher Implementation

#pragma once

#include <cstring>
#include <expected>
#include <map>
#include <string>
#include <string_view>

namespace tprotect::cipher
{
class substitution_cipher
{
  public:
    explicit substitution_cipher(const std::string_view mapping) noexcept
    {
        set_key(mapping);
    }

    [[nodiscard]] std::expected<std::string, std::string> encrypt(const std::string_view input) const noexcept
    {
        std::string result;
        result.reserve(input.size());

        for (const char ch : input)
        {
            if (const auto it{encryption_map_.find(ch)}; it != encryption_map_.end())
            {
                result += it->second;
            }
            else
            {
                result += ch; // non-characters remain the same
            }
        }
        return result;
    }

    [[nodiscard]] std::expected<std::string, std::string> decrypt(const std::string_view input) const noexcept
    {
        std::string result;
        result.reserve(input.size());

        for (const char ch : input)
        {
            if (const auto it{decryption_map_.find(ch)}; it != decryption_map_.end())
            {
                result += it->second;
            }
            else
            {
                result += ch;
            }
        }
        return result;
    }

    void set_key(const std::string_view mapping) noexcept
    {
        constexpr auto alphabet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};

        for (size_t i{}; i < std::strlen(alphabet); ++i)
        {
            encryption_map_[alphabet[i]] = mapping[i % mapping.size()];
            decryption_map_[mapping[i % mapping.size()]] = alphabet[i];
        }
    }

  private:
    std::map<char, char> encryption_map_;
    std::map<char, char> decryption_map_;
};
} // namespace tprotect::cipher
