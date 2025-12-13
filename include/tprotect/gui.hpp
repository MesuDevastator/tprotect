// gui.hpp: Dear ImGui User Interface Manager

#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <tprotect/cipher/substitution_cipher.hpp>
#include <tprotect/cipher/transposition_cipher.hpp>
#include <tprotect/global.hpp>

struct GLFWwindow;
struct ImFont;

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
     * @return result_type<void> the result
     */
    [[nodiscard]] static eresult<void> create(int width, int height, std::string title) noexcept;

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

    [[nodiscard]] eresult<void> main_loop() noexcept;

    ~gui();

    // Disable copying and moving
    gui(const gui &) noexcept = delete;
    gui &operator=(const gui &) noexcept = delete;
    gui(gui &&) noexcept = delete;
    gui &operator=(gui &&) noexcept = delete;

  private:
    gui() noexcept = default; // keep the constructor private

    [[nodiscard]] eresult<void> initialize(int width, int height,
                                           std::string title) noexcept; // the actual initializer
    void shutdown() noexcept;
    void render_window() noexcept;                       // render the gui
    [[nodiscard]] eresult<void> process_file() noexcept; // display dialogs

    std::mutex main_loop_mutex_;
    std::string title_; // save it to ensure its validity
    GLFWwindow *window_{};

    // UI state
    ImFont *noto_sans_regular{};
    ImFont *jetbrains_mono_regular{};
    enum class cipher
    {
        substitution,
        transposition,
    };
    std::string encrypted_text_;
    std::string decrypted_text_;
    cipher selected_cipher_{cipher::substitution};
    tprotect::cipher::substitution_cipher substitution_cipher{initial_mapping};
    tprotect::cipher::transposition_cipher transposition_cipher{initial_key};
    int transposition_key{initial_key};
    double fps_idle_{9.};
    bool is_idling_{};
    std::atomic<bool> is_initialized_; // `std::atomic<bool>` for thread safety
    bool should_exit_{};
};
} // namespace tprotect
