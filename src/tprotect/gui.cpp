// gui.cpp: Dear ImGui User Interface Manager

#include <fonts.hpp>
#include <tprotect/file_dialog.hpp>
#include <tprotect/gui.hpp>

#include <filesystem>
#include <ranges>

#include <imgui_additions.hpp>

#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

using namespace std::literals;

namespace tprotect
{
gui::~gui()
{
    if (is_initialized())
    {
        shutdown();
    }
}

gui &gui::instance() noexcept
{
    static gui instance{};
    return instance;
}

[[nodiscard]] eresult<void> gui::create(const int width, const int height, std::string title) noexcept
{
    return instance().initialize(width, height, std::move(title));
}

void gui::destroy() noexcept
{
    return instance().shutdown();
}

bool gui::is_initialized() const noexcept
{
    return is_initialized_.load(std::memory_order_acquire);
}

[[nodiscard]] eresult<void> gui::initialize(const int width, const int height, std::string title) noexcept
{
    if (bool expected{};
        !is_initialized_.compare_exchange_strong(expected, true, std::memory_order_release, std::memory_order_relaxed))
    {
        return std::unexpected{"GUI has already been initialized"};
    }

    title_ = std::move(title);

    // Initialize GLFW
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        return std::unexpected{std::format("Failed to initialize SDL: {}", SDL_GetError())};
    }

    // Create window
    const float main_scale{SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay())};
    window_ =
        SDL_CreateWindow(title_.c_str(), static_cast<int>(width * main_scale), static_cast<int>(height * main_scale),
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window_ == nullptr)
    {
        return std::unexpected{std::format("Failed to create SDL window: {}", SDL_GetError())};
    }

    // Create renderer
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr)
    {
        return std::unexpected{std::format("Failed to create SDL renderer: {}", SDL_GetError())};
    }
    SDL_SetRenderVSync(renderer_, 1);

    // Show window
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window_);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io{ImGui::GetIO()};
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;

    // Setup style
    ImGui::StyleColorsComfortableDark();

    // Setup scaling
    auto &style{ImGui::GetStyle()};
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Setup backends
    ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer3_Init(renderer_);

    // Setup fonts
    ImFontConfig font_cfg{};
    font_cfg.MergeMode = true;
    io.Fonts->AddFontFromMemoryCompressedTTF(futura_medium_compressed_data, sizeof futura_medium_compressed_data);
    futura_medium = io.Fonts->AddFontFromMemoryCompressedTTF(
        noto_sans_cjk_regular_compressed_data, sizeof noto_sans_cjk_regular_compressed_data, 0.f, &font_cfg);
    io.Fonts->AddFontFromMemoryCompressedTTF(jetbrains_mono_regular_compressed_data,
                                             sizeof jetbrains_mono_regular_compressed_data);
    jetbrains_mono_regular = io.Fonts->AddFontFromMemoryCompressedTTF(
        noto_sans_cjk_regular_compressed_data, sizeof noto_sans_cjk_regular_compressed_data, 0.f, &font_cfg);

    return {};
}

void gui::shutdown() noexcept
{
    if (bool expected{true};
        !is_initialized_.compare_exchange_strong(expected, false, std::memory_order_acq_rel, std::memory_order_relaxed))
    {
        return;
    }

    std::lock_guard<std::mutex> main_loop_guard_{main_loop_mutex_}; // prevent shutdown while the main loop is running

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (renderer_)
    {
        SDL_DestroyRenderer(renderer_);
        window_ = nullptr;
    }

    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

[[nodiscard]] eresult<void> gui::main_loop() noexcept
{
    if (!is_initialized() || !window_)
    {
        return std::unexpected{"GUI has not been initialized"};
    }
    std::lock_guard<std::mutex> main_loop_guard_{main_loop_mutex_}; // prevent shutdown while the main loop is running
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_BEGIN
#endif
    while (!should_exit_)
    {
#ifdef __EMSCRIPTEN__
        if (ShallIdleThisFrame_Emscripten(is_idling_))
        {
            continue;
        }
#else
        // Render idling
        is_idling_ = false;
        if (fps_idle_ > 0.)
        {
            const auto before_wait{std::chrono::steady_clock::now()};
            const double wait_expected{1. / fps_idle_};
            SDL_WaitEventTimeout(nullptr, wait_expected * 1000);
            const auto after_wait{std::chrono::steady_clock::now()};
            const double wait_duration{duration_cast<std::chrono::duration<double>>(after_wait - before_wait).count()};
            is_idling_ = wait_duration > wait_expected * 0.5;
        }
#endif

        // Handle events
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                should_exit_ = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window_))
            {
                should_exit_ = true;
            }
        }

        // Halt minimized render
        if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start a new frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Render the user draw list
        const auto viewport{ImGui::GetMainViewport()};
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        if (ImGui::Begin("TProtect", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            render_window();

            // Display and process dialogs
            std::string message{};
            if (auto result{process_file()}; !result)
            {
                ImGui::OpenPopup("Error Processing File");
                message = std::move(result.error());
            }

            ImGui::InformationPopup("Error Processing File", message.c_str(), [] {});
        }
        ImGui::End();

        // Render the frame
        ImGui::Render();
        auto &io{ImGui::GetIO()};
        SDL_SetRenderScale(renderer_, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer_, 0.f, 0.f, 0.f, 0.f);
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
        SDL_RenderPresent(renderer_);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    return {};
}

void gui::render_window() noexcept
{
    // Top title with larger font
    ImGui::PushFont(futura_medium, ImGui::GetFontSize() * 2.f);
    ImGui::TextCentered("TProtect");
    ImGui::PopFont();
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The Text Protector");
    }

    ImGui::Separator();

    std::string cipher_message{};

    if (ImGui::BeginTable("MainTable", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody))
    {
        // Setup column widths (2:1:2 ratio)
        ImGui::TableSetupColumn("Encrypted", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Buttons", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Decrypted", ImGuiTableColumnFlags_WidthStretch, 2.0f);

        // Row 1: Titles
        ImGui::TableNextRow();

        // Cell (1,1): Decrypted title
        ImGui::TableSetColumnIndex(0);
        if (ImGui::BeginTable("DecryptedHeader", 3,
                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoBordersInBody))
        {
            ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthFixed, 0.0f);
            ImGui::TableSetupColumn("Spacer", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Buttons", ImGuiTableColumnFlags_WidthFixed, 0.0f);

            ImGui::TableNextRow();

            // Cell 1: Text (Left Aligned)
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding(); // vertical aligned
            ImGui::Text("Decrypted");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("The decrypted text");
            }

            // Cell 2: Dummy
            ImGui::TableSetColumnIndex(1);

            // Cell 3: Buttons (Right Aligned)
            ImGui::TableSetColumnIndex(2);
            if (ImGui::ButtonPadded("Clear"))
            {
                decrypted_text_.clear();
            }
            ImGui::SameLine();
            if (ImGui::ButtonPadded("Load"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##LoadDecrypted", "Choose Decrypted Text To Load", ".txt",
                                                        {.path = "."});
            }
            ImGui::SameLine();
            if (ImGui::ButtonPadded("Save"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##SaveDecrypted", "Choose Decrypted Text To Save", ".txt",
                                                        {.path = "."});
            }

            ImGui::EndTable();
        }

        // Cell (1,2): Cipher title
        ImGui::TableSetColumnIndex(1);
        ImGui::Spacing();
        ImGui::TextCentered("Cipher");

        // Cell (1,3): Encrypted title
        ImGui::TableSetColumnIndex(2);
        if (ImGui::BeginTable("EncryptedHeader", 3,
                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoBordersInBody))
        {
            ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthFixed, 0.0f);
            ImGui::TableSetupColumn("Spacer", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Buttons", ImGuiTableColumnFlags_WidthFixed, 0.0f);

            ImGui::TableNextRow();

            // Cell 1: Text (Left Aligned)
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding(); // vertically aligned
            ImGui::Text("Encrypted");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("The encrypted text");
            }

            // Cell 2: Dummy
            ImGui::TableSetColumnIndex(1);

            // Cell 3: Buttons (Right Aligned)
            ImGui::TableSetColumnIndex(2);
            if (ImGui::ButtonPadded("Clear"))
            {
                encrypted_text_.clear();
            }
            ImGui::SameLine();
            if (ImGui::ButtonPadded("Load"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##LoadEncrypted", "Choose Encrypted Text To Load", ".txt",
                                                        {.path = "."});
            }
            ImGui::SameLine();
            if (ImGui::ButtonPadded("Save"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##SaveEncrypted", "Choose Encrypted Text To Save", ".txt",
                                                        {.path = "."});
            }

            ImGui::EndTable();
        }

        // Row 2: Content
        ImGui::TableNextRow();

        // Cell (2,1): Encrypted text input
        ImGui::TableSetColumnIndex(0);
        ImGui::PushFont(jetbrains_mono_regular, 0.f);
        ImGui::InputTextMultiline("##Decrypted", &decrypted_text_, ImVec2{-1, -1});
        ImGui::PopFont();

        // Cell (2,2): Buttons and options
        ImGui::TableSetColumnIndex(1);

        // Stretch buttons in the column
        const auto button_width{ImGui::GetContentRegionAvail().x};
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - button_width) / 2);
        ImGui::PushItemWidth(button_width);

        ImGui::Spacing();
        ImGui::RadioButton("Substitution", reinterpret_cast<int *>(&selected_cipher_),
                           static_cast<int>(cipher::substitution));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Each letter is replaced by another letter based on a fixed mapping");
        }
        ImGui::RadioButton("Transposition", reinterpret_cast<int *>(&selected_cipher_),
                           static_cast<int>(cipher::transposition));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Letters of the message are rearranged according to a shifted pattern");
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Encrypt", ImVec2{button_width, 0}))
        {
            [this]() -> eresult<std::string> {
                switch (selected_cipher_)
                {
                case cipher::substitution:
                    return substitution_cipher.encrypt(decrypted_text_);
                case cipher::transposition:
                    return transposition_cipher.encrypt(decrypted_text_);
                }
            }()
                            .and_then([this](const std::string value) -> eresult<void> {
                                encrypted_text_ = std::move(value);
                                return {};
                            })
                            .or_else([this, &cipher_message](const std::string error) -> eresult<void> {
                                ImGui::OpenPopup("Error Encrypting");
                                cipher_message = std::move(error);
                                return std::unexpected{error};
                            })
                            .emplace();
        }
        if (ImGui::Button("Decrypt", ImVec2{button_width, 0}))
        {
            [this]() -> eresult<std::string> {
                switch (selected_cipher_)
                {
                case cipher::substitution:
                    return substitution_cipher.decrypt(encrypted_text_);
                case cipher::transposition:
                    return transposition_cipher.decrypt(encrypted_text_);
                }
            }()
                            .and_then([this](const std::string value) -> eresult<void> {
                                decrypted_text_ = std::move(value);
                                return {};
                            })
                            .or_else([this, &cipher_message](const std::string error) -> eresult<void> {
                                ImGui::OpenPopup("Error Decrypting");
                                cipher_message = std::move(error);
                                return std::unexpected{error};
                            })
                            .emplace();
        }

        if (selected_cipher_ == cipher::transposition)
        {
            if (ImGui::Button("Decrypt Brute", ImVec2{button_width, 0}))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##SaveDecryptedBrute", "Choose Decrypted Texts To Save",
                                                        ".txt", {.path = "."});
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextCentered("Transposition Key");
            ImGui::InputInt("##TranspositionKey", &transposition_key);
        }

        ImGui::InformationPopup("Error Encrypting", cipher_message.c_str(), [] {});
        ImGui::InformationPopup("Error Decrypting", cipher_message.c_str(), [] {});

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Exit", ImVec2{button_width, 0}))
        {
            ImGui::OpenPopup("Exit Confirmation");
        }

        ImGui::ConfirmationPopup("Exit Confirmation", "Are you sure to exit?", [this] { should_exit_ = true; });

        ImGui::PopItemWidth();

        // Cell (2,3): Decrypted text input
        ImGui::TableSetColumnIndex(2);
        ImGui::PushFont(jetbrains_mono_regular, 0.f);
        ImGui::InputTextMultiline("##Encrypted", &encrypted_text_, ImVec2{-1, -1});
        ImGui::PopFont();

        ImGui::EndTable();
    }

    // ImGui::PopFont();
}

[[nodiscard]] eresult<void> gui::process_file() noexcept
{
    return read_file_dialog("##LoadEncrypted", encrypted_text_)
        .and_then([this] { return read_file_dialog("##LoadDecrypted", decrypted_text_); })
        .and_then([this] { return write_file_dialog("##SaveEncrypted", encrypted_text_); })
        .and_then([this] { return write_file_dialog("##SaveDecrypted", decrypted_text_); })
        .and_then([this] {
            return display_file_dialog("##SaveDecryptedBrute")
                .transform([this](const std::string path) -> eresult<void> {
                    std::ranges::for_each(std::views::iota(1, 27), [&](const int i) {
                        tprotect::cipher::transposition_cipher cipher{i};
                        std::filesystem::path fs_path{path}, fs_extention{fs_path.extension()};
                        return cipher.decrypt(encrypted_text_).and_then([&](const std::string decrypted_text) {
                            return write_file(fs_path.replace_extension()
                                                  .replace_filename({std::format("{}_{}{}", fs_path.filename().string(),
                                                                                 i, fs_extention.string())})
                                                  .string(),
                                              std::move(decrypted_text));
                        });
                    });
                    ImGui::OpenPopup("Save Decrypt Brute");
                    ImGui::InformationPopup("Save Decrypt Brute", "Successfully decrypted brute-forcely", [] {});
                    return {};
                })
                .value_or({});
        });
}
} // namespace tprotect
