// gui.cpp: Dear ImGui User Interface Manager

#include <tprotect/file_dialog.hpp>
#include <tprotect/gui.hpp>

#include <filesystem>
#include <print>
#include <ranges>

#include <imgui_additions.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // includes system OpenGL headers

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

[[nodiscard]] gui::result_type gui::create(const int width, const int height, std::string title) noexcept
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

[[nodiscard]] gui::result_type gui::initialize(const int width, const int height, std::string title) noexcept
{
    if (bool expected{};
        !is_initialized_.compare_exchange_strong(expected, true, std::memory_order_release, std::memory_order_relaxed))
    {
        return std::unexpected{"GUI has already been initialized"};
    }

    title_ = std::move(title);

    // Initialize GLFW
    glfwSetErrorCallback([](const int error, const char *const description) {
        std::println(stderr, "[GLFW] Error {}: {}", error, description);
    });
    if (!glfwInit())
    {
        return std::unexpected{"Failed to initialize GLFW"};
    }
#ifdef __APPLE__
    // OpenGL 3.2 + GLSL 150
    constexpr const char *glsl_version{"#version 150"};
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // OpenGL 3.0 + GLSL 130
    constexpr const char *glsl_version{"#version 130"};
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Create window
    const float main_scale{ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor())};
    window_ = glfwCreateWindow(static_cast<int>(width * main_scale), static_cast<int>(height * main_scale),
                               title_.c_str(), nullptr, nullptr);
    if (window_ == nullptr)
    {
        return std::unexpected{"Failed to create GLFW window"};
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io{ImGui::GetIO()};
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle &style{ImGui::GetStyle()};
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Setup backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

[[nodiscard]] gui::result_type gui::main_loop() noexcept
{
    if (!is_initialized() || !window_)
    {
        return std::unexpected{"GUI has not been initialized"};
    }
    std::lock_guard<std::mutex> main_loop_guard_{main_loop_mutex_}; // prevent shutdown while the main loop is running
    while (!glfwWindowShouldClose(window_))
    {
        // Handle events
        glfwPollEvents();
        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start a new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render the user draw list
        render();

        // Display and process dialogs
        std::string message{};
        if (auto result{process()}; !result)
        {
            ImGui::OpenPopup("Error Processing File");
            message = std::move(result.error());
        }

        ImGui::InformationPopup("Error Processing File", message.c_str(), [] {});

        if (should_exit_)
        {
            return {};
        }

        // Render the frame
        ImGui::Render();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
    return {};
}

void gui::render() noexcept
{
    const auto viewport{ImGui::GetMainViewport()};
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::Begin("TProtect", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Top title with larger font
    ImGui::SetWindowFontScale(2.0f);
    ImGui::TextCentered("TProtect");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The Text Protector");
    }
    ImGui::SetWindowFontScale(1.0f); // Reset font scale
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
            ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch, 0.0f);
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
            if (ImGui::Button("Clear"))
            {
                decrypted_text_.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##LoadDecrypted", "Choose Decrypted Text To Load", ".txt",
                                                        {.path = "."});
            }
            ImGui::SameLine();
            if (ImGui::Button("Save"))
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
            ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch, 0.0f);
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
            if (ImGui::Button("Clear"))
            {
                encrypted_text_.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("##LoadEncrypted", "Choose Encrypted Text To Load", ".txt",
                                                        {.path = "."});
            }
            ImGui::SameLine();
            if (ImGui::Button("Save"))
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
        ImGui::InputTextMultiline("##Decrypted", &decrypted_text_, ImVec2{-1, -1});

        // Cell (2,2): Buttons and options
        ImGui::TableSetColumnIndex(1);

        // Stretch buttons in the column
        const auto button_width{ImGui::GetContentRegionAvail().x};
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - button_width) / 2);
        ImGui::PushItemWidth(button_width);

        ImGui::Spacing();
        ImGui::RadioButton("Auto", reinterpret_cast<int *>(&selected_cipher_), static_cast<int>(cipher::automatic));
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Defaults to Substitution on encryption");
        }
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
            switch (selected_cipher_)
            {
            case cipher::automatic:
            case cipher::substitution: {
                if (auto result{substitution_cipher.encrypt(decrypted_text_)}; result)
                {
                    encrypted_text_ = std::move(result.value());
                }
                else
                {
                    ImGui::OpenPopup("Error Encrypting");
                    cipher_message = std::move(result.error());
                }
                break;
            }
            case cipher::transposition: {
                if (auto result{transposition_cipher.encrypt(decrypted_text_)}; result)
                {
                    encrypted_text_ = std::move(result.value());
                }
                else
                {
                    ImGui::OpenPopup("Error Encrypting");
                    cipher_message = std::move(result.error());
                }
                break;
            }
            }
        }
        if (ImGui::Button("Decrypt", ImVec2{button_width, 0}))
        {
            switch (selected_cipher_)
            {
            case cipher::automatic:
            case cipher::substitution: {
                if (auto result{substitution_cipher.decrypt(encrypted_text_)}; result)
                {
                    decrypted_text_ = std::move(result.value());
                }
                else
                {
                    ImGui::OpenPopup("Error Decrypting");
                    cipher_message = std::move(result.error());
                }
                break;
            }
            case cipher::transposition: {
                if (auto result{transposition_cipher.decrypt(encrypted_text_)}; result)
                {
                    decrypted_text_ = std::move(result.value());
                }
                else
                {
                    ImGui::OpenPopup("Error Decrypting");
                    cipher_message = std::move(result.error());
                }
                break;
            }
            }
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

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Exit", ImVec2{button_width, 0}))
        {
            ImGui::OpenPopup("Exit Confirmation");
        }

        ImGui::ConfirmationPopup("Exit Confirmation", "Are you sure you want to exit?",
                                 [this] { should_exit_ = true; });

        ImGui::PopItemWidth();

        // Cell (2,3): Decrypted text input
        ImGui::TableSetColumnIndex(2);
        ImGui::InputTextMultiline("##Encrypted", &encrypted_text_, ImVec2{-1, -1});

        ImGui::EndTable();
    }

    ImGui::End();
}

[[nodiscard]] gui::result_type gui::process() noexcept
{
    return read_file_dialog("##LoadEncrypted", encrypted_text_)
        .and_then([this] { return read_file_dialog("##LoadDecrypted", decrypted_text_); })
        .and_then([this] { return write_file_dialog("##SaveEncrypted", encrypted_text_); })
        .and_then([this] { return write_file_dialog("##SaveDecrypted", decrypted_text_); })
        .and_then([this] {
            return display_file_dialog("##SaveDecryptedBrute")
                .and_then([this](const std::string &path) -> std::optional<std::expected<void, std::string>> {
                    std::filesystem::path fs_path{path};
                    std::ranges::for_each(std::views::iota(1, 26), [&](const int i) {
                        tprotect::cipher::transposition_cipher cipher{i};
                        return cipher.decrypt(encrypted_text_).and_then([&](const std::string &decrypted_text) {
                            return write_file(
                                fs_path.replace_filename(std::format("{}_{}.txt", fs_path.filename().string(), i))
                                    .string(),
                                std::move(decrypted_text));
                        });
                    });
                    return {};
                })
                .value_or({});
        });
}
} // namespace tprotect
