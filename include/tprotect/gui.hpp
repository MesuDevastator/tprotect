// gui.hpp: Dear ImGui User Interface Manager

#pragma once

#include <atomic>
#include <expected>
#include <mutex>
#include <string>

#include <tprotect/cipher/substitution_cipher.hpp>
#include <tprotect/cipher/transposition_cipher.hpp>
#include <tprotect/global.hpp>

struct GLFWwindow;

namespace tprotect
{
/**
 * @brief The GUI Manager
 *
 * The singleton should be acquired by `gui::instance()`
 *
 */
class gui final
{
  public:
    using result_type = std::expected<void, std::string>; // the `std::string` holds error messages

    /**
     * @brief Acquire the singleton
     *
     * @return gui& the singleton
     */
    static gui &instance() noexcept;

    /**
     * @brief Initialize the singleton
     *
     * @param[in] title is intentionally made `std::string`, which could prevent a potential copy
     *
     * @return result_type the result
     */
    [[nodiscard]] static result_type create(int width, int height, std::string title) noexcept;

    /**
     * @brief Destroy the singleton
     */
    static void destroy() noexcept;

    /**
     * @brief Get whether the singleton is initialized
     *
     * This function is thread-safe
     *
     */
    bool is_initialized() const noexcept;

    [[nodiscard]] gui::result_type main_loop() noexcept;

    ~gui();

    // Disable copying and moving
    gui(const gui &) noexcept = delete;
    gui &operator=(const gui &) noexcept = delete;
    gui(gui &&) noexcept = delete;
    gui &operator=(gui &&) noexcept = delete;

  private:
    gui() noexcept = default; // keep the constructor private

    [[nodiscard]] result_type initialize(int width, int height, std::string title) noexcept; // the actual initializer
    void shutdown() noexcept;
    void render() noexcept;                       // render the gui
    [[nodiscard]] result_type process() noexcept; // display dialogs

    std::mutex main_loop_mutex_;
    std::string title_; // save it to ensure its validity
    GLFWwindow *window_{};

    // UI state
    enum class cipher
    {
        automatic,
        substitution,
        transposition
    };
    std::string encrypted_text_;
    std::string decrypted_text_;
    cipher selected_cipher_{cipher::automatic};
    tprotect::cipher::substitution_cipher substitution_cipher{initial_mapping};
    tprotect::cipher::transposition_cipher transposition_cipher{initial_key};
    int transposition_key{initial_key};

    std::atomic<bool> is_initialized_; // `std::atomic<bool>` for thread safety
    bool should_exit_{};
};
} // namespace tprotect
