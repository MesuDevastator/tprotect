// frequency_analyzer.hpp: Letter Frequency Analyzer for Cipher Breaking

#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace tprotect::cipher
{
struct letter_frequency
{
    char letter;
    int count;
    float percentage;

    bool operator>(const letter_frequency &other) const
    {
        return count > other.count;
    }
};

class frequency_analyzer
{
  public:
    /**
     * @brief Analyze letter frequency in the given text
     *
     * @param text The text to analyze
     * @param case_sensitive If true, 'A' and 'a' are counted separately
     * @return std::vector<letter_frequency> Sorted by frequency (descending)
     */
    [[nodiscard]] static std::vector<letter_frequency> analyze(std::string_view text,
                                                               bool case_sensitive = false) noexcept
    {
        // Count frequencies
        std::array<int, 52> counts{}; // A-Z (0-25), a-z (26-51)
        int total_letters{};

        for (const char ch : text)
        {
            if (ch >= 'A' && ch <= 'Z')
            {
                counts[ch - 'A']++;
                total_letters++;
            }
            else if (ch >= 'a' && ch <= 'z')
            {
                if (case_sensitive)
                {
                    counts[26 + (ch - 'a')]++;
                }
                else
                {
                    counts[ch - 'a']++;
                }
                total_letters++;
            }
        }

        // Build result vector
        std::vector<letter_frequency> result;
        result.reserve(52);

        const int letter_range{case_sensitive ? 52 : 26};
        for (int i{}; i < letter_range; ++i)
        {
            if (counts[i] > 0)
            {
                char letter;
                if (i < 26)
                {
                    letter = static_cast<char>('A' + i);
                }
                else
                {
                    letter = static_cast<char>('a' + (i - 26));
                }

                const float percentage{total_letters > 0
                                           ? (static_cast<float>(counts[i]) * 100.f / static_cast<float>(total_letters))
                                           : 0.f};
                result.push_back({letter, counts[i], percentage});
            }
        }

        // Sort by frequency (descending)
        std::sort(result.begin(), result.end(), std::greater<>());

        return result;
    }

    /**
     * @brief Get standard English letter frequencies for comparison
     *
     * @return std::array<float, 26> Frequency percentages for A-Z
     */
    [[nodiscard]] static constexpr std::array<float, 26> get_english_frequencies() noexcept
    {
        // Standard English letter frequencies (approximate)
        return {
            8.17f,  // A
            1.49f,  // B
            2.78f,  // C
            4.25f,  // D
            12.70f, // E
            2.23f,  // F
            2.02f,  // G
            6.09f,  // H
            6.97f,  // I
            0.15f,  // J
            0.77f,  // K
            4.03f,  // L
            2.41f,  // M
            6.75f,  // N
            7.51f,  // O
            1.93f,  // P
            0.10f,  // Q
            5.99f,  // R
            6.33f,  // S
            9.06f,  // T
            2.76f,  // U
            0.98f,  // V
            2.36f,  // W
            0.15f,  // X
            1.97f,  // Y
            0.07f   // Z
        };
    }
};
} // namespace tprotect::cipher
