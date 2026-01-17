#pragma once

#include <core/foundation/object.hpp>

namespace april
{
    struct EngineConfig
    {
        bool enableEditor{true};
    };

    class Engine : public core::Object
    {
        APRIL_OBJECT(Engine)
    public:
        Engine(const EngineConfig& config = {});
        ~Engine() override;

        static auto get() -> Engine&;

        auto getConfig() const -> const EngineConfig& { return m_config; }

    private:
        EngineConfig m_config;
        static Engine* s_instance;
    };
}
