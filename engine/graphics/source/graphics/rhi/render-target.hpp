#pragma once

#include "format.hpp"
#include "fwd.hpp"
#include "math/type.hpp"
#include "resource-views.hpp"
#include "texture.hpp"

#include <core/foundation/object.hpp>
#include <vector>
#include <unordered_set>

namespace april::graphics
{
    enum class LoadOp
    {
        Load,
        Clear,
        DontCare
    };

    enum class StoreOp
    {
        Store,
        DontCare
    };

    struct Attachment
    {
        core::ref<Texture> texture{};
        core::ref<ResourceView> nullView{};
        uint32_t mipLevel{0};
        uint32_t arraySize{1};
        uint32_t firstArraySlice{0};
    };

    struct ColorTarget final
    {
        ColorTarget() = default;
        ColorTarget(
            core::ref<RenderTargetView> const& view,
            LoadOp loadOp,
            StoreOp storeOp,
            float4 clearColor = float4{0.0f, 0.0f, 0.0f, 1.0f},
            bool allowUav = false
        )
            : loadOp(loadOp)
            , storeOp(storeOp)
            , clearColor(clearColor)
            , allowUav(allowUav)
            , m_colorTargetView(view)
        {}

        auto getGfxTextureView() const -> rhi::ITextureView* { return m_colorTargetView->getGfxTextureView(); }
        explicit operator bool () const { return m_colorTargetView; }

        LoadOp loadOp{LoadOp::Load};
        StoreOp storeOp{StoreOp::Store};
        float4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
        bool allowUav{false};
        core::ref<RenderTargetView> m_colorTargetView{};
    };
    using ColorTargets = std::vector<ColorTarget>;

    struct DepthStencilTarget final
    {
        DepthStencilTarget() = default;
        DepthStencilTarget(
            core::ref<DepthStencilView> depthStencilView,
            LoadOp depthLoadOp,
            StoreOp depthStoreOp,
            float clearDepth,
            LoadOp stencilLoadOp,
            StoreOp stencilStoreOp,
            uint8_t clearStencil,
            bool allowUav
        )
            : depthLoadOp(depthLoadOp)
            , depthStoreOp(depthStoreOp)
            , clearDepth(clearDepth)
            , stencilLoadOp(stencilLoadOp)
            , stencilStoreOp(stencilStoreOp)
            , clearStencil(clearStencil)
            , allowUav(allowUav)
            , m_depthStencilView(depthStencilView)
        {}

        DepthStencilTarget(
            core::ref<DepthStencilView> depthStencilView,
            LoadOp depthLoadOp,
            StoreOp depthStoreOp,
            float clearDepth = 1.0f,
            bool allowUav = false
        )
            : DepthStencilTarget(depthStencilView, depthLoadOp, depthStoreOp, clearDepth, LoadOp::DontCare, StoreOp::DontCare, 0, allowUav)
        {}

        auto getGfxTextureView() const -> rhi::ITextureView* { return m_depthStencilView->getGfxTextureView(); }
        explicit operator bool () const { return m_depthStencilView; }

        LoadOp depthLoadOp{LoadOp::Load};
        StoreOp depthStoreOp{StoreOp::Store};
        float clearDepth{1.0f};
        LoadOp stencilLoadOp{LoadOp::DontCare};
        StoreOp stencilStoreOp{StoreOp::DontCare};
        uint8_t clearStencil{0};
        bool allowUav{false};
        core::ref<DepthStencilView> m_depthStencilView{};
    };
}
