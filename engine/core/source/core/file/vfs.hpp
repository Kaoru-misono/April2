#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <map>
#include <mutex>
#include <span>

namespace april
{
    using Blob = std::vector<std::byte>;

    struct File
    {
        virtual ~File() = default;

        [[nodiscard]] virtual auto getSize() const -> size_t = 0;

        virtual auto read(std::span<std::byte> buffer) -> size_t = 0;

        [[nodiscard]] virtual auto readAll() -> Blob;

        [[nodiscard]] virtual auto readText() -> std::string;
    };


    struct VFS
    {
        static auto init() -> void;
        static auto shutdown() -> void;

        static auto mount(std::string const& alias, std::filesystem::path const& physicalPath) -> void;

        static auto unmount(std::string const& alias) -> void;

        [[nodiscard]] static auto open(std::string const& virtualPath) -> std::unique_ptr<File>;

        [[nodiscard]] static auto exists(std::string const& virtualPath) -> bool;

        [[nodiscard]] static auto readTextFile(std::string const& virtualPath) -> std::string;
        [[nodiscard]] static auto readBinaryFile(std::string const& virtualPath) -> Blob;

        [[nodiscard]] static auto resolvePath(std::string const& virtualPath) -> std::filesystem::path;

    private:
        [[nodiscard]] static auto normalize(std::string const& path) -> std::string;

    private:
        static std::map<std::string, std::filesystem::path, std::greater<>> m_mountPoints;
        static std::mutex m_mutex;
    };

} // namespace april
