#include "core/file/vfs.hpp"
#include "core/log/logger.hpp"

#include <fstream>
#include <algorithm>
#include <system_error>

namespace april
{
    // --- NativeFile Implementation (Hidden in .cpp) ---
    struct NativeFile final : public File
    {
        NativeFile(std::filesystem::path const& path)
            : m_path(path)
        {
            m_stream.open(path, std::ios::binary | std::ios::ate);
            if (m_stream.is_open())
            {
                m_size = m_stream.tellg();
                m_stream.seekg(0, std::ios::beg);
            }
            else
            {
                m_size = 0;
            }
        }

        ~NativeFile() override
        {
            if (m_stream.is_open())
            {
                m_stream.close();
            }
        }

        [[nodiscard]] auto isValid() const -> bool { return m_stream.is_open(); }

        auto getSize() const -> size_t override { return m_size; }

        auto read(std::span<std::byte> buffer) -> size_t override
        {
            if (!m_stream.is_open()) return 0;

            m_stream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            return static_cast<size_t>(m_stream.gcount());
        }

    private:
        std::filesystem::path m_path;
        std::ifstream m_stream;
        size_t m_size{0};
    };

    // --- File Default Implementation ---

    auto File::readAll() -> Blob
    {
        auto const fileSize = getSize();
        if (fileSize == 0) return {};

        auto buffer = Blob(fileSize);
        read(buffer);
        return buffer;
    }

    auto File::readText() -> std::string
    {
        auto const fileSize = getSize();
        if (fileSize == 0) return {};

        auto str = std::string{};
        str.resize(fileSize);

        read(std::span<std::byte>(reinterpret_cast<std::byte*>(str.data()), fileSize));

        return str;
    }

    // --- VFS Implementation ---

    std::map<std::string, std::filesystem::path, std::greater<>> VFS::m_mountPoints{};
    std::mutex VFS::m_mutex{};

    auto VFS::normalize(std::string const& path) -> std::string
    {
        auto res = path;
        std::replace(res.begin(), res.end(), '\\', '/');

        return res;
    }

    auto VFS::init() -> void
    {
        AP_INFO("VFS Initialized");
    }

    auto VFS::shutdown() -> void
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);
        m_mountPoints.clear();
    }

    auto VFS::mount(std::string const& alias, std::filesystem::path const& physicalPath) -> void
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);

        auto cleanAlias = normalize(alias);

        if (!cleanAlias.empty() && cleanAlias.back() == '/')
        {
            cleanAlias.pop_back();
        }

        m_mountPoints[cleanAlias] = std::filesystem::absolute(physicalPath);

        AP_INFO("VFS Mounted: '{}' -> '{}'", cleanAlias, physicalPath.generic_string());
    }

    auto VFS::unmount(std::string const& alias) -> void
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);

        auto cleanAlias = normalize(alias);
        m_mountPoints.erase(cleanAlias);
    }

    auto VFS::resolvePath(std::string const& virtualPath) -> std::filesystem::path
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);

        auto vPath = normalize(virtualPath);

        for (auto const& [alias, rootPath] : m_mountPoints)
        {
            if (vPath.rfind(alias, 0) == 0)
            {
                auto subPath = vPath.substr(alias.length());

                if (!subPath.empty() && (subPath[0] == '/' || subPath[0] == '\\'))
                {
                    subPath = subPath.substr(1);
                }

                auto finalPath = rootPath / subPath;
                return finalPath;
            }
        }

        return virtualPath;
    }

    auto VFS::exists(std::string const& virtualPath) -> bool
    {
        return existsFile(virtualPath);
    }

    auto VFS::existsFile(std::string const& virtualPath) -> bool
    {
        auto path = resolvePath(virtualPath);

        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    }

    auto VFS::existsDirectory(std::string const& virtualPath) -> bool
    {
        auto path = resolvePath(virtualPath);

        return std::filesystem::exists(path) && std::filesystem::is_directory(path);
    }

    auto VFS::createDirectories(std::string const& virtualPath) -> bool
    {
        auto path = resolvePath(virtualPath);
        auto ec = std::error_code{};

        if (std::filesystem::exists(path, ec))
        {
            return std::filesystem::is_directory(path);
        }

        auto created = std::filesystem::create_directories(path, ec);
        if (ec)
        {
            AP_ERROR("<VFS>: Failed to create directories: {} ({})", path.generic_string(), ec.message());
            return false;
        }

        return created || std::filesystem::exists(path);
    }

    auto VFS::removeFile(std::string const& virtualPath) -> bool
    {
        auto path = resolvePath(virtualPath);
        auto ec = std::error_code{};
        auto removed = std::filesystem::remove(path, ec);
        if (ec)
        {
            AP_ERROR("<VFS>: Failed to remove file: {} ({})", path.generic_string(), ec.message());
            return false;
        }

        return removed;
    }

    auto VFS::rename(std::string const& fromVirtualPath, std::string const& toVirtualPath) -> bool
    {
        auto fromPath = resolvePath(fromVirtualPath);
        auto toPath = resolvePath(toVirtualPath);
        auto ec = std::error_code{};

        std::filesystem::rename(fromPath, toPath, ec);
        if (ec)
        {
            AP_ERROR("<VFS>: Failed to rename file: {} -> {} ({})",
                fromPath.generic_string(),
                toPath.generic_string(),
                ec.message());
            return false;
        }

        return true;
    }

    auto VFS::open(std::string const& virtualPath) -> std::unique_ptr<File>
    {
        auto path = resolvePath(virtualPath);

        auto displayPath = normalize(virtualPath);

        if (!std::filesystem::exists(path))
        {
            AP_ERROR("<VFS>: File not found: {} (Physical: {})", displayPath, path.generic_string());
            return nullptr;
        }

        auto file = std::make_unique<NativeFile>(path);
        if (!file->isValid())
        {
            AP_ERROR("<VFS>: Failed to open file: {}", path.generic_string());
            return nullptr;
        }

        return file;
    }

    auto VFS::readTextFile(std::string const& virtualPath) -> std::string
    {
        auto file = open(virtualPath);
        if (!file) return {};

        return file->readText();
    }

    auto VFS::readBinaryFile(std::string const& virtualPath) -> Blob
    {
        auto file = open(virtualPath);
        if (!file) return {};

        return file->readAll();
    }

    auto VFS::writeTextFile(std::string const& virtualPath, std::string const& contents) -> bool
    {
        auto path = resolvePath(virtualPath);

        auto file = std::ofstream{path, std::ios::trunc};
        if (!file.is_open())
        {
            AP_ERROR("<VFS>: Failed to write file: {}", path.generic_string());
            return false;
        }

        file << contents;

        return file.good();
    }

    auto VFS::writeBinaryFile(std::string const& virtualPath, std::span<std::byte const> contents) -> bool
    {
        auto path = resolvePath(virtualPath);

        auto file = std::ofstream{path, std::ios::binary | std::ios::trunc};
        if (!file.is_open())
        {
            AP_ERROR("<VFS>: Failed to write file: {}", path.generic_string());
            return false;
        }

        if (!contents.empty())
        {
            file.write(reinterpret_cast<char const*>(contents.data()),
                static_cast<std::streamsize>(contents.size()));
        }

        return file.good();
    }

    auto VFS::listFilesRecursive(std::string const& virtualPath, std::string_view extensionFilter)
        -> std::vector<std::string>
    {
        auto rootPath = resolvePath(virtualPath);
        auto files = std::vector<std::string>{};

        if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath))
        {
            return files;
        }

        auto baseVirtual = normalize(virtualPath);
        if (!baseVirtual.empty() && baseVirtual.back() == '/')
        {
            baseVirtual.pop_back();
        }

        auto ec = std::error_code{};
        auto it = std::filesystem::recursive_directory_iterator(rootPath, ec);
        if (ec)
        {
            AP_ERROR("<VFS>: Failed to iterate directory: {} ({})", rootPath.generic_string(), ec.message());
            return files;
        }

        for (auto const& entry : it)
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (!extensionFilter.empty() && entry.path().extension().string() != extensionFilter)
            {
                continue;
            }

            auto relEc = std::error_code{};
            auto relPath = std::filesystem::relative(entry.path(), rootPath, relEc);
            if (relEc)
            {
                continue;
            }

            auto relString = relPath.generic_string();
            if (baseVirtual.empty())
            {
                files.push_back(relString);
            }
            else
            {
                files.push_back(baseVirtual + "/" + relString);
            }
        }

        return files;
    }

} // namespace april
