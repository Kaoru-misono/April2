#include "local-ddc.hpp"

#include <core/log/logger.hpp>
#include <core/tools/hash.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <string>
#include <system_error>
#include <thread>

namespace april::asset
{
    LocalDdc::LocalDdc(std::filesystem::path rootPath)
        : m_rootPath{std::move(rootPath)}
    {
        if (!std::filesystem::exists(m_rootPath))
        {
            std::filesystem::create_directories(m_rootPath);
        }
    }

    auto LocalDdc::get(std::string const& key, DdcValue& outValue) -> bool
    {
        auto path = makePathForKey(key);
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        auto fileBytes = readFile(path);
        if (fileBytes.size() < sizeof(LocalDdc::DdcFileHeader))
        {
            AP_WARN("[DDC] Invalid DDC file size: {}", path.string());
            return false;
        }

        auto header = LocalDdc::DdcFileHeader{};
        std::memcpy(&header, fileBytes.data(), sizeof(LocalDdc::DdcFileHeader));

        if (header.magic != LocalDdc::DdcFileHeader{}.magic || header.version != LocalDdc::DdcFileHeader{}.version)
        {
            AP_WARN("[DDC] Invalid DDC header: {}", path.string());
            return false;
        }

        auto payloadOffset = sizeof(LocalDdc::DdcFileHeader);
        auto payloadSize = static_cast<size_t>(header.payloadSize);
        if (fileBytes.size() < payloadOffset + payloadSize)
        {
            AP_WARN("[DDC] Truncated DDC payload: {}", path.string());
            return false;
        }

        outValue.bytes.assign(fileBytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset),
                               fileBytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset + payloadSize));
        outValue.contentHash = core::computeStringHash(std::string{
            reinterpret_cast<char const*>(outValue.bytes.data()),
            outValue.bytes.size()
        });

        return !outValue.bytes.empty();
    }

    auto LocalDdc::put(std::string const& key, DdcValue const& value) -> void
    {
        auto lock = std::scoped_lock{m_writeMutex};
        auto path = makePathForKey(key);
        auto directory = path.parent_path();
        if (!std::filesystem::exists(directory))
        {
            std::filesystem::create_directories(directory);
        }

        auto header = LocalDdc::DdcFileHeader{};
        header.payloadSize = value.bytes.size();
        auto keyHash = core::computeStringHash(key);
        std::memcpy(header.keyHash.data(), keyHash.data(), std::min(keyHash.size(), header.keyHash.size()));

        auto fileBytes = std::vector<std::byte>{};
        fileBytes.resize(sizeof(LocalDdc::DdcFileHeader) + value.bytes.size());
        std::memcpy(fileBytes.data(), &header, sizeof(LocalDdc::DdcFileHeader));
        if (!value.bytes.empty())
        {
            std::memcpy(fileBytes.data() + sizeof(LocalDdc::DdcFileHeader), value.bytes.data(), value.bytes.size());
        }

        writeFile(path, fileBytes);
    }

    auto LocalDdc::exists(std::string const& key) -> bool
    {
        auto path = makePathForKey(key);
        return std::filesystem::exists(path);
    }

    auto LocalDdc::makePathForKey(std::string const& key) const -> std::filesystem::path
    {
        auto hash = core::computeStringHash(key);
        auto subDir = hash.substr(0, 2);
        auto subDir2 = hash.substr(2, 2);
        return m_rootPath / subDir / subDir2 / (hash + ".bin");
    }

    auto LocalDdc::readFile(std::filesystem::path const& path) const -> std::vector<std::byte>
    {
        auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
        if (!file.is_open())
        {
            AP_ERROR("[DDC] Failed to open file: {}", path.string());
            return {};
        }

        auto size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        auto data = std::vector<std::byte>(size);
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));

        return data;
    }

    auto LocalDdc::writeFile(std::filesystem::path const& path, std::vector<std::byte> const& data) const -> void
    {
        static auto counter = std::atomic<uint64_t>{0};
        auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        auto unique = std::format("{}.{}.{}",
            stamp,
            tid,
            counter.fetch_add(1));

        auto tempPath = path;
        tempPath += ".";
        tempPath += unique;
        tempPath += ".tmp";

        auto file = std::ofstream{tempPath, std::ios::binary | std::ios::trunc};
        if (!file.is_open())
        {
            AP_ERROR("[DDC] Failed to write file: {}", tempPath.string());
            return;
        }

        file.write(reinterpret_cast<char const*>(data.data()), static_cast<std::streamsize>(data.size()));
        file.flush();
        file.close();

        auto ec = std::error_code{};
        if (std::filesystem::exists(path, ec))
        {
            std::filesystem::remove(path, ec);
        }

        ec = {};
        std::filesystem::rename(tempPath, path, ec);
        if (ec)
        {
            AP_ERROR("[DDC] Failed to finalize file: {} -> {} ({})", tempPath.string(), path.string(), ec.message());
            std::filesystem::remove(tempPath, ec);
        }
    }
} // namespace april::asset
