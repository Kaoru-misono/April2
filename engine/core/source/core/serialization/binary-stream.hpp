#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <vector>
#include <span>
#include <filesystem>

namespace april
{
    struct Serializer
    {
        Serializer(std::filesystem::path const& path)
            : m_stream{path, std::ios::binary | std::ios::out}
        {
        }

        template <typename T>
        auto write(T const& value) -> void
        {
            m_stream.write(reinterpret_cast<char const*>(&value), sizeof(T));
        }

        auto writeString(std::string const& str) -> void
        {
            auto size = static_cast<uint32_t>(str.size());
            write(size);
            m_stream.write(str.data(), size);
        }

        auto writeBuffer(std::span<std::byte const> data) -> void
        {
            auto size = static_cast<uint32_t>(data.size());
            write(size);
            m_stream.write(reinterpret_cast<char const*>(data.data()), data.size());
        }

        [[nodiscard]] auto isOpen() const -> bool { return m_stream.is_open(); }

    private:
        std::ofstream m_stream{};
    };

    struct Deserializer
    {
        Deserializer(std::filesystem::path const& path)
            : m_stream{path, std::ios::binary | std::ios::in}
        {
        }

        template <typename T>
        auto read(T& outValue) -> void
        {
            m_stream.read(reinterpret_cast<char*>(&outValue), sizeof(T));
        }

        auto readString(std::string& outStr) -> void
        {
            auto size = uint32_t{0};
            read(size);
            outStr.resize(size);
            m_stream.read(outStr.data(), size);
        }

        // Reads data into a vector.
        auto readBuffer(std::vector<std::byte>& outData) -> void
        {
            auto size = uint32_t{0};
            read(size);
            outData.resize(size);
            m_stream.read(reinterpret_cast<char*>(outData.data()), size);
        }

        [[nodiscard]] auto isOpen() const -> bool { return m_stream.is_open(); }

    private:
        std::ifstream m_stream{};
    };
}