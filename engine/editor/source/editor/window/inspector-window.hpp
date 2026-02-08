#pragma once

#include <editor/tool-window.hpp>
#include <scene/ecs-core.hpp>

#include <string>

namespace april::editor
{
    class InspectorWindow final : public ToolWindow
    {
    public:
        InspectorWindow() = default;

        [[nodiscard]] auto title() const -> char const* override { return "Inspector"; }
        auto onUIRender(EditorContext& context) -> void override;

    private:
        scene::Entity m_lastEntity{scene::NullEntity};
        std::string m_tagBuffer{};
        std::string m_meshAssetBuffer{};
        std::string m_materialAssetBuffer{};
    };
}
