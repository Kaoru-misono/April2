#include "sha1.hpp"

#include <bit>
#include <cstring>

namespace april::core
{
    namespace
    {
        constexpr std::uint32_t K[] = {
            0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6
        };

        inline auto loadBigEndian32(std::uint8_t const* p) -> std::uint32_t
        {
            return (
                (static_cast<std::uint32_t>(p[0]) << 24) |
                (static_cast<std::uint32_t>(p[1]) << 16) |
                (static_cast<std::uint32_t>(p[2]) << 8)  |
                (static_cast<std::uint32_t>(p[3]))
            );
        }
    }

    Sha1::Sha1()
    {
        reset();
    }

    auto Sha1::reset() -> void
    {
        m_state = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
        m_bitCount = 0;
        m_bufferIdx = 0;
    }

    auto Sha1::update(std::string_view data) -> Sha1&
    {
        return update(data.data(), data.size());
    }

    auto Sha1::update(std::span<std::uint8_t const> data) -> Sha1&
    {
        return update(data.data(), data.size());
    }

    auto Sha1::update(std::span<std::byte const> data) -> Sha1&
    {
        return update(reinterpret_cast<std::uint8_t const*>(data.data()), data.size());
    }

    auto Sha1::update(std::uint8_t byte) -> Sha1&
    {
        addByte(byte);
        m_bitCount += 8;
        return *this;
    }

    auto Sha1::update(void const* data, std::size_t len) -> Sha1&
    {
        if (!data || len == 0) return *this;

        auto const* ptr = static_cast<std::uint8_t const*>(data);
        m_bitCount += static_cast<std::uint64_t>(len) * 8;

        // 1. Fill buffer
        if (m_bufferIdx > 0)
        {
            std::size_t left = m_buffer.size() - m_bufferIdx;
            if (len < left)
            {
                std::memcpy(m_buffer.data() + m_bufferIdx, ptr, len);
                m_bufferIdx += static_cast<std::uint32_t>(len);
                return *this;
            }
            std::memcpy(m_buffer.data() + m_bufferIdx, ptr, left);
            processBlock(m_buffer.data());
            ptr += left;
            len -= left;
            m_bufferIdx = 0;
        }

        // 2. Process full blocks
        while (len >= m_buffer.size())
        {
            processBlock(ptr);
            ptr += m_buffer.size();
            len -= m_buffer.size();
        }

        // 3. Buffer remaining
        if (len > 0)
        {
            std::memcpy(m_buffer.data(), ptr, len);
            m_bufferIdx = static_cast<std::uint32_t>(len);
        }

        return *this;
    }

    auto Sha1::addByte(std::uint8_t byte) -> void
    {
        m_buffer[m_bufferIdx++] = byte;
        if (m_bufferIdx >= m_buffer.size())
        {
            m_bufferIdx = 0;
            processBlock(m_buffer.data());
        }
    }

    auto Sha1::getDigest() const -> Digest
    {
        Sha1 copy = *this;

        // 1. Append 0x80
        copy.addByte(0x80);

        // 2. Pad with zeros
        while (copy.m_bufferIdx % 64 != 56)
        {
            copy.addByte(0);
        }

        // 3. Append Length
        for (int i = 7; i >= 0; --i)
        {
            copy.addByte(static_cast<std::uint8_t>((m_bitCount >> (i * 8)) & 0xFF));
        }

        // 4. Output
        Digest digest;
        for (int i = 0; i < 5; i++)
        {
            for (int j = 3; j >= 0; j--)
            {
                digest[i * 4 + j] = (copy.m_state[i] >> ((3 - j) * 8)) & 0xFF;
            }
        }
        return digest;
    }

    auto Sha1::getHexDigest() const -> std::string
    {
        Digest d = getDigest();
        std::string hex;
        hex.reserve(40);
        static constexpr char hexChars[] = "0123456789abcdef";
        for (std::uint8_t b : d)
        {
            hex.push_back(hexChars[b >> 4]);
            hex.push_back(hexChars[b & 0x0F]);
        }
        return hex;
    }

    auto Sha1::processBlock(std::uint8_t const* ptr) -> void
    {
        std::uint32_t w[16];
        for (int i = 0; i < 16; i++)
        {
            w[i] = loadBigEndian32(ptr + i * 4);
        }

        std::uint32_t a = m_state[0];
        std::uint32_t b = m_state[1];
        std::uint32_t c = m_state[2];
        std::uint32_t d = m_state[3];
        std::uint32_t e = m_state[4];

        auto R0 = [&](std::uint32_t w_val) {
            return std::rotl(a, 5) + ((b & c) | (~b & d)) + e + w_val + K[0];
        };
        auto R1 = [&](std::uint32_t w_val) {
            return std::rotl(a, 5) + (b ^ c ^ d) + e + w_val + K[1];
        };
        auto R2 = [&](std::uint32_t w_val) {
            return std::rotl(a, 5) + ((b & c) | (b & d) | (c & d)) + e + w_val + K[2];
        };

        // SHA-1:
        // 0-19: (b & c) | (~b & d)   (K=0x5A827999)
        // 20-39: b ^ c ^ d           (K=0x6ED9EBA1)
        // 40-59: (b & c) | (b & d) | (c & d) (K=0x8F1BBCDC)
        // 60-79: b ^ c ^ d           (K=0xCA62C1D6)

        auto performRound = [&](auto func, int i)
        {
            int const j = i & 15;
            if (i >= 16)
            {
                w[j] = std::rotl(w[(j + 13) & 15] ^ w[(j + 8) & 15] ^ w[(j + 2) & 15] ^ w[j], 1);
            }

            std::uint32_t temp = func(w[j]);
            e = d; d = c; c = std::rotl(b, 30); b = a; a = temp;
        };

        // 0-19
        for(int i = 0; i < 20; ++i) performRound(R0, i);
        // 20-39
        for(int i = 20; i < 40; ++i) performRound(R1, i);
        // 40-59
        for(int i = 40; i < 60; ++i) performRound(R2, i);

        auto F_XOR = [&](std::uint32_t k_val, std::uint32_t w_val) {
            return std::rotl(a, 5) + (b ^ c ^ d) + e + w_val + k_val;
        };

        for(int i = 0; i < 20; ++i) performRound(R0, i);
        for(int i = 20; i < 40; ++i) performRound([&](auto w){ return F_XOR(K[1], w); }, i);
        for(int i = 40; i < 60; ++i) performRound(R2, i);
        for(int i = 60; i < 80; ++i) performRound([&](auto w){ return F_XOR(K[3], w); }, i);

        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
        m_state[4] += e;
    }
}
