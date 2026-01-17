// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "buffer.hpp"
#include "fence.hpp"

#include <core/foundation/object.hpp>
#include <slang-rhi.h>
#include <queue>
#include <unordered_map>
#include <memory>

namespace april::graphics
{
    class Device;

    class GpuMemoryHeap : public core::Object
    {
        APRIL_OBJECT(GpuMemoryHeap)
    public:
        struct BaseData
        {
            rhi::ComPtr<rhi::IBuffer> gfxBuffer;
            uint32_t size{0};
            uint64_t offset{0}; // Using uint64_t for GpuAddress
            uint8_t* data{nullptr};

            auto getGpuAddress() const -> uint64_t { return gfxBuffer->getDeviceAddress() + offset; }
        };

        struct Allocation : public BaseData
        {
            uint64_t pageID{0};
            uint64_t fenceValue{0};

            static constexpr uint64_t kMegaPageId = 0xffffffffffffffff;
            auto operator<(Allocation const& other) const -> bool { return fenceValue > other.fenceValue; }
        };

        virtual ~GpuMemoryHeap();

        /**
         * Create a new GPU memory heap.
         * @param[in] memoryType The memory type of heap.
         * @param[in] pageSize Page size in bytes.
         * @param[in] pFence Fence to use for synchronization.
         * @return A new object, or throws an exception if creation failed.
         */
        static auto create(core::ref<Device> p_device, MemoryType memoryType, size_t pageSize, core::ref<Fence> const& p_fence) -> core::ref<GpuMemoryHeap>;

        auto allocate(size_t size, size_t alignment = 1) -> Allocation;
        auto allocate(size_t size, ResourceBindFlags bindFlags) -> Allocation;
        auto release(Allocation& data) -> void;
        auto getPageSize() const -> size_t { return m_pageSize; }
        auto executeDeferredReleases() -> void;

        auto breakStrongReferenceToDevice() -> void;

    private:
        GpuMemoryHeap(core::ref<Device> p_device, MemoryType memoryType, size_t pageSize, core::ref<Fence> const& p_fence);

        struct PageData : public BaseData
        {
            uint32_t allocationsCount{0};
            size_t currentOffset{0};
            using UniquePtr = std::unique_ptr<PageData>;
        };

        core::BreakableReference<Device> mp_device;
        MemoryType m_memoryType{};
        core::ref<Fence> mp_fence{};
        size_t m_pageSize{0};
        size_t m_currentPageId{0};
        PageData::UniquePtr m_activePage{};

        std::priority_queue<Allocation> m_deferredReleases{};
        std::unordered_map<size_t, PageData::UniquePtr> m_usedPages{};
        std::queue<PageData::UniquePtr> m_availablePages{};

        auto allocateNewPage() -> void;
        auto initBasePageData(BaseData& data, size_t size) -> void;
    };
} // namespace april::graphics
