// transposition_cipher.hpp: The Transposition Cipher Implementation

#pragma once

#include <cmath>
#include <expected>
#include <string>
#include <vector>

namespace tprotect::cipher
{
class transposition_cipher
{
  public:
    explicit transposition_cipher(const int key) noexcept
    {
        set_key(key);
    }

    [[nodiscard]] std::expected<std::string, std::string> encrypt(const std::string_view input) const noexcept
    {
        std::string result{};

        for (const char ch : input)
        {
            if (std::isalpha(static_cast<unsigned char>(ch)))
            {
                const char base{std::isupper(static_cast<unsigned char>(ch)) ? 'A' : 'a'};
                result.push_back(static_cast<char>((ch - base + key_) % 26 + base));
            }
            else
            {
                result.push_back(ch); // non alpha remains the same
            }
        }

        return result;
    }

    [[nodiscard]] std::expected<std::string, std::string> decrypt(const std::string_view input) const noexcept
    {
        std::string result{};

        for (const char ch : input)
        {
            if (std::isalpha(static_cast<unsigned char>(ch)))
            {
                const char base{std::isupper(static_cast<unsigned char>(ch)) ? 'A' : 'a'};
                int shifted{(ch - base - key_) % 26};
                if (shifted < 0)
                {
                    shifted += 26;
                }
                result.push_back(static_cast<char>(shifted + base));
            }
            else
            {
                result.push_back(ch); // non alpha remains the same
            }
        }

        return result;
    }

    void set_key(const int key) noexcept
    {
        key_ = std::abs(key) % 26;
    }

    // Attempt to use all the keys
    [[nodiscard]] static std::vector<std::string> decrypt_all_shifts(const std::string_view input) noexcept
    {
        std::vector<std::string> results;

        for (int shift{1}; shift <= 25; ++shift)
        {
            transposition_cipher cipher{shift};
            if (auto result{cipher.decrypt(input)}; result)
            {
                results.push_back(std::move(result.value()));
            }
        }
        return results;
    }

  private:
    int key_;
};
} // namespace tprotect::cipher
