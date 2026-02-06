#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <map>
#include <mutex>
#include <span>
#include <string_view>

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
        [[nodiscard]] static auto existsFile(std::string const& virtualPath) -> bool;
        [[nodiscard]] static auto existsDirectory(std::string const& virtualPath) -> bool;
        static auto createDirectories(std::string const& virtualPath) -> bool;
        static auto removeFile(std::string const& virtualPath) -> bool;
        static auto rename(std::string const& fromVirtualPath, std::string const& toVirtualPath) -> bool;

        [[nodiscard]] static auto readTextFile(std::string const& virtualPath) -> std::string;
        [[nodiscard]] static auto readBinaryFile(std::string const& virtualPath) -> Blob;
        [[nodiscard]] static auto writeTextFile(std::string const& virtualPath, std::string const& contents) -> bool;
        [[nodiscard]] static auto writeBinaryFile(std::string const& virtualPath, std::span<std::byte const> contents) -> bool;

        [[nodiscard]] static auto resolvePath(std::string const& virtualPath) -> std::filesystem::path;

        [[nodiscard]] static auto listFilesRecursive(
            std::string const& virtualPath,
            std::string_view extensionFilter = {}
        ) -> std::vector<std::string>;

    private:
        [[nodiscard]] static auto normalize(std::string const& path) -> std::string;

    private:
        static std::map<std::string, std::filesystem::path, std::greater<>> m_mountPoints;
        static std::mutex m_mutex;
    };

} // namespace april
