#include <runtime/engine.hpp>

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Game";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::Default;

    april::Engine engine(config);
    return engine.run();
}
