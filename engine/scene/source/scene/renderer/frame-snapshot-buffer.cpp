#include "frame-snapshot-buffer.hpp"

namespace april::scene
{
    auto FrameSnapshotBuffer::acquireWrite() -> FrameSnapshot&
    {
        return m_snapshots[m_writeIndex];
    }

    auto FrameSnapshotBuffer::submitWrite() -> void
    {
        m_readIndex = m_writeIndex;
        m_writeIndex = (m_writeIndex + 1) % m_snapshots.size();
    }

    auto FrameSnapshotBuffer::getRead() const -> FrameSnapshot const&
    {
        return m_snapshots[m_readIndex];
    }
}
