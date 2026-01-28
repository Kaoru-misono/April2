#include "engine.hpp"
#include <core/error/assert.hpp>

namespace april
{
    Engine* Engine::s_instance = nullptr;

    Engine::Engine(EngineConfig const& config)
        : m_config(config)
    {
        AP_ASSERT(s_instance == nullptr, "Only one Engine instance allowed.");
        s_instance = this;
    }

    Engine::~Engine()
    {
        s_instance = nullptr;
    }

    auto Engine::get() -> Engine&
    {
        AP_ASSERT(s_instance != nullptr, "Engine instance not created.");
        return *s_instance;
    }
}
