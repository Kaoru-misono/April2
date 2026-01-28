// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "program-manager.hpp"
#include "program.hpp"
#include "program-version.hpp"
#include "program-variables.hpp"
#include "rhi/render-device.hpp"
#include "tools/enum-flags.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <core/file/vfs.hpp>
#include <slang.h>
#include <filesystem>
#include <vector>
#include <chrono>

namespace april::graphics
{
    inline namespace
    {
        auto getShaderDirectoriesList() -> std::vector<std::filesystem::path>
        {
            return {};
        }

        auto findFileInShaderDirectories(std::filesystem::path const& path, std::filesystem::path& fullPath)
        {
            if (path.is_absolute() && std::filesystem::exists(path))
            {
                fullPath = path;
                return true;
            }
            for (auto const& dir : getShaderDirectoriesList())
            {
                fullPath = dir / path;
                if (std::filesystem::exists(fullPath))
                {
                    return true;
                }
            }
            return false;
        }

    }
    static auto getSlangStage(ShaderType type) -> SlangStage
    {
        switch (type)
        {
        case ShaderType::Vertex: return SLANG_STAGE_VERTEX;
        case ShaderType::Pixel: return SLANG_STAGE_PIXEL;
        case ShaderType::Geometry: return SLANG_STAGE_GEOMETRY;
        case ShaderType::Hull: return SLANG_STAGE_HULL;
        case ShaderType::Domain: return SLANG_STAGE_DOMAIN;
        case ShaderType::Compute: return SLANG_STAGE_COMPUTE;
        case ShaderType::RayGeneration: return SLANG_STAGE_RAY_GENERATION;
        case ShaderType::Intersection: return SLANG_STAGE_INTERSECTION;
        case ShaderType::AnyHit: return SLANG_STAGE_ANY_HIT;
        case ShaderType::ClosestHit: return SLANG_STAGE_CLOSEST_HIT;
        case ShaderType::Miss: return SLANG_STAGE_MISS;
        case ShaderType::Callable: return SLANG_STAGE_CALLABLE;
        default: AP_CRITICAL("Unknown shader type"); return SLANG_STAGE_NONE;
        }
    }

    static auto getSlangProfileString(ShaderModel shaderModel) -> std::string
    {
        return std::format("sm_{}_{}", getShaderModelMajorVersion(shaderModel), getShaderModelMinorVersion(shaderModel));
    }

    static auto doSlangReflection(
        ProgramVersion const& programVersion,
        slang::IComponentType* pSlangGlobalScope,
        std::vector<Slang::ComPtr<slang::IComponentType>> const& pSlangLinkedEntryPoints,
        core::ref<ProgramReflection const>& pReflector,
        std::string& log
    ) -> bool
    {
        auto pSlangGlobalScopeLayout = pSlangGlobalScope->getLayout();

        std::vector<slang::EntryPointLayout*> pSlangEntryPointReflectors;

        for (auto const& pSlangLinkedEntryPoint : pSlangLinkedEntryPoints)
        {
            auto pSlangEntryPointLayout = pSlangLinkedEntryPoint->getLayout()->getEntryPointByIndex(0);
            pSlangEntryPointReflectors.push_back(pSlangEntryPointLayout);
        }

        pReflector = ProgramReflection::create(&programVersion, pSlangGlobalScopeLayout, pSlangEntryPointReflectors, log);

        return true;
    }

    ProgramManager::ProgramManager(Device* device) : mp_device(device)
    {
        VFS::mount("shader", std::filesystem::current_path() / "shader/graphics");
        DefineList globalDefines = {};

        addGlobalDefines(globalDefines);
    }

    auto ProgramManager::createProgramVersion(Program const& program, std::string& log) const -> core::ref<ProgramVersion const>
    {
        auto pSlangRequest = createSlangCompileRequest(program);
        if (!pSlangRequest) return nullptr;

        SlangResult slangResult = spCompile(pSlangRequest);
        log += spGetDiagnosticOutput(pSlangRequest);
        if (SLANG_FAILED(slangResult))
        {
            spDestroyCompileRequest(pSlangRequest);
            return nullptr;
        }

        Slang::ComPtr<slang::IComponentType> pSlangGlobalScope;
        spCompileRequest_getProgram(pSlangRequest, pSlangGlobalScope.writeRef());

        Slang::ComPtr<slang::ISession> pSlangSession(pSlangGlobalScope->getSession());

        std::vector<Slang::ComPtr<slang::IComponentType>> pSlangEntryPoints;
        for (auto const& entryPointGroup : program.getDescription().entryPointGroups)
        {
            for (auto const& entryPoint : entryPointGroup.entryPoints)
            {
                Slang::ComPtr<slang::IComponentType> pSlangEntryPoint;
                spCompileRequest_getEntryPoint(pSlangRequest, entryPoint.globalIndex, pSlangEntryPoint.writeRef());

                if (entryPoint.exportName != entryPoint.name)
                {
                    Slang::ComPtr<slang::IComponentType> pRenamedEntryPoint;
                    pSlangEntryPoint->renameEntryPoint(entryPoint.exportName.c_str(), pRenamedEntryPoint.writeRef());
                    pSlangEntryPoints.push_back(pRenamedEntryPoint);
                }
                else
                {
                    pSlangEntryPoints.push_back(pSlangEntryPoint);
                }
            }
        }

        int depFileCount = spGetDependencyFileCount(pSlangRequest);
        for (int ii = 0; ii < depFileCount; ++ii)
        {
            std::string depFilePath = spGetDependencyFilePath(pSlangRequest, ii);
            if (std::filesystem::exists(depFilePath))
            {
                auto ftime = std::filesystem::last_write_time(depFilePath);
                auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
                program.m_fileTimestamps[depFilePath] = std::chrono::system_clock::to_time_t(sys_time);
            }
        }

        // Note: the `ProgramReflection` needs to be able to refer back to the
        // `ProgramVersion`, but the `ProgramVersion` can't be initialized
        // until we have its reflection. We cut that dependency knot by
        // creating an "empty" program first, and then initializing it
        // after the reflection is created.
        //
        // TODO: There is no meaningful semantic difference between `ProgramVersion`
        // and `ProgramReflection`: they are one-to-one. Ideally in a future version
        // of Falcor they could be the same object.
        //
        // TODO @skallweit remove const cast
        auto pVersion = ProgramVersion::createEmpty(const_cast<Program*>(&program), pSlangGlobalScope);

        // Note: Because of interactions between how `SV_Target` outputs
        // and `u` register bindings work in Slang today (as a compatibility
        // feature for Shader Model 5.0 and below), we need to make sure
        // that the entry points are included in the component type we use
        // for reflection.
        //
        // TODO: Once Slang drops that behavior for SM 5.1+, we should be able
        // to just use `pSlangGlobalScope` for the reflection step instead
        // of `pSlangProgram`.
        //
        Slang::ComPtr<slang::IComponentType> pSlangProgram;
        spCompileRequest_getProgram(pSlangRequest, pSlangProgram.writeRef());

        core::ref<ProgramReflection const> pReflector;
        if (!doSlangReflection(*pVersion, pSlangGlobalScope, pSlangEntryPoints, pReflector, log))
        {
            return nullptr;
        }

        auto descStr = program.getProgramDescString();
        pVersion->init(program.getDefineList(), pReflector, descStr, pSlangEntryPoints);

        // timer.update();
        // double time = timer.delta();
        // mCompilationStats.programVersionCount++;
        // mCompilationStats.programVersionTotalTime += time;
        // mCompilationStats.programVersionMaxTime = std::max(mCompilationStats.programVersionMaxTime, time);
        // logDebug("Created program version in {:.3f} s: {}", timer.delta(), descStr);

        return pVersion;
    }

    auto ProgramManager::createProgramKernels(
        Program const& program,
        ProgramVersion const& programVersion,
        ProgramVariables const& programVars,
        std::string& log
    ) const -> core::ref<ProgramKernels const>
    {
        // TODO: Timer
        (void) programVars;

        auto pSlangGlobalScope = programVersion.getSlangGlobalScope();
        auto pSlangSession = pSlangGlobalScope->getSession();

        slang::IComponentType* pSpecializedSlangGlobalScope = pSlangGlobalScope;

        // Create a composite component type that represents all type conformances
        // linked into the `ProgramVersion`.
        auto createTypeConformanceComponentList = [&](TypeConformanceList const& typeConformances
                                                ) -> std::optional<Slang::ComPtr<slang::IComponentType>>
        {
            Slang::ComPtr<slang::IComponentType> pTypeConformancesCompositeComponent;
            std::vector<Slang::ComPtr<slang::ITypeConformance>> typeConformanceComponentList;
            std::vector<slang::IComponentType*> typeConformanceComponentRawPtrList;

            for (auto& typeConformance : typeConformances)
            {
                Slang::ComPtr<slang::IBlob> pSlangDiagnostics;
                Slang::ComPtr<slang::ITypeConformance> pTypeConformanceComponent;

                // Look for the type and interface type specified by the type conformance.
                // If not found we'll log an error and return.
                auto slangType = pSlangGlobalScope->getLayout()->findTypeByName(typeConformance.first.typeName.c_str());
                auto slangInterfaceType = pSlangGlobalScope->getLayout()->findTypeByName(typeConformance.first.interfaceName.c_str());
                if (!slangType)
                {
                    log += std::format("Type '{}' in type conformance was not found.\n", typeConformance.first.typeName.c_str());
                    return {};
                }
                if (!slangInterfaceType)
                {
                    log += std::format("Interface type '{}' in type conformance was not found.\n", typeConformance.first.interfaceName.c_str());
                    return {};
                }

                auto res = pSlangSession->createTypeConformanceComponentType(
                    slangType,
                    slangInterfaceType,
                    pTypeConformanceComponent.writeRef(),
                    (SlangInt)typeConformance.second,
                    pSlangDiagnostics.writeRef()
                );
                if (SLANG_FAILED(res))
                {
                    log += "Slang call createTypeConformanceComponentType() failed.\n";
                    return {};
                }
                if (pSlangDiagnostics && pSlangDiagnostics->getBufferSize() > 0)
                {
                    log += (char const*)pSlangDiagnostics->getBufferPointer();
                }
                if (pTypeConformanceComponent)
                {
                    typeConformanceComponentList.push_back(pTypeConformanceComponent);
                    typeConformanceComponentRawPtrList.push_back(pTypeConformanceComponent.get());
                }
            }
            if (!typeConformanceComponentList.empty())
            {
                Slang::ComPtr<slang::IBlob> pSlangDiagnostics;
                auto res = pSlangSession->createCompositeComponentType(
                    &typeConformanceComponentRawPtrList[0],
                    (SlangInt)typeConformanceComponentRawPtrList.size(),
                    pTypeConformancesCompositeComponent.writeRef(),
                    pSlangDiagnostics.writeRef()
                );
                if (SLANG_FAILED(res))
                {
                    log += "Slang call createCompositeComponentType() failed.\n";
                    return {};
                }
            }
            return pTypeConformancesCompositeComponent;
        };

        // Create one composite component type for the type conformances of each entry point group.
        // The type conformances for each group is the combination of the global and group type conformances.
        std::vector<Slang::ComPtr<slang::IComponentType>> typeConformancesCompositeComponents;
        typeConformancesCompositeComponents.reserve(program.m_description.entryPointGroups.size());
        for (auto const& group : program.m_description.entryPointGroups)
        {
            TypeConformanceList typeConformances = program.m_typeConformanceList;
            typeConformances.add(group.typeConformances);
            if (auto typeConformanceComponentList = createTypeConformanceComponentList(typeConformances))
                typeConformancesCompositeComponents.emplace_back(*typeConformanceComponentList);
            else
                return nullptr;
        }

        std::vector<Slang::ComPtr<slang::IComponentType>> pTypeConformanceSpecializedEntryPoints{};
        std::vector<slang::IComponentType*> pTypeConformanceSpecializedEntryPointsRawPtr{};
        std::vector<Slang::ComPtr<slang::IComponentType>> pLinkedEntryPoints{};

         // Create a `IComponentType` for each entry point.
        for (size_t groupIndex = 0; groupIndex < program.m_description.entryPointGroups.size(); ++groupIndex)
        {
            auto const& entryPointGroup = program.m_description.entryPointGroups[groupIndex];

            for (auto const& entryPoint : entryPointGroup.entryPoints)
            {
                auto pSlangEntryPoint = programVersion.getSlangEntryPoint(entryPoint.globalIndex);

                Slang::ComPtr<slang::IBlob> pSlangDiagnostics;

                Slang::ComPtr<slang::IComponentType> pTypeComformanceSpecializedEntryPoint;
                if (typeConformancesCompositeComponents[groupIndex])
                {
                    slang::IComponentType* componentTypes[] = {pSlangEntryPoint, typeConformancesCompositeComponents[groupIndex]};
                    auto res = pSlangSession->createCompositeComponentType(
                        componentTypes, 2, pTypeComformanceSpecializedEntryPoint.writeRef(), pSlangDiagnostics.writeRef()
                    );
                    if (SLANG_FAILED(res))
                    {
                        log += "Slang call createCompositeComponentType() failed.\n";
                        return nullptr;
                    }
                }
                else
                {
                    pTypeComformanceSpecializedEntryPoint = pSlangEntryPoint;
                }
                pTypeConformanceSpecializedEntryPoints.push_back(pTypeComformanceSpecializedEntryPoint);
                pTypeConformanceSpecializedEntryPointsRawPtr.push_back(pTypeComformanceSpecializedEntryPoint.get());

                Slang::ComPtr<slang::IComponentType> pLinkedSlangEntryPoint;
                {
                    slang::IComponentType* componentTypes[] = {pSpecializedSlangGlobalScope, pTypeComformanceSpecializedEntryPoint};

                    auto res = pSlangSession->createCompositeComponentType(
                        componentTypes, 2, pLinkedSlangEntryPoint.writeRef(), pSlangDiagnostics.writeRef()
                    );
                    if (SLANG_FAILED(res))
                    {
                        log += "Slang call createCompositeComponentType() failed.\n";
                        return nullptr;
                    }
                }
                pLinkedEntryPoints.push_back(pLinkedSlangEntryPoint);
            }
        }

        // Once specialization and linking are completed we need to
        // re-run the reflection step.
        //
        // A key guarantee we get from Slang is that the relative
        // ordering of parameters at the global scope or within a
        // given entry-point group will not change, so that when
        // `ParameterBlock`s and their descriptor tables/sets are allocated
        // using the unspecialized `ProgramReflection`, they will still
        // be valid to bind to the specialized program.
        //
        // Still, the specialized reflector may differ from the
        // unspecialized reflector in a few key ways:
        //
        // * There may be additional registers/bindings allocated for
        //   the global scope to account for the data required by
        //   specialized shader parameters (e.g., now that we know
        //   an `IFoo` parameter should actually be a `Bar`, we need
        //   to allocate those `Bar` resources somewhere).
        //
        // * As a result of specialized global-scope parameters taking
        //   up additional bindings/registers, the bindings/registers
        //   allocated to entry points and entry-point groups may be
        //   shifted.
        //
        // Note: Because of interactions between how `SV_Target` outputs
        // and `u` register bindings work in Slang today (as a compatibility
        // feature for Shader Model 5.0 and below), we need to make sure
        // that the entry points are included in the component type we use
        // for reflection.
        //
        // TODO: Once the behavior is fixed in Slang for SM 5.1+, we can
        // eliminate this step and use `pSpecializedSlangGlobalScope` instead
        // of `pSpecializedSlangProgram`, so long as we are okay with dropping
        // support for SM5.0 and below.
        //
        Slang::ComPtr<slang::IComponentType> pSpecializedSlangProgram;
        {
            // We are going to compose the global scope (specialized) with
            // all the entry points. Note that we do *not* use the "linked"
            // versions of the entry points because those already incorporate
            // the global scope, and we'd end up with multiple copies of
            // the global scope in that case.
            //
            std::vector<slang::IComponentType*> componentTypesForProgram;
            componentTypesForProgram.push_back(pSpecializedSlangGlobalScope);

            // TODO: Eventually this would need to use the specialized
            // (but not linked) version of each entry point.
            //
            componentTypesForProgram.insert(
                componentTypesForProgram.end(), programVersion.getSlangEntryPoints().begin(), programVersion.getSlangEntryPoints().end()
            );

            // Add type conformances for all entry point groups.
            // TODO: Is it correct to put all these in the global scope?
            for (auto pTypeConformancesComposite : typeConformancesCompositeComponents)
            {
                if (pTypeConformancesComposite)
                {
                    componentTypesForProgram.push_back(pTypeConformancesComposite);
                }
            }

            auto res = pSlangSession->createCompositeComponentType(
                componentTypesForProgram.data(), componentTypesForProgram.size(), pSpecializedSlangProgram.writeRef()
            );
            if (SLANG_FAILED(res))
            {
                log += "Slang call createCompositeComponentType() failed.\n";
                return nullptr;
            }
        }

        core::ref<const ProgramReflection> pReflector;
        doSlangReflection(programVersion, pSpecializedSlangProgram, pLinkedEntryPoints, pReflector, log);

        // Create kernel objects for each entry point and cache them here.
        std::vector<core::ref<EntryPointKernel>> allKernels;
        for (auto const& entryPointGroup : program.m_description.entryPointGroups)
        {
            for (auto const& entryPoint : entryPointGroup.entryPoints)
            {
                auto pLinkedEntryPoint = pLinkedEntryPoints[entryPoint.globalIndex];
                core::ref<EntryPointKernel> kernel = EntryPointKernel::create(pLinkedEntryPoint, entryPoint.type, entryPoint.exportName);
                if (!kernel)
                    return nullptr;

                allKernels.push_back(std::move(kernel));
            }
        }

        // In order to construct the `ProgramKernels` we need to extract
        // the kernels for each entry-point group.
        //
        std::vector<core::ref<const EntryPointGroupKernels>> entryPointGroups;

        // TODO: Because we aren't actually specializing entry-point groups,
        // we will again loop over the original unspecialized entry point
        // groups from the `ProgramDesc`, and assume that they line up
        // one-to-one with the entries in `pLinkedEntryPointGroups`.
        //
        for (size_t groupIndex = 0; groupIndex < program.m_description.entryPointGroups.size(); ++groupIndex)
        {
            auto const& entryPointGroup = program.m_description.entryPointGroups[groupIndex];

            // For each entry-point group we will collect the compiled kernel
            // code for its constituent entry points, using the "linked"
            // version of the entry-point group.
            //
            std::vector<core::ref<EntryPointKernel>> kernels;
            for (auto const& entryPoint : entryPointGroup.entryPoints)
            {
                kernels.push_back(allKernels[entryPoint.globalIndex]);
            }
            auto pGroupReflector = pReflector->getEntryPointGroup((uint32_t) groupIndex);
            auto pEntryPointGroupKernels = createEntryPointGroupKernels(kernels, pGroupReflector);
            entryPointGroups.push_back(pEntryPointGroupKernels);
        }

        auto descStr = program.getProgramDescString();
        core::ref<const ProgramKernels> pProgramKernels = ProgramKernels::create(
            mp_device,
            &programVersion,
            pSpecializedSlangGlobalScope,
            pTypeConformanceSpecializedEntryPointsRawPtr,
            pReflector,
            entryPointGroups,
            log,
            descStr
        );

        // timer.update();
        // double time = timer.delta();
        // mCompilationStats.programKernelsCount++;
        // mCompilationStats.programKernelsTotalTime += time;
        // mCompilationStats.programKernelsMaxTime = std::max(mCompilationStats.programKernelsMaxTime, time);
        // logDebug("Created program kernels in {:.3f} s: {}", time, descStr);

        return pProgramKernels;
    }

    auto ProgramManager::createEntryPointGroupKernels(
        std::vector<core::ref<EntryPointKernel>> const& kernels,
        core::ref<EntryPointBaseReflection> const& reflector
    ) const -> core::ref<EntryPointGroupKernels const>
    {
        AP_ASSERT(!kernels.empty(), "Kernels must not be empty");

        switch (kernels[0]->getType())
        {
        case ShaderType::Vertex:
        case ShaderType::Pixel:
        case ShaderType::Geometry:
        case ShaderType::Hull:
        case ShaderType::Domain:
            return EntryPointGroupKernels::create(EntryPointGroupKernels::Type::Rasterization, kernels, kernels[0]->getEntryPointName());
        case ShaderType::Compute:
            return EntryPointGroupKernels::create(EntryPointGroupKernels::Type::Compute, kernels, kernels[0]->getEntryPointName());
        case ShaderType::AnyHit:
        case ShaderType::ClosestHit:
        case ShaderType::Intersection:
        {
            if (reflector->getResourceRangeCount() > 0 || reflector->getRootDescriptorRangeCount() > 0 ||
                reflector->getParameterBlockSubObjectRangeCount() > 0)
            {
                AP_CRITICAL("Local root signatures are not supported for raytracing entry points.");
            }
            std::string exportName = std::format("HitGroup{}", m_hitGroupID++);
            return EntryPointGroupKernels::create(EntryPointGroupKernels::Type::RayTracingHitGroup, kernels, exportName);
        }
        case ShaderType::RayGeneration:
        case ShaderType::Miss:
        case ShaderType::Callable:
            return EntryPointGroupKernels::create(EntryPointGroupKernels::Type::RayTracingSingleShader, kernels, kernels[0]->getEntryPointName());
        default:
            AP_UNREACHABLE();
        }
    }

    void ProgramManager::registerProgramForReload(Program* program)
    {
        m_loadedPrograms.push_back(program);
    }

    void ProgramManager::unregisterProgramForReload(Program* program)
    {
        std::erase(m_loadedPrograms, program);
    }

    bool ProgramManager::reloadAllPrograms(bool forceReload)
    {
        bool reloaded = false;
        for (auto program : m_loadedPrograms)
        {
            if (program->checkIfFilesChanged() || forceReload)
            {
                program->reset();
                reloaded = true;
            }
        }
        return reloaded;
    }

    void ProgramManager::addGlobalDefines(DefineList const& defineList)
    {
        m_globalDefineList.add(defineList);
        reloadAllPrograms(true);
    }

    void ProgramManager::removeGlobalDefines(DefineList const& defineList)
    {
        m_globalDefineList.remove(defineList);
        reloadAllPrograms(true);
    }

    void ProgramManager::setGenerateDebugInfoEnabled(bool enabled)
    {
        m_generateDebugInfo = enabled;
    }

    bool ProgramManager::isGenerateDebugInfoEnabled()
    {
        return m_generateDebugInfo;
    }

    void ProgramManager::setForcedCompilerFlags(ForcedCompilerFlags forcedCompilerFlags)
    {
        m_forcedCompilerFlags = forcedCompilerFlags;
        reloadAllPrograms(true);
    }

    auto ProgramManager::getForcedCompilerFlags() -> ForcedCompilerFlags
    {
        return m_forcedCompilerFlags;
    }

    auto ProgramManager::getHlslLanguagePrelude() const -> std::string
    {
        Slang::ComPtr<ISlangBlob> prelude;
        mp_device->getSlangGlobalSession()->getLanguagePrelude(SLANG_SOURCE_LANGUAGE_HLSL, prelude.writeRef());
        AP_ASSERT(prelude, "Failed to get Slang language prelude");

        return std::string(reinterpret_cast<char const*>(prelude->getBufferPointer()), prelude->getBufferSize());
    }

    auto ProgramManager::setHlslLanguagePrelude(std::string const& prelude) -> void
    {
        mp_device->getSlangGlobalSession()->setLanguagePrelude(SLANG_SOURCE_LANGUAGE_HLSL, prelude.c_str());
    }

    auto ProgramManager::createSlangCompileRequest(Program const& program) const -> SlangCompileRequest*
    {
        slang::IGlobalSession* pSlangGlobalSession = mp_device->getSlangGlobalSession();
        AP_ASSERT(pSlangGlobalSession, "Failed to get Slang global session");

        slang::SessionDesc sessionDesc;

        // Add our shader search paths as `#include` search paths for Slang.
        //
        // Note: Slang allows application to plug in a callback API to
        // implement file I/O, and this could be used instead of specifying
        // the data directories to Slang.
        //
        std::vector<std::string> searchPaths;
        std::vector<char const*> slangSearchPaths;
        for (auto& path : getShaderDirectoriesList())
        {
            searchPaths.push_back(path.string());
            slangSearchPaths.push_back(searchPaths.back().data());
        }
        sessionDesc.searchPaths = slangSearchPaths.data();
        sessionDesc.searchPathCount = (SlangInt)slangSearchPaths.size();

        slang::TargetDesc targetDesc;
        targetDesc.format = SLANG_TARGET_UNKNOWN;
        targetDesc.profile = pSlangGlobalSession->findProfile(getSlangProfileString(program.m_description.shaderModel).c_str());

        if (targetDesc.profile == SLANG_PROFILE_UNKNOWN)
        {
            AP_CRITICAL("Can't find Slang profile for shader model {}", program.m_description.shaderModel);
        }

        // Get compiler flags and adjust with forced flags.
        SlangCompilerFlags compilerFlags = program.m_description.compilerFlags;
        compilerFlags &= ~m_forcedCompilerFlags.disabled;
        compilerFlags |= m_forcedCompilerFlags.enabled;

        // Set floating point mode. If no shader compiler flags for this were set, we use Slang's default mode.
        bool flagFast = enum_has_any_flags(compilerFlags, SlangCompilerFlags::FloatingPointModeFast);
        bool flagPrecise = enum_has_any_flags(compilerFlags, SlangCompilerFlags::FloatingPointModePrecise);
        if (flagFast && flagPrecise)
        {
            AP_WARN(
                "Shader compiler flags 'FloatingPointModeFast' and 'FloatingPointModePrecise' can't be used simultaneously. Ignoring "
                "'FloatingPointModeFast'."
            );
            flagFast = false;
        }

        SlangFloatingPointMode slangFpMode = SLANG_FLOATING_POINT_MODE_DEFAULT;
        if (flagFast)
        {
            slangFpMode = SLANG_FLOATING_POINT_MODE_FAST;
        }
        else if (flagPrecise)
        {
            slangFpMode = SLANG_FLOATING_POINT_MODE_PRECISE;
        }

        targetDesc.floatingPointMode = slangFpMode;

        targetDesc.forceGLSLScalarBufferLayout = true;

        if (/* getEnvironmentVariable("FALCOR_USE_SLANG_SPIRV_BACKEND") == "1" || */ program.m_description.useSPIRVBackend)
        {
            targetDesc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
        }
        else
        {
            targetDesc.flags &= ~SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
        }

        char const* targetMacroName;

        // Pick the right target based on the current graphics API
        switch (mp_device->getType())
        {
        case Device::Type::D3D12:
            targetDesc.format = SLANG_DXIL;
            targetMacroName = "APRIL_D3D12";
            break;
        case Device::Type::Vulkan:
            targetDesc.format = SLANG_SPIRV;
            targetMacroName = "APRIL_VULKAN";
            break;
        default:
            AP_UNREACHABLE("");
        }

        // Pass any `#define` flags along to Slang, since we aren't doing our
        // own preprocessing any more.
        //
        std::vector<slang::PreprocessorMacroDesc> slangDefines;
        const auto addSlangDefine = [&slangDefines](char const* name, char const* value) { slangDefines.push_back({name, value}); };

        // Add global followed by program specific defines.
        for (auto const& shaderDefine : m_globalDefineList)
        {
            addSlangDefine(shaderDefine.first.c_str(), shaderDefine.second.c_str());
        }
        for (auto const& shaderDefine : program.getDefineList())
        {
            addSlangDefine(shaderDefine.first.c_str(), shaderDefine.second.c_str());
        }

        // Add a `#define`s based on the target and shader model.
        addSlangDefine(targetMacroName, "1");

        // Add a `#define` based on the shader model.
        std::string sm = std::format(
            "__SM_{}_{}__", getShaderModelMajorVersion(program.m_description.shaderModel), getShaderModelMinorVersion(program.m_description.shaderModel)
        );
        addSlangDefine(sm.c_str(), "1");

        sessionDesc.preprocessorMacros = slangDefines.data();
        sessionDesc.preprocessorMacroCount = (SlangInt)slangDefines.size();

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        // TODO: Use our filesystem.

        // Setup additional compiler options.
        std::vector<slang::CompilerOptionEntry> compilerOptionEntries;
        auto addIntOption = [&compilerOptionEntries](slang::CompilerOptionName name, int value) {
            compilerOptionEntries.push_back({name, {slang::CompilerOptionValueKind::Int, 1, value, nullptr, nullptr}});
        };
        auto addStringOption = [&compilerOptionEntries](slang::CompilerOptionName name, char const* value) {
            compilerOptionEntries.push_back({name, {slang::CompilerOptionValueKind::String, 0, 0, value, nullptr}});
        };

        // We always use row-major matrix layout in Falcor so by default that's what we pass to Slang
        // to allow it to compute correct reflection information. Slang then invokes the downstream compiler.
        // Column major option can be useful when compiling external shader sources that don't depend
        // on anything Falcor.
        bool useColumnMajor = enum_has_any_flags(compilerFlags, SlangCompilerFlags::MatrixLayoutColumnMajor);
        addIntOption(useColumnMajor ? slang::CompilerOptionName::MatrixLayoutColumn : slang::CompilerOptionName::MatrixLayoutRow, 1);

        // New versions of slang default to short-circuiting for logical and/or operators.
        // Falcor is still written with the assumption that these operators do not short-circuit.
        // We want to transition to the new behavior, but for now we disable it.
        addIntOption(slang::CompilerOptionName::DisableShortCircuit, 1);

        // Disable noisy warnings enabled in newer slang versions.
        addStringOption(slang::CompilerOptionName::DisableWarning, "15602"); // #pragma once in modules
        addStringOption(slang::CompilerOptionName::DisableWarning, "30056"); // non-short-circuiting `?:` operator is deprecated, use 'select'
                                                                            // instead
        addStringOption(slang::CompilerOptionName::DisableWarning, "30081"); // implicit conversion
        addStringOption(slang::CompilerOptionName::DisableWarning, "41203"); // reinterpret<> into not equally sized types

        sessionDesc.compilerOptionEntries = compilerOptionEntries.data();
        sessionDesc.compilerOptionEntryCount = (uint32_t)compilerOptionEntries.size();

        Slang::ComPtr<slang::ISession> pSlangSession;
        pSlangGlobalSession->createSession(sessionDesc, pSlangSession.writeRef());
        AP_ASSERT(pSlangSession, "Failed to create Slang session");

        program.m_fileTimestamps.clear(); // TODO @skallweit

        SlangCompileRequest* pSlangRequest = nullptr;
        pSlangSession->createCompileRequest(&pSlangRequest);
        AP_ASSERT(pSlangRequest, "Failed to create Slang compile request");

        // Enable/disable intermediates dump
        bool dumpIR = enum_has_any_flags(program.m_description.compilerFlags, SlangCompilerFlags::DumpIntermediates);
        spSetDumpIntermediates(pSlangRequest, dumpIR);

        // Set debug level
        if (m_generateDebugInfo || enum_has_any_flags(program.m_description.compilerFlags, SlangCompilerFlags::GenerateDebugInfo))
        {
            spSetDebugInfoLevel(pSlangRequest, SLANG_DEBUG_INFO_LEVEL_STANDARD);
        }

        // Configure any flags for the Slang compilation step
        SlangCompileFlags slangFlags = 0;

        // When we invoke the Slang compiler front-end, skip code generation step
        // so that the compiler does not complain about missing arguments for
        // specialization parameters.
        //
        slangFlags |= SLANG_COMPILE_FLAG_NO_CODEGEN;

        spSetCompileFlags(pSlangRequest, slangFlags);

        // Set additional command line arguments.
        {
            std::vector<char const*> args;
            for (auto const& arg : m_globalCompilerArguments)
            {
                args.push_back(arg.c_str());
            }
            for (auto const& arg : program.m_description.compilerArguments)
            {
                args.push_back(arg.c_str());
            }
    #if APRIL_NVAPI_AVAILABLE
            // If NVAPI is available, we need to inform slang/dxc where to find it.
            std::string nvapiInclude = "-I" + (getRuntimeDirectory() / "shaders/nvapi").string();
            args.push_back("-Xdxc");
            args.push_back(nvapiInclude.c_str());
    #endif
            if (!args.empty())
            {
                spProcessCommandLineArguments(pSlangRequest, args.data(), (int)args.size());
            }
        }

        for (size_t moduleIndex = 0; moduleIndex < program.m_description.shaderModules.size(); ++moduleIndex)
        {
            auto const& module = program.m_description.shaderModules[moduleIndex];
            // If module name is empty, pass in nullptr to let Slang generate a name internally.
            char const* name = !module.name.empty() ? module.name.c_str() : nullptr;
            int translationUnitIndex = spAddTranslationUnit(pSlangRequest, SLANG_SOURCE_LANGUAGE_SLANG, name);
            AP_ASSERT(translationUnitIndex == (int)moduleIndex, "Translation unit index does not match module index");

            for (auto const& source : module.sources)
            {
                // Add source code to the translation unit
                if (source.type == ProgramDesc::ShaderSource::Type::File)
                {
                    // If this is not an HLSL or a SLANG file, display a warning
                    auto const& path = source.path;

                    if (!(path.extension() == ".hlsl" || path.extension() == ".slang"))
                    {
                        AP_WARN(
                            "Compiling a shader file which is not a SLANG file or an HLSL file. This is not an error, but make sure that the "
                            "file contains valid shaders"
                        );
                    }
                    auto fullPath = VFS::resolvePath(("shader" / path).string());
                    if (!std::filesystem::exists(fullPath))
                    {
                        spDestroyCompileRequest(pSlangRequest);
                        AP_CRITICAL("Can't find shader file {}", path.string());
                    }
                    spAddTranslationUnitSourceFile(pSlangRequest, translationUnitIndex, fullPath.string().c_str());
                }
                else
                {
                    AP_ASSERT(source.type == ProgramDesc::ShaderSource::Type::String, "Invalid shader source type");
                    spAddTranslationUnitSourceString(
                        pSlangRequest, translationUnitIndex, source.path.empty() ? "empty" : source.path.string().c_str(), source.string.c_str()
                    );
                }
            }
        }

        // Now we make a separate pass and add the entry points.
        // Each entry point references the index of the source
        // it uses, and luckily, the Slang API can use these
        // indices directly.
        for (auto const& entryPointGroup : program.m_description.entryPointGroups)
        {
            for (auto const& entryPoint : entryPointGroup.entryPoints)
            {
                spAddEntryPoint(pSlangRequest, entryPointGroup.shaderModuleIndex, entryPoint.name.c_str(), getSlangStage(entryPoint.type));
            }
        }

        return pSlangRequest;
    }
}
