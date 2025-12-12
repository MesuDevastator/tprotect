// imgui_additions.hpp: A Few Additions to Dear ImGui

#pragma once

#include <functional>

#include <imgui_config.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace ImGui
{
inline void TextCentered(const char *const text) noexcept
{
    const auto text_size{ImGui::CalcTextSize(text)};
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - text_size.x) / 2);
    ImGui::TextUnformatted(text);
}

inline void ConfirmationPopup(const char *const id, const char *const message,
                              std::function<void()> &&callback) noexcept
{
    if (ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::TextUnformatted(message);
        ImGui::Separator();

        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere(1);
        }

        if (ImGui::Button("Yes", ImVec2{80, 0}))
        {
            callback();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2{80, 0}) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void InformationPopup(const char *const id, const char *const message, std::function<void()> &&callback) noexcept
{
    if (ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::TextUnformatted(message);
        ImGui::Separator();

        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere(1);
        }

        if (ImGui::Button("OK", ImVec2{80, 0}))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
} // namespace ImGui
