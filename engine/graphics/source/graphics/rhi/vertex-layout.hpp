// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "rhi/format.hpp"

#include <core/foundation/object.hpp>
#include <slang-rhi.h>
#include <vector>
#include <string>

namespace april::graphics
{
    class VertexBufferLayout : public core::Object
    {
        APRIL_OBJECT(VertexBufferLayout)
    public:
        enum class InputClass
        {
            PerVertexData,
            PerInstanceData
        };

        static auto create() -> core::ref<VertexBufferLayout> { return core::ref<VertexBufferLayout>(new VertexBufferLayout()); }

        auto addElement(std::string const& name, uint32_t offset, ResourceFormat format, uint32_t arraySize, uint32_t shaderLocation) -> void
        {
            Element elem;
            elem.offset = offset;
            elem.format = format;
            elem.shaderLocation = shaderLocation;
            elem.name = name;
            elem.arraySize = arraySize;
            m_elements.push_back(elem);
            m_vertexStride += getFormatBytesPerBlock(elem.format) * elem.arraySize;
        }

        auto getElementOffset(uint32_t index) const -> uint32_t { return m_elements[index].offset; }
        auto getElementFormat(uint32_t index) const -> ResourceFormat { return m_elements[index].format; }
        auto getElementName(uint32_t index) const -> std::string const& { return m_elements[index].name; }
        auto getElementArraySize(uint32_t index) const -> uint32_t { return m_elements[index].arraySize; }
        auto getElementShaderLocation(uint32_t index) const -> uint32_t { return m_elements[index].shaderLocation; }
        auto getElementCount() const -> uint32_t { return (uint32_t)m_elements.size(); }

        auto getStride() const -> uint32_t { return m_vertexStride; }
        auto getInputClass() const -> InputClass { return m_class; }
        auto getInstanceStepRate() const -> uint32_t { return m_instanceStepRate; }

        auto setInputClass(InputClass inputClass, uint32_t stepRate) -> void
        {
            m_class = inputClass;
            m_instanceStepRate = stepRate;
        }

        static constexpr uint32_t kInvalidShaderLocation = uint32_t(-1);

    private:
        VertexBufferLayout() = default;

        struct Element
        {
            uint32_t offset{0};
            ResourceFormat format{ResourceFormat::Unknown};
            uint32_t shaderLocation{kInvalidShaderLocation};
            std::string name;
            uint32_t arraySize{1};
            uint32_t vbIndex{0};
        };

        std::vector<Element> m_elements{};
        InputClass m_class{InputClass::PerVertexData};
        uint32_t m_instanceStepRate{0};
        uint32_t m_vertexStride{0};
    };

    class VertexLayout : public core::Object
    {
        APRIL_OBJECT(VertexLayout)
    public:
        static auto create() -> core::ref<VertexLayout> { return core::ref<VertexLayout>(new VertexLayout()); }

        auto addBufferLayout(uint32_t index, core::ref<VertexBufferLayout> layout) -> void
        {
            if (mp_bufferLayouts.size() <= index)
            {
                mp_bufferLayouts.resize(index + 1);
            }
            mp_bufferLayouts[index] = layout;

            m_layoutDirty = true;
        }

        auto getBufferLayout(size_t index) const -> core::ref<VertexBufferLayout> const& { return mp_bufferLayouts[index]; }
        auto getBufferCount() const -> size_t { return mp_bufferLayouts.size(); }
        auto getGfxInputLayout() const -> rhi::IInputLayout*;

    private:
        VertexLayout() { mp_bufferLayouts.reserve(16); }
        auto createGfxInputLayout() -> void;

        bool m_layoutDirty{true};
        rhi::ComPtr<rhi::IInputLayout> m_inputLayout{};
        std::vector<core::ref<VertexBufferLayout>> mp_bufferLayouts{};
    };
}
