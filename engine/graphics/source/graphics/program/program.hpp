// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program-version.hpp"
#include "define-list.hpp"
#include "rhi/types.hpp"
#include "rhi/ray-tracing.hpp"
#include "state/graph-state.hpp"

#include <core/tools/enum-flags.hpp>
#include <core/foundation/object.hpp>
#include <core/tools/hash.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <map>

namespace april::graphics
{
    class RayTracingPipeline;
    class RtProgramVariables;

    /**
     * Representing a shader implementation of an interface.
     * When linked into a `ProgramVersion`, the specialized shader will contain
     * the implementation of the specified type in a dynamic dispatch function.
     */
    struct TypeConformance
    {
        std::string typeName{};
        std::string interfaceName{};

        TypeConformance() = default;
        TypeConformance(std::string const& typeName, std::string const& interfaceName)
            : typeName(typeName), interfaceName(interfaceName) {}

        auto operator<(TypeConformance const& other) const -> bool
        {
            return typeName < other.typeName || (typeName == other.typeName && interfaceName < other.interfaceName);
        }

        auto operator==(TypeConformance const& other) const -> bool
        {
            return typeName == other.typeName && interfaceName == other.interfaceName;
        }

        struct HashFunction
        {
            auto operator()(TypeConformance const& conformance) const -> size_t
            {
                return core::hash(conformance.typeName, conformance.interfaceName);
            }
        };
    };

    class TypeConformanceList : public std::map<TypeConformance, uint32_t>
    {
    public:
        TypeConformanceList() = default;
        TypeConformanceList(std::initializer_list<std::pair<const TypeConformance, uint32_t>> initList)
            : std::map<TypeConformance, uint32_t>(initList) {}

        /**
         * Adds a type conformance. If the type conformance exists, it will be replaced.
         * @param[in] typeName The name of the implementation type.
         * @param[in] interfaceName The name of the interface type.
         * @param[in] id Optional. The id representing the implementation type for this interface. If it is -1, Slang will automatically
         * assign a unique Id for the type.
         * @return The updated list of type conformances.
         */
        auto add(std::string const& typeName, std::string const& interfaceName, uint32_t id = -1) -> TypeConformanceList&
        {
            (*this)[TypeConformance(typeName, interfaceName)] = id;
            return *this;
        }

        /**
         * Removes a type conformance. If the type conformance doesn't exist, the call will be silently ignored.
         * @param[in] typeName The name of the implementation type.
         * @param[in] interfaceName The name of the interface type.
         * @return The updated list of type conformances.
         */
        auto remove(std::string const& typeName, std::string const& interfaceName) -> TypeConformanceList&
        {
            (*this).erase(TypeConformance(typeName, interfaceName));
            return *this;
        }

        /**
         * Add a type conformance list to the current list
         */
        auto add(TypeConformanceList const& other) -> TypeConformanceList&
        {
            for (auto const& pair : other)
            {
                add(pair.first.typeName, pair.first.interfaceName, pair.second);
            }
            return *this;
        }

        /**
         * Remove a type conformance list from the current list
         */
        auto remove(TypeConformanceList const& other) -> TypeConformanceList&
        {
            for (auto const& pair : other)
            {
                remove(pair.first.typeName, pair.first.interfaceName);
            }
            return *this;
        }
    };

    enum class SlangCompilerFlags : uint32_t
    {
        None = 0x0,
        TreatWarningsAsErrors = 0x1,
        DumpIntermediates = 0x2,
        FloatingPointModeFast = 0x4,
        FloatingPointModePrecise = 0x8,
        GenerateDebugInfo = 0x10,
        MatrixLayoutColumnMajor = 0x20,
    };
    AP_ENUM_CLASS_OPERATORS(SlangCompilerFlags);

    struct ProgramDesc
    {
        struct ShaderID
        {
            int32_t groupIndex{-1};
            auto isValid() const -> bool { return groupIndex >= 0; }
        };

        struct ShaderSource
        {
            enum class Type
            {
                File,
                String
            };

            Type type{Type::File};
            std::filesystem::path path{};
            std::string string{};

            auto operator==(ShaderSource const& rhs) const -> bool { return type == rhs.type && path == rhs.path && string == rhs.string; }
        };

        struct ShaderModule
        {
            std::string name{};
            std::vector<ShaderSource> sources{};

            ShaderModule() = default;
            explicit ShaderModule(std::string name) : name(std::move(name)) {}

            static auto fromFile(std::filesystem::path path) -> ShaderModule
            {
                ShaderModule module{};
                module.addFile(std::move(path));
                return module;
            }

            static auto fromString(std::string string, std::filesystem::path path = {}, std::string moduleName = {}) -> ShaderModule
            {
                ShaderModule module(std::move(moduleName));
                module.addString(std::move(string), std::move(path));
                return module;
            }

            auto addFile(std::filesystem::path path) -> ShaderModule&
            {
                sources.push_back({ShaderSource::Type::File, std::move(path), {}});
                return *this;
            }

            auto addString(std::string string, std::filesystem::path path = {}) -> ShaderModule&
            {
                sources.push_back({ShaderSource::Type::String, std::move(path), std::move(string)});
                return *this;
            }

            auto operator==(ShaderModule const& rhs) const -> bool { return name == rhs.name && sources == rhs.sources; }
        };

        struct EntryPoint
        {
            ShaderType type{};
            std::string name{};
            std::string exportName{};
            uint32_t globalIndex{0};
        };

        struct EntryPointGroup
        {
            uint32_t shaderModuleIndex{};
            TypeConformanceList typeConformances{};
            std::vector<EntryPoint> entryPoints{};

            auto setTypeConformances(TypeConformanceList conformances) -> EntryPointGroup&
            {
                typeConformances = std::move(conformances);
                return *this;
            }

            auto addTypeConformances(TypeConformanceList const& conformances) -> EntryPointGroup&
            {
                typeConformances.add(conformances);
                return *this;
            }

            auto addEntryPoint(ShaderType type, std::string_view name, std::string_view exportName = "") -> EntryPointGroup&
            {
                if (exportName.empty()) exportName = name;
                entryPoints.push_back({type, std::string(name), std::string(exportName)});
                return *this;
            }
        };

        using ShaderModuleList = std::vector<ShaderModule>;

        ShaderModuleList shaderModules{};
        std::vector<EntryPointGroup> entryPointGroups{};
        TypeConformanceList typeConformances{};
        ShaderModel shaderModel{ShaderModel::Unknown};
        SlangCompilerFlags compilerFlags{SlangCompilerFlags::None};
        std::vector<std::string> compilerArguments{};

        // RayTracing specific
        uint32_t maxTraceRecursionDepth{uint32_t(-1)};
        uint32_t maxPayloadSize{uint32_t(-1)};
        uint32_t maxAttributeSize = getRaytracingMaxAttributeSize(); // Default value
        RtPipelineFlags rtPipelineFlags = RtPipelineFlags::None;
        bool useSPIRVBackend{false};

        auto addShaderModule(std::string name = {}) -> ShaderModule&
        {
            shaderModules.push_back(ShaderModule(std::move(name)));
            return shaderModules.back();
        }

        auto addShaderModule(ShaderModule module) -> ProgramDesc&
        {
            shaderModules.push_back(std::move(module));
            return *this;
        }

        auto addShaderModules(ShaderModuleList const& modules) -> ProgramDesc&
        {
            shaderModules.append_range(modules);
            return *this;
        }

        auto addShaderLibrary(std::filesystem::path path) -> ProgramDesc&
        {
            addShaderModule().addFile(std::move(path));
            return *this;
        }

        auto addEntryPointGroup(uint32_t shaderModuleIndex = uint32_t(-1)) -> EntryPointGroup&
        {
            // Error handling omitted for header generation
            if (shaderModuleIndex == uint32_t(-1)) shaderModuleIndex = uint32_t(shaderModules.size()) - 1;
            entryPointGroups.push_back({shaderModuleIndex});
            return entryPointGroups.back();
        }

        auto addEntryPoint(ShaderType type, std::string_view name) -> ProgramDesc&
        {
             if (entryPointGroups.empty() || entryPointGroups.back().shaderModuleIndex != shaderModules.size() - 1)
                addEntryPointGroup();
            entryPointGroups.back().addEntryPoint(type, name);
            return *this;
        }

        // Renamed helpers
        auto vsEntryPoint(std::string const& name) -> ProgramDesc& { return addEntryPoint(ShaderType::Vertex, name); }
        auto psEntryPoint(std::string const& name) -> ProgramDesc& { return addEntryPoint(ShaderType::Pixel, name); }
        auto csEntryPoint(std::string const& name) -> ProgramDesc& { return addEntryPoint(ShaderType::Compute, name); }
        auto gsEntryPoint(std::string const& name) -> ProgramDesc& { return addEntryPoint(ShaderType::Geometry, name); }
        auto hsEntryPoint(std::string const& name) -> ProgramDesc& { return addEntryPoint(ShaderType::Hull, name); }
        auto dsEntryPoint(std::string const& name) -> ProgramDesc& { return addEntryPoint(ShaderType::Domain, name); }

        auto hasEntryPoint(ShaderType type, std::string_view name) const -> bool
        {
            for (auto const& group : entryPointGroups)
            {
                for (auto const& entryPoint : group.entryPoints)
                {
                    if (entryPoint.type == type && entryPoint.name == name) return true;
                }
            }

            return false;
        }

        /// Add global type conformances.
        auto addTypeConformances(TypeConformanceList const& conformances) -> ProgramDesc&
        {
            typeConformances.add(conformances);

            return *this;
        }

        /// Set the shader model.
        auto setShaderModel(ShaderModel shaderModel_) -> ProgramDesc&
        {
            shaderModel = shaderModel_;

            return *this;
        }

        /// Set the compiler flags.
        auto setCompilerFlags(SlangCompilerFlags flags) -> ProgramDesc&
        {
            compilerFlags = flags;

            return *this;
        }

        /// Set the compiler arguments (as set on the compiler command line).
        auto setCompilerArguments(std::vector<std::string> args) -> ProgramDesc&
        {
            compilerArguments = std::move(args);

            return *this;
        }

        /// Add compiler arguments (as set on the compiler command line).
        auto addCompilerArguments(std::vector<std::string> const& args) -> ProgramDesc&
        {
            compilerArguments.insert(compilerArguments.end(), args.begin(), args.end());

            return *this;
        }

        // RayTracing Compatibility Helpers
        auto addRayGen(std::string const& raygen, TypeConformanceList const& conformances = {}, std::string const& entryPointNameSuffix = "") -> ShaderID
        {
            // AP_CRITICAL(!raygen.empty(), "Raygen name cannot be empty");

            auto& group = addEntryPointGroup();
            group.setTypeConformances(conformances);
            group.addEntryPoint(ShaderType::RayGeneration, raygen, raygen + entryPointNameSuffix);

            return ShaderID{int32_t(entryPointGroups.size() - 1)};
        }

        auto addMiss(std::string const& miss, TypeConformanceList const& conformances = {}, std::string const& entryPointNameSuffix = "") -> ShaderID
        {
            // AP_CRITICAL(!miss.empty(), "Miss name cannot be empty");

            auto& group = addEntryPointGroup();
            group.setTypeConformances(conformances);
            group.addEntryPoint(ShaderType::Miss, miss, miss + entryPointNameSuffix);

            return ShaderID{int32_t(entryPointGroups.size() - 1)};
        }

        auto addHitGroup(std::string const& closestHit, std::string const& anyHit = "", std::string const& intersection = "", TypeConformanceList const& conformances = {}, std::string const& entryPointNameSuffix = "") -> ShaderID
        {
            // AP_CRITICAL(!closestHit.empty() || !anyHit.empty() || !intersection.empty(), "Hit group must have at least one entry point");

            auto& group = addEntryPointGroup();
            group.setTypeConformances(conformances);
            if (!closestHit.empty()) group.addEntryPoint(ShaderType::ClosestHit, closestHit, closestHit + entryPointNameSuffix);
            if (!anyHit.empty()) group.addEntryPoint(ShaderType::AnyHit, anyHit, anyHit + entryPointNameSuffix);
            if (!intersection.empty()) group.addEntryPoint(ShaderType::Intersection, intersection, intersection + entryPointNameSuffix);

            return ShaderID{int32_t(entryPointGroups.size() - 1)};
        }

        auto setMaxTraceRecursionDepth(uint32_t maxDepth) -> ProgramDesc&
        {
            maxTraceRecursionDepth = maxDepth;

            return *this;
        }

        auto setMaxPayloadSize(uint32_t size) -> ProgramDesc&
        {
            maxPayloadSize = size;

            return *this;
        }

        auto setMaxAttributeSize(uint32_t size) -> ProgramDesc&
        {
            maxAttributeSize = size;
            return *this;
        }

        auto setRtPipelineFlags(RtPipelineFlags flags) -> ProgramDesc&
        {
            rtPipelineFlags = flags;

            return *this;
        }

        auto setUseSPIRVBackend(bool b = true) ->void
        {
            useSPIRVBackend = b;
        }

        auto finalize() -> void;
    };

    class Program : public core::Object
    {
        APRIL_OBJECT(Program)
    public:
        Program(core::ref<Device> pDevice, ProgramDesc description, DefineList programDefines);
        virtual ~Program() override;

        static auto create(core::ref<Device> pDevice, ProgramDesc description, DefineList programDefines = {}) -> core::ref<Program>
        {
            return core::make_ref<Program>(std::move(pDevice), std::move(description), std::move(programDefines));
        }

        static auto createCompute(
            core::ref<Device> pDevice,
            std::string const& path,
            std::string const& computeShaderEntryPoint,
            DefineList programDefines = {},
            SlangCompilerFlags flags = SlangCompilerFlags::None,
            ShaderModel shaderModel = ShaderModel::Unknown
        ) -> core::ref<Program>
        {
            auto desc = ProgramDesc();
            desc.addShaderLibrary(path);
            if (shaderModel != ShaderModel::Unknown) desc.setShaderModel(shaderModel);
            desc.setCompilerFlags(flags);
            desc.csEntryPoint(computeShaderEntryPoint);

            return create(std::move(pDevice), std::move(desc), std::move(programDefines));
        }

        static auto createGraphics(
            core::ref<Device> pDevice,
            std::string const& path,
            std::string const& vertexShaderEntryPoint,
            std::string const& pixelShaderEntryPoint,
            DefineList programDefines = {},
            SlangCompilerFlags flags = SlangCompilerFlags::None,
            ShaderModel shaderModel = ShaderModel::Unknown
        ) -> core::ref<Program>
        {
            auto desc = ProgramDesc();
            desc.addShaderLibrary(path);
            if (shaderModel != ShaderModel::Unknown) desc.setShaderModel(shaderModel);
            desc.setCompilerFlags(flags);
            desc.vsEntryPoint(vertexShaderEntryPoint);
            desc.psEntryPoint(pixelShaderEntryPoint);

            return create(std::move(pDevice), std::move(desc), std::move(programDefines));
        }

        auto getActiveVersion() const -> core::ref<ProgramVersion const>;

        auto addDefine(std::string const& name, std::string const& value = "") -> bool;
        auto addDefines(DefineList const& defineList) -> bool;
        auto removeDefine(std::string const& name) -> bool;
        auto removeDefines(DefineList const& defineList) -> bool;
        auto removeDefines(size_t pos, size_t len, std::string const& str) -> bool;

        auto setDefines(DefineList const& defineList) -> bool;
        auto getDefines() const -> DefineList const& { return m_defineList; }

        auto addTypeConformance(std::string const& typeName, std::string const interfaceType, uint32_t id) -> bool;
        auto removeTypeConformance(std::string const& typeName, std::string const interfaceType) -> bool;
        auto setTypeConformances(TypeConformanceList const& conformances) -> bool;
        auto getTypeConformances() const -> TypeConformanceList const& { return m_typeConformanceList; }

        auto getDefineList() const -> DefineList const& { return m_defineList; }
        auto getDescription() const -> ProgramDesc const& { return m_description; }

        // Inline dependency on ProgramVersion/ProgramReflection
        // Moved out of line or requires ProgramVersion to be complete.
        // auto getReflector() const -> core::ref<ProgramReflection const> { return getActiveVersion()->getReflector(); }
        auto getReflector() const -> core::ref<ProgramReflection const> { return getActiveVersion()->getReflector(); }

        // Missing API
        auto getEntryPointGroupCount() const -> uint32_t { return (uint32_t)m_description.entryPointGroups.size(); }
        auto getGroupEntryPointCount(uint32_t groupIndex) const -> uint32_t { return (uint32_t)m_description.entryPointGroups[groupIndex].entryPoints.size(); }
        auto getGroupEntryPointIndex(uint32_t groupIndex, uint32_t entryPointIndexInGroup) const -> uint32_t
        {
            return m_description.entryPointGroups[groupIndex].entryPoints[entryPointIndexInGroup].globalIndex;
        }

        void breakStrongReferenceToDevice();
        auto getRtso(RtProgramVariables* pVars) -> core::ref<RayTracingPipeline>;

    protected:
        friend class ProgramManager;
        friend class ProgramVersion;
        friend class ParameterBlockReflection;

        auto validateEntryPoints() const -> bool;
        auto link() const -> bool;

        core::BreakableReference<Device> mp_device;

        ProgramDesc m_description{};
        DefineList m_defineList{};
        TypeConformanceList m_typeConformanceList{};

        struct ProgramVersionKey
        {
            DefineList defineList{};
            TypeConformanceList typeConformanceList{};

            auto operator<(ProgramVersionKey const& rhs) const -> bool
            {
                return std::tie(defineList, typeConformanceList) < std::tie(rhs.defineList, rhs.typeConformanceList);
            }
        };

        mutable bool m_linkRequired{true};
        mutable std::map<ProgramVersionKey, core::ref<ProgramVersion const>> m_programVersions{};
        mutable core::ref<ProgramVersion const> m_activeVersion{};

        auto markDirty() -> void { m_linkRequired = true; }

        auto getProgramDescString() const -> std::string;

        using string_time_map = std::unordered_map<std::string, time_t>;
        mutable string_time_map m_fileTimestamps{};

        auto checkIfFilesChanged() -> bool;
        auto reset() -> void;

        using StateGraph = april::graphics::StateGraph<core::ref<RayTracingPipeline>, void*>;
        StateGraph m_rtStateGraph{};
    };
}
