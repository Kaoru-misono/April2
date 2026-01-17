// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "program-version.hpp"
#include "program.hpp"
#include "program-manager.hpp"
#include "program-variables.hpp"
#include "rhi/render-device.hpp"
#include "rhi/parameter-block.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>

#include <slang.h>
#include <set>

namespace april::graphics
{
    // EntryPointGroupKernels

    auto EntryPointGroupKernels::create(Type type, std::vector<core::ref<EntryPointKernel>> const& kernels, std::string const& exportName) -> core::ref<EntryPointGroupKernels const>
    {
        return core::ref<EntryPointGroupKernels const>(new EntryPointGroupKernels(type, kernels, exportName));
    }

    EntryPointGroupKernels::EntryPointGroupKernels(Type type, std::vector<core::ref<EntryPointKernel>> const& kernels, std::string const& exportName)
        : m_type(type), m_kernels(kernels), m_exportName(exportName)
    {}

    auto EntryPointGroupKernels::getKernel(ShaderType type) const -> EntryPointKernel const*
    {
        for (auto const& kernel : m_kernels)
        {
            if (kernel->getType() == type) return kernel.get();
        }
        return nullptr;
    }

    // ProgramKernels

    ProgramKernels::ProgramKernels(
        ProgramVersion const* pVersion,
        core::ref<ProgramReflection const> pReflector,
        UniqueEntryPointGroups const& uniqueEntryPointGroups,
        std::string const& name
    )
        : m_name(name), m_uniqueEntryPointGroups(uniqueEntryPointGroups), m_reflector(pReflector), m_version(pVersion)
    {}

    auto ProgramKernels::create(
        Device* pDevice,
        ProgramVersion const* pVersion,
        slang::IComponentType* pSpecializedSlangGlobalScope,
        std::vector<slang::IComponentType*> const& typeConformanceSpecializedEntryPoints,
        core::ref<ProgramReflection const> pReflector,
        UniqueEntryPointGroups const& uniqueEntryPointGroups,
        std::string& log,
        std::string const& name
    ) -> core::ref<ProgramKernels>
    {
        auto pProgram = core::ref<ProgramKernels>(new ProgramKernels(pVersion, pReflector, uniqueEntryPointGroups, name));

        rhi::ShaderProgramDesc programDesc = {};
        programDesc.linkingStyle = rhi::LinkingStyle::SeparateEntryPointCompilation;
        programDesc.slangGlobalScope = pSpecializedSlangGlobalScope;

        bool isRayTracingProgram = false;
        if (!typeConformanceSpecializedEntryPoints.empty())
        {
            auto stage = typeConformanceSpecializedEntryPoints[0]->getLayout()->getEntryPointByIndex(0)->getStage();
            switch (stage)
            {
            case SLANG_STAGE_ANY_HIT:
            case SLANG_STAGE_RAY_GENERATION:
            case SLANG_STAGE_CLOSEST_HIT:
            case SLANG_STAGE_CALLABLE:
            case SLANG_STAGE_INTERSECTION:
            case SLANG_STAGE_MISS:
                isRayTracingProgram = true;
                break;
            default:
                break;
            }
        }

        std::vector<slang::IComponentType*> deduplicatedEntryPoints;
        if (isRayTracingProgram)
        {
            std::set<std::string> entryPointNames;
            for (auto entryPoint : typeConformanceSpecializedEntryPoints)
            {
                auto compiledEntryPointName = std::string(entryPoint->getLayout()->getEntryPointByIndex(0)->getNameOverride());
                if (entryPointNames.find(compiledEntryPointName) == entryPointNames.end())
                {
                    entryPointNames.insert(compiledEntryPointName);
                    deduplicatedEntryPoints.push_back(entryPoint);
                }
            }
            programDesc.slangEntryPointCount = (uint32_t)deduplicatedEntryPoints.size();
            programDesc.slangEntryPoints = (slang::IComponentType**)deduplicatedEntryPoints.data();
        }
        else
        {
            programDesc.slangEntryPointCount = (uint32_t)typeConformanceSpecializedEntryPoints.size();
            programDesc.slangEntryPoints = (slang::IComponentType**)typeConformanceSpecializedEntryPoints.data();
        }

        rhi::ComPtr<ISlangBlob> diagnostics;
        if (SLANG_FAILED(pDevice->getGfxDevice()->createShaderProgram(programDesc, pProgram->m_gfxShaderProgram.writeRef(), diagnostics.writeRef())))
        {
            pProgram = nullptr;
        }
        if (diagnostics)
        {
            log = (const char*)diagnostics->getBufferPointer();
        }

        return pProgram;
    }

    auto ProgramKernels::getKernel(int32_t type) const -> EntryPointKernel const*
    {
        for (auto const& group : m_uniqueEntryPointGroups)
        {
            if (auto kernel = group->getKernel((ShaderType)type)) return kernel;
        }
        return nullptr;
    }

    // ProgramVersion

    ProgramVersion::ProgramVersion(Program* pProgram, slang::IComponentType* pSlangGlobalScope)
        : m_program(pProgram), m_slangGlobalScope(pSlangGlobalScope)
    {
        AP_ASSERT(pProgram, "Program is null");
    }

    auto ProgramVersion::init(
        DefineList const& defineList,
        core::ref<ProgramReflection const> pReflector,
        std::string const& name,
        std::vector<rhi::ComPtr<slang::IComponentType>> const& slangEntryPoints
    ) -> void
    {
        AP_ASSERT(pReflector, "Reflector is null");
        m_defines = defineList;
        m_reflector = pReflector;
        m_name = name;
        m_slangEntryPoints = slangEntryPoints;
    }

    auto ProgramVersion::createEmpty(Program* pProgram, slang::IComponentType* pSlangGlobalScope) -> core::ref<ProgramVersion>
    {
        return core::ref<ProgramVersion>(new ProgramVersion(pProgram, pSlangGlobalScope));
    }

    auto ProgramVersion::getKernels(Device* pDevice, ProgramVariables const* pVars) const -> core::ref<ProgramKernels const>
    {
        std::string specializationKey;

        // Collect specialization args
        // ParameterBlock::SpecializationArgs is std::vector<slang::SpecializationArg>
        std::vector<slang::SpecializationArg> specializationArgs;
        if (pVars)
        {
            pVars->collectSpecializationArgs(specializationArgs);
        }

        bool first = true;
        for (auto const& arg : specializationArgs)
        {
            if (!first) specializationKey += ",";
            specializationKey += std::string(arg.type->getName()); // arg.type might be null if it's value specialization?
            // Falcor implementation assumes arg.type is valid or uses it.
            first = false;
        }

        auto it = m_kernels.find(specializationKey);
        if (it != m_kernels.end())
        {
            return it->second;
        }

        AP_ASSERT(m_program, "Program is null");

        for (;;)
        {
            std::string log;
            // Need ProgramManager::createProgramKernels
            auto pKernels = pDevice->getProgramManager()->createProgramKernels(*m_program, *this, *pVars, log);
            if (pKernels)
            {
                // Success

                if (!log.empty())
                {
                    AP_WARN("Warnings in program:\n{}\n{}", getName(), log);
                }

                m_kernels[specializationKey] = pKernels;
                return pKernels;
            }
            else
            {
                // Failure
                AP_CRITICAL("Failed to link program:\n{}\n\n{}", getName(), log);
            }
        }
    }

    auto ProgramVersion::getSlangSession() const -> slang::ISession*
    {
        return getSlangGlobalScope()->getSession();
    }

    auto ProgramVersion::getSlangGlobalScope() const -> slang::IComponentType*
    {
        return m_slangGlobalScope;
    }

    auto ProgramVersion::getSlangEntryPoint(uint32_t index) const -> slang::IComponentType*
    {
        return m_slangEntryPoints[index];
    }

} // namespace april::graphics
