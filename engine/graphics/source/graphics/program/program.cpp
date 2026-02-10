// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "program.hpp"
#include "program-manager.hpp"
#include "program-variables.hpp"
#include "rhi/render-device.hpp"
#include "rhi/parameter-block.hpp"
#include "rhi/ray-tracing-pipeline.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <core/tools/enum.hpp>

#include <slang.h>
#include <format>
#include <set>
#include <utility>

namespace april::graphics
{
    auto ProgramDesc::finalize() -> void
    {
        uint32_t globalIndex = 0;
        for (auto& entryPointGroup : entryPointGroups)
            for (auto& entryPoint : entryPointGroup.entryPoints)
                entryPoint.globalIndex = globalIndex++;
    }

    Program::Program(core::ref<Device> pDevice, ProgramDesc desc, DefineList defineList)
        : mp_device(pDevice), m_description(std::move(desc)), m_defineList(std::move(defineList)), m_typeConformanceList(m_description.typeConformances)
    {
        m_description.finalize();

        if (m_description.shaderModel == ShaderModel::Unknown)
        {
            m_description.shaderModel = mp_device->getDefaultShaderModel();
        }

        if (!mp_device->isShaderModelSupported(m_description.shaderModel))
        {
            AP_ERROR("Requested Shader Model {} is not supported by the device", enumToString(m_description.shaderModel));
        }

        if (m_description.hasEntryPoint(ShaderType::RayGeneration, "")) // Assuming hasEntryPoint(Type) was intended, checking overloading
        {
            if (desc.maxTraceRecursionDepth == uint32_t(-1))
            {
                AP_ERROR("Can't create a raytacing program without specifying maximum trace recursion depth");
            }
            if (desc.maxPayloadSize == uint32_t(-1))
            {
                AP_ERROR("Can't create a raytacing program without specifying maximum ray payload size");
            }
        }

        validateEntryPoints();

        mp_device->getProgramManager()->registerProgramForReload(this);
    }

    Program::~Program()
    {
        mp_device->getProgramManager()->unregisterProgramForReload(this);
        // m_programVersions cleared by destructor of map
    }

    auto Program::validateEntryPoints() const -> bool
    {
        using NameTypePair = std::pair<std::string, ShaderType>;
        std::set<NameTypePair> entryPointNamesAndTypes;
        for (auto const& group : m_description.entryPointGroups)
        {
            for (auto const& e : group.entryPoints)
            {
                if (!entryPointNamesAndTypes.insert(NameTypePair(e.exportName, e.type)).second)
                {
                    AP_WARN("Duplicate program entry points '{}' of type '{}'.", e.exportName, enumToString(e.type));
                }
            }
        }
        return true;
    }

    auto Program::getProgramDescString() const -> std::string
    {
        std::string desc;

        for (auto const& shaderModule : m_description.shaderModules)
        {
            for (auto const& source : shaderModule.sources)
            {
                switch (source.type)
                {
                case ProgramDesc::ShaderSource::Type::File:
                    desc += source.path.string();
                    break;
                case ProgramDesc::ShaderSource::Type::String:
                    desc += "<string>";
                    break;
                default:
                    AP_UNREACHABLE("Invalid shader source type");
                }
                desc += " ";
            }
        }

        desc += "(";
        size_t entryPointIndex = 0;
        for (auto const& entryPointGroup : m_description.entryPointGroups)
        {
            for (auto const& entryPoint : entryPointGroup.entryPoints)
            {
                if (entryPointIndex++ > 0)
                    desc += ", ";
                desc += entryPoint.exportName;
            }
        }
        desc += ")";

        return desc;
    }

    auto Program::addDefine(std::string const& name, std::string const& value) -> bool
    {
        if (m_defineList.find(name) != m_defineList.end())
        {
            if (m_defineList.at(name) == value)
            {
                return false;
            }
        }
        markDirty();
        m_defineList[name] = value;
        return true;
    }

    auto Program::addDefines(DefineList const& dl) -> bool
    {
        bool dirty = false;
        for (auto const& it : dl)
        {
            if (addDefine(it.first, it.second))
            {
                dirty = true;
            }
        }
        return dirty;
    }

    auto Program::removeDefine(std::string const& name) -> bool
    {
        if (m_defineList.find(name) != m_defineList.end())
        {
            markDirty();
            m_defineList.erase(name);
            return true;
        }
        return false;
    }

    auto Program::removeDefines(DefineList const& dl) -> bool
    {
        bool dirty = false;
        for (auto const& it : dl)
        {
            if (removeDefine(it.first))
            {
                dirty = true;
            }
        }
        return dirty;
    }

    auto Program::removeDefines(size_t pos, size_t len, std::string const& str) -> bool
    {
        bool dirty = false;
        for (auto it = m_defineList.cbegin(); it != m_defineList.cend();)
        {
            if (pos < it->first.length() && it->first.compare(pos, len, str) == 0)
            {
                markDirty();
                it = m_defineList.erase(it);
                dirty = true;
            }
            else
            {
                ++it;
            }
        }
        return dirty;
    }

    auto Program::setDefines(DefineList const& dl) -> bool
    {
        if (dl != m_defineList)
        {
            markDirty();
            m_defineList = dl;
            return true;
        }
        return false;
    }

    auto Program::addTypeConformance(std::string const& typeName, std::string const interfaceType, uint32_t id) -> bool
    {
        auto const before = m_typeConformanceList;
        m_typeConformanceList.add(typeName, interfaceType, id);
        if (m_typeConformanceList != before)
        {
            markDirty();
            return true;
        }
        return false;
    }

    auto Program::removeTypeConformance(std::string const& typeName, std::string const interfaceType) -> bool
    {
        auto const before = m_typeConformanceList;
        m_typeConformanceList.remove(typeName, interfaceType);
        if (m_typeConformanceList != before)
        {
            markDirty();
            return true;
        }
        return false;
    }

    auto Program::setTypeConformances(TypeConformanceList const& conformances) -> bool
    {
        if (conformances != m_typeConformanceList)
        {
            markDirty();
            m_typeConformanceList = conformances;
            return true;
        }
        return false;
    }

    auto Program::checkIfFilesChanged() -> bool
    {
        if (m_activeVersion == nullptr)
        {
            return false;
        }

        // Have any of the files we depend on changed?
        // ProgramManager handles file timestamps logic usually, but here checking m_fileTimestamps
        // Need getFileModifiedTime implementation or helper
        // Assuming OS::getFileModifiedTime exists or using std::filesystem
        for (auto& entry : m_fileTimestamps)
        {
            auto& path = entry.first;
            auto& modifiedTime = entry.second;

            auto ftime = std::filesystem::last_write_time(path);

            auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(ftime);

            std::time_t cftime = std::chrono::system_clock::to_time_t(sys_time);

            if (modifiedTime != cftime)
            {
                return true;
            }
        }
        return false;
    }

    auto Program::getActiveVersion() const -> core::ref<ProgramVersion const>
    {
        if (m_linkRequired)
        {
            ProgramVersionKey key{m_defineList, m_typeConformanceList};
            auto const& it = m_programVersions.find(key);
            if (it == m_programVersions.end())
            {
                if (link() == false)
                {
                    AP_ERROR("Program linkage failed");
                }
                else
                {
                    m_programVersions[key] = m_activeVersion;
                }
            }
            else
            {
                m_activeVersion = it->second;
            }
            m_linkRequired = false;
        }
        AP_ASSERT(m_activeVersion, "Active version is null");
        return m_activeVersion;
    }

    auto Program::validateConformancesPreflight() const -> bool
    {
        // Keep this hook for future semantic checks. Avoid path-name heuristics that can
        // reject valid reflection/analysis programs before Slang semantic diagnostics run.
        return true;
    }

    auto Program::link() const -> bool
    {
        validateConformancesPreflight();

        while (1)
        {
            std::string log;
            auto pVersion = mp_device->getProgramManager()->createProgramVersion(*this, log);

            if (pVersion == nullptr)
            {
                std::string conformanceSummary;
                for (auto const& [conformance, id] : m_typeConformanceList)
                {
                    conformanceSummary += std::format(
                        "  - {} -> {} (id={})\n",
                        conformance.interfaceName,
                        conformance.typeName,
                        id
                    );
                }
                if (conformanceSummary.empty())
                {
                    conformanceSummary = "  (none)\n";
                }

                std::string msg = "Failed to link program:\n" + getProgramDescString() +
                    "\n\nType conformances:\n" + conformanceSummary + "\n" + log;
                // reportErrorAndAllowRetry logic omitted
                AP_ERROR("{}", msg);
                return false;
            }
            else
            {
                if (!log.empty())
                {
                    AP_WARN("Warnings in program:\n{} {}", getProgramDescString(), log);
                }

                m_activeVersion = pVersion;
                return true;
            }
        }
        return false;
    }

    auto Program::reset() -> void
    {
        m_activeVersion = nullptr;
        m_programVersions.clear();
        m_fileTimestamps.clear();
        m_linkRequired = true;
    }

    auto Program::breakStrongReferenceToDevice() -> void
    {
        mp_device.breakStrongReference();
    }

    auto Program::getRtso(RtProgramVariables* pVars) -> core::ref<RayTracingPipeline>
    {
        auto pProgramVersion = getActiveVersion();
        auto pProgramKernels = pProgramVersion->getKernels(mp_device, pVars);

        m_rtStateGraph.walk((void*)pProgramKernels.get());

        core::ref<RayTracingPipeline> pRtso = m_rtStateGraph.getCurrentNode();

        if (pRtso == nullptr)
        {
            RayTracingPipelineDesc desc;
            desc.programKernels = pProgramKernels;
            desc.maxTraceRecursionDepth = m_description.maxTraceRecursionDepth;
            desc.pipelineFlags = m_description.rtPipelineFlags;

            auto cmpFunc = [&desc](core::ref<RayTracingPipeline> pRtso) -> bool { return pRtso && (desc == pRtso->getDesc()); };

            if (m_rtStateGraph.scanForMatchingNode(cmpFunc))
            {
                pRtso = m_rtStateGraph.getCurrentNode();
            }
            else
            {
                pRtso = mp_device->createRayTracingPipeline(desc);
                m_rtStateGraph.setCurrentNodeData(pRtso);
            }
        }

        return pRtso;
    }

} // namespace april::graphics
