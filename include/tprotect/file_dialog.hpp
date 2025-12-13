// file_dialog.hpp: File Dialog Helpers

#pragma once

#include <algorithm>
#include <expected>
#include <fstream>

#include <ImGuiFileDialog.h>

#include <tprotect/global.hpp>

namespace tprotect
{
[[nodiscard]] inline eresult<std::string> read_file(const std::string &file_name) noexcept
{
    std::ifstream ifs{file_name};
    if (!ifs)
    {
        return std::unexpected{"Failed to open file"};
    }
    std::string result{std::istreambuf_iterator{ifs}, {}}; // read file using iterators
    if (!ifs)
    {
        return std::unexpected{"Failed to read file"};
    }
    return {result};
}

[[nodiscard]] inline eresult<void> write_file(const std::string &file_name, const std::string &content) noexcept
{
    std::ofstream ofs{file_name};
    if (!ofs)
    {
        return std::unexpected{"Failed to open file"};
    }
    std::ranges::copy(content, std::ostreambuf_iterator{ofs}); // write file using iterators
    if (!ofs)
    {
        return std::unexpected{"Failed to write file"};
    }
    return {};
}

[[nodiscard]] inline oresult<std::string> display_file_dialog(const std::string &key) noexcept
{
    if (const auto instance{ImGuiFileDialog::Instance()};
        instance->Display(key, ImGuiWindowFlags_NoCollapse, ImVec2{700, 400}))
    {
        instance->Close();
        if (instance->IsOk())
        {
            return {instance->GetFilePathName()};
        }
    }
    return std::nullopt;
}

[[nodiscard]] inline eresult<void> read_file_dialog(const std::string &key, std::string &content) noexcept
{
    return display_file_dialog(key)
        .and_then([&](const std::string &file_path) -> std::optional<eresult<void>> {
            return read_file(file_path).transform([&](std::string file_content) {
                content = std::move(file_content); // if succeeding, overwriting the content
            });
        })
        .value_or({});
}

[[nodiscard]] inline eresult<void> write_file_dialog(const std::string &key, const std::string &content) noexcept
{
    return display_file_dialog(key)
        .and_then([&](const std::string &file_path) -> std::optional<eresult<void>> {
            return write_file(file_path, content);
        })
        .value_or({});
}
} // namespace tprotect
