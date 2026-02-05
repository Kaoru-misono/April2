#pragma once

#include "ddc.hpp"

#include <array>
#include <filesystem>

namespace april::asset
{
    class LocalDdc final : public IDdc
    {
    public:
        explicit LocalDdc(std::filesystem::path rootPath = "Cache/DDC");

        auto get(std::string const& key, DdcValue& outValue) -> bool override;
        auto put(std::string const& key, DdcValue const& value) -> void override;
        auto exists(std::string const& key) -> bool override;

        [[nodiscard]] auto getRootPath() const -> std::filesystem::path const& { return m_rootPath; }

    private:
        struct DdcFileHeader
        {
            uint32_t magic = 0x30434444; // 'DDC0'
            uint16_t version = 1;
            uint16_t reserved = 0;
            uint64_t payloadSize = 0;
            std::array<char, 40> keyHash{};
        };

        std::filesystem::path m_rootPath;

        [[nodiscard]] auto makePathForKey(std::string const& key) const -> std::filesystem::path;
        [[nodiscard]] auto readFile(std::filesystem::path const& path) const -> std::vector<std::byte>;
        auto writeFile(std::filesystem::path const& path, std::vector<std::byte> const& data) const -> void;
    };
} // namespace april::asset
