#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-reflection.hpp>
#include <graphics/rhi/render-device.hpp>

using namespace april::graphics;

namespace
{
    bool hasMember(ProgramReflection const* reflector, std::string_view name)
    {
        if (!reflector) return false;
        if (reflector->findMember(name)) return true;

        for (auto const& group : reflector->getEntryPointGroups())
        {
            if (group && group->findMember(name)) return true;
        }
        return false;
    }

    bool hasNestedMember(ProgramReflection const* reflector, std::string_view blockName, std::string_view memberName)
    {
        if (!reflector) return false;
        auto blockVar = reflector->findMember(blockName);
        if (!blockVar) return false;

        auto blockType = blockVar->getType();
        if (!blockType) return false;

        auto structType = blockType->asStructType();
        if (structType && structType->getMember(memberName))
        {
            return true;
        }

        for (auto const& group : reflector->getEntryPointGroups())
        {
            if (!group) continue;
            auto groupVar = group->findMember(blockName);
            if (!groupVar) continue;

            auto groupType = groupVar->getType();
            if (!groupType) continue;

            auto groupStruct = groupType->asStructType();
            if (groupStruct && groupStruct->getMember(memberName))
            {
                return true;
            }
        }
        return false;
    }
}

TEST_SUITE("Slang Reflection")
{
    TEST_CASE("Blit Shader Parameters")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            ProgramDesc progDesc;
            progDesc.addShaderLibrary("blit.slang");
            progDesc.vsEntryPoint("vertexMain");
            progDesc.psEntryPoint("fragmentMain");

            auto program = Program::create(device, progDesc);
            REQUIRE(program);

            auto reflector = program->getReflector();
            REQUIRE(reflector);

            CHECK_MESSAGE(hasMember(reflector.get(), "sourceTexture"), "Missing sourceTexture in reflection");
            CHECK_MESSAGE(hasMember(reflector.get(), "sourceSampler"), "Missing sourceSampler in reflection");

            bool hasUvTransform = hasMember(reflector.get(), "uvTransform");
            bool hasUniformBlock = hasNestedMember(reflector.get(), "BlitUniforms", "uvTransform");
            CHECK_MESSAGE(
                (hasUvTransform || hasUniformBlock) == true,
                "Missing uvTransform (either root or BlitUniforms.uvTransform) in reflection"
            );
        }
    }
}
