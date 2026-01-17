#pragma once

#include <core/foundation/object.hpp>
#include <string>

namespace april::graphics
{
    /**
     * @brief Base class for editor panels.
     */
    class EditorPanel : public core::Object
    {
        APRIL_OBJECT(EditorPanel)
    public:
        explicit EditorPanel(std::string name) : m_name(std::move(name)) {}
        ~EditorPanel() override = default;

        virtual auto onUpdate() -> void {}
        virtual auto onUIRender() -> void = 0;

        [[nodiscard]] auto getName() const -> const std::string& { return m_name; }
        [[nodiscard]] auto isOpen() const -> bool { return m_isOpen; }
        auto setOpen(bool open) -> void { m_isOpen = open; }

    protected:
        std::string m_name;
        bool m_isOpen{true};
    };
}
