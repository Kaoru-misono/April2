#pragma once

#include "render-types.hpp"

#include <array>
#include <cstddef>

namespace april::scene
{
    class FrameSnapshotBuffer
    {
    public:
        auto acquireWrite() -> FrameSnapshot&;
        auto submitWrite() -> void;
        auto getRead() const -> FrameSnapshot const&;

    private:
        std::array<FrameSnapshot, 2> m_snapshots{};
        size_t m_readIndex{0};
        size_t m_writeIndex{1};
    };
}
