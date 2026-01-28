#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-manager.hpp>
#include <graphics/rhi/render-device.hpp>

using namespace april::graphics;

TEST_SUITE("Conformance")
{
    TEST_CASE("Type Conformance Linkage")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        // To really test conformance, we should use a global interface variable or specialized entry point.
        // But Slang's dynamic dispatch often requires conformances to be registered.

        char const* shaderSourceWithInterface = R"(
            interface IBar {
                float getVal();
            };

            struct BarA : IBar {
                float getVal() { return 1.0; }
            };

            struct VSOut {
                float4 pos : SV_Position;
            };

            ParameterBlock<IBar> gBar;

            VSOut main() {
                VSOut output;
                output.pos = float4(gBar.getVal(), 0.0, 0.0, 1.0);
                return output;
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("ConformanceVS").addString(shaderSourceWithInterface, "ConformanceVS.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        // Add Type Conformance
        // Note: In Slang, linking an implementation to an interface can be done via addTypeConformance
        program->addTypeConformance("BarA", "IBar", 0);

        auto version = program->getActiveVersion();
        REQUIRE(version);

        CHECK(version != nullptr);
    }
}
