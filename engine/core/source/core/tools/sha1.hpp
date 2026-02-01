#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <span>

namespace april::core
{
    class Sha1
    {
    public:
        using Digest = std::array<std::uint8_t, 20>;
        using State  = std::array<std::uint32_t, 5>;
        using Buffer = std::array<std::uint8_t, 64>;

        Sha1();

        auto update(std::string_view data) -> Sha1&;
        auto update(std::span<std::uint8_t const> data) -> Sha1&;
        auto update(std::span<std::byte const> data) -> Sha1&;

        auto update(void const* data, std::size_t len) -> Sha1&;
        auto update(std::uint8_t byte) -> Sha1&;

        [[nodiscard]] auto getDigest() const -> Digest;
        [[nodiscard]] auto getHexDigest() const -> std::string;

        auto reset() -> void;

    private:
        auto processBlock(std::uint8_t const* ptr) -> void;
        auto addByte(std::uint8_t byte) -> void;

        State         m_state{};
        Buffer        m_buffer{};    // 512-bit block
        std::uint64_t m_bitCount{0};
        std::uint32_t m_bufferIdx{0};
    };
}
