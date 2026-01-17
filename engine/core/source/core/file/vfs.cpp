#include "core/file/vfs.hpp"
#include "core/log/logger.hpp"

#include <fstream>
#include <algorithm>

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
        auto path = resolvePath(virtualPath);

        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    }

    auto VFS::open(std::string const& virtualPath) -> std::unique_ptr<File>
    {
        auto path = resolvePath(virtualPath);

        auto displayPath = normalize(virtualPath);

        if (path.string() == virtualPath)
        {
            AP_ERROR("<VFS>: Mount point not found for path: {}", displayPath);
            return nullptr;
        }

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

} // namespace april
