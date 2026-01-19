// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "gpu-memory-heap.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>
#include <core/math/math.hpp>
#include <core/tools/alignment.hpp>

namespace april::graphics
{
    namespace
    {
        auto getInitState(MemoryType memoryType) -> Resource::State
        {
            switch (memoryType)
            {
            case MemoryType::DeviceLocal:
                return Resource::State::Common;
            case MemoryType::Upload:
                return Resource::State::GenericRead;
            case MemoryType::ReadBack:
                return Resource::State::CopyDest;
            default:
                AP_UNREACHABLE();
            }
        }
    } // namespace

    GpuMemoryHeap::GpuMemoryHeap(core::ref<Device> p_device, MemoryType memoryType, size_t pageSize, core::ref<Fence> const& p_fence)
        : mp_device(p_device), m_memoryType(memoryType), mp_fence(p_fence), m_pageSize(pageSize)
    {
        allocateNewPage();
    }

    GpuMemoryHeap::~GpuMemoryHeap()
    {
        m_deferredReleases = decltype(m_deferredReleases)();
    }

    auto GpuMemoryHeap::create(core::ref<Device> p_device, MemoryType memoryType, size_t pageSize, core::ref<Fence> const& p_fence) -> core::ref<GpuMemoryHeap>
    {
        return core::ref<GpuMemoryHeap>(new GpuMemoryHeap(p_device, memoryType, pageSize, p_fence));
    }

    auto GpuMemoryHeap::allocateNewPage() -> void
    {
        if (m_activePage)
        {
            m_usedPages[m_currentPageId] = std::move(m_activePage);
        }

        if (!m_availablePages.empty())
        {
            m_activePage = std::move(m_availablePages.front());
            m_availablePages.pop();
            m_activePage->allocationsCount = 0;
            m_activePage->currentOffset = 0;
        }
        else
        {
            m_activePage = std::make_unique<PageData>();
            initBasePageData((*m_activePage), m_pageSize);
        }

        m_activePage->currentOffset = 0;
        m_currentPageId++;
    }

    auto GpuMemoryHeap::allocate(size_t size, size_t alignment) -> Allocation
    {
        Allocation data;
        if (size > m_pageSize)
        {
            data.pageID = GpuMemoryHeap::Allocation::kMegaPageId;
            initBasePageData(data, size);
        }
        else
        {
            // Calculate the start
            size_t currentOffset = align_up(m_activePage->currentOffset, alignment);
            if (currentOffset + size > m_pageSize)
            {
                currentOffset = 0;
                allocateNewPage();
            }

            data.pageID = m_currentPageId;
            data.size = (uint32_t)size;
            data.offset = currentOffset;
            data.data = m_activePage->data + currentOffset;
            data.gfxBuffer = m_activePage->gfxBuffer;
            m_activePage->currentOffset = currentOffset + size;
            m_activePage->allocationsCount++;
        }

        data.fenceValue = mp_fence->getSignaledValue();
        return data;
    }

    auto GpuMemoryHeap::allocate(size_t size, ResourceBindFlags bindFlags) -> Allocation
    {
        size_t alignment = mp_device->getBufferDataAlignment(bindFlags);
        return allocate(align_up(size, alignment), alignment);
    }

    auto GpuMemoryHeap::release(Allocation& data) -> void
    {
        AP_ASSERT(data.gfxBuffer, "Allocation must have a valid buffer resource.");
        m_deferredReleases.push(data);
    }

    auto GpuMemoryHeap::executeDeferredReleases() -> void
    {
        uint64_t currentValue = mp_fence->getCurrentValue();
        while (!m_deferredReleases.empty() && m_deferredReleases.top().fenceValue < currentValue)
        {
            Allocation const& data = m_deferredReleases.top();
            if (data.pageID == m_currentPageId)
            {
                m_activePage->allocationsCount--;
                if (m_activePage->allocationsCount == 0)
                {
                    m_activePage->currentOffset = 0;
                }
            }
            else
            {
                if (data.pageID != Allocation::kMegaPageId)
                {
                    auto& pData = m_usedPages[data.pageID];
                    pData->allocationsCount--;
                    if (pData->allocationsCount == 0)
                    {
                        m_availablePages.push(std::move(pData));
                        m_usedPages.erase(data.pageID);
                    }
                }
            }
            m_deferredReleases.pop();
        }
    }

    auto GpuMemoryHeap::initBasePageData(BaseData& data, size_t size) -> void
    {
        data.gfxBuffer = createBufferResource(
            core::ref<Device>(mp_device.get()),
            getInitState(m_memoryType),
            size,
            0,
            ResourceFormat::Unknown,
            ResourceBindFlags::Vertex | ResourceBindFlags::Index | ResourceBindFlags::Constant,
            m_memoryType
        );
        data.size = (uint32_t)size;
        data.offset = 0;
        rhi::CpuAccessMode accessMode = (m_memoryType == MemoryType::ReadBack) ? rhi::CpuAccessMode::Read : rhi::CpuAccessMode::Write;
        checkResult(mp_device->getGfxDevice()->mapBuffer(data.gfxBuffer, accessMode, (void**)&data.data), "Failed to map buffer resource");
    }

    auto GpuMemoryHeap::breakStrongReferenceToDevice() -> void
    {
        mp_device.breakStrongReference();
    }
} // namespace april::graphics
