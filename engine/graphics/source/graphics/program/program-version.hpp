// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program-reflection.hpp"
#include "define-list.hpp"
#include "rhi/fwd.hpp"
#include "rhi/types.hpp"

#include <core/foundation/object.hpp>
#include <core/log/logger.hpp>
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-rhi.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace april::graphics
{
    class Program;
    class ProgramVariables;
    class ProgramVersion;

    /**
     * Represents a single program entry point and its associated kernel code.
     *
     * In GFX, we do not generate actual shader code at program creation.
     * The actual shader code will only be generated and cached when all specialization arguments
     * are known, which is right before a draw/dispatch command is issued, and this is done
     * internally within GFX.
     * The `EntryPointKernel` implementation here serves as a helper utility for application code that
     * uses raw graphics API to get shader kernel code from an ordinary slang source.
     * Since most users/render-passes do not need to get shader kernel code, we defer
     * the call to slang's `getEntryPointCode` function until it is actually needed.
     * to avoid redundant shader compiler invocation.
     */
    class EntryPointKernel : public core::Object
    {
        APRIL_OBJECT(EntryPointKernel)
    public:
        struct BlobData
        {
            void const* data{nullptr};
            size_t size{0};
        };

        /**
        * Create a shader object
        * @param[in] linkedSlangEntryPoint The Slang IComponentType that defines the shader entry point.
        * @param[in] type The Type of the shader
        * @return If success, a new shader object, otherwise nullptr
        */
        static auto create(rhi::ComPtr<slang::IComponentType> linkedSlangEntryPoint, ShaderType type, std::string const& entryPointName) -> core::ref<EntryPointKernel>
        {
            return core::ref<EntryPointKernel>(new EntryPointKernel(linkedSlangEntryPoint, type, entryPointName));
        }

        auto getType() const -> ShaderType { return m_type; }
        auto getEntryPointName() const -> std::string const& { return m_entryPointName; }
        auto getBlobData() const -> BlobData
        {
            if (!mp_blob)
            {
                rhi::ComPtr<ISlangBlob> pDiagnostics;
                if (SLANG_FAILED(mp_linkedSlangEntryPoint->getEntryPointCode(0, 0, mp_blob.writeRef(), pDiagnostics.writeRef())))
                {
                    AP_CRITICAL("Shader compilation failed. \n {}", (const char*)pDiagnostics->getBufferPointer());
                }
            }

            auto result = BlobData{};
            result.data = mp_blob->getBufferPointer();
            result.size = mp_blob->getBufferSize();

            return result;
        }

    protected:
        EntryPointKernel(rhi::ComPtr<slang::IComponentType> linkedSlangEntryPoint, ShaderType type, std::string const& entryPointName)
            : mp_linkedSlangEntryPoint(linkedSlangEntryPoint), m_type(type), m_entryPointName(entryPointName)
        {}

        rhi::ComPtr<slang::IComponentType> mp_linkedSlangEntryPoint{};
        ShaderType m_type{};
        std::string m_entryPointName{};
        mutable rhi::ComPtr<ISlangBlob> mp_blob{};
    };

    /**
     * A collection of one or more entry points in a program kernels object.
     */
    class EntryPointGroupKernels : public core::Object
    {
        APRIL_OBJECT(EntryPointGroupKernels)
    public:
        enum class Type
        {
            Compute,
            Rasterization,
            RayTracingSingleShader,
            RayTracingHitGroup,
        };

        static auto create(Type type, std::vector<core::ref<EntryPointKernel>> const& kernels, std::string const& exportName) -> core::ref<EntryPointGroupKernels const>;

        virtual ~EntryPointGroupKernels() = default;

        auto getType() const -> Type { return m_type; }
        auto getKernel(ShaderType type) const -> EntryPointKernel const*;
        auto getKernelByIndex(size_t index) const -> EntryPointKernel const* { return m_kernels[index].get(); }
        auto getExportName() const -> std::string const& { return m_exportName; }

    protected:
        EntryPointGroupKernels(Type type, std::vector<core::ref<EntryPointKernel>> const& kernels, std::string const& exportName);
        EntryPointGroupKernels() = default;
        EntryPointGroupKernels(const EntryPointGroupKernels&) = delete;
        EntryPointGroupKernels& operator=(const EntryPointGroupKernels&) = delete;

        Type m_type{};
        std::vector<core::ref<EntryPointKernel>> m_kernels{};
        std::string m_exportName{};
    };

    /**
     * Low-level program object
     * This class abstracts the API's program creation and management
     */
    class ProgramKernels : public core::Object
    {
        APRIL_OBJECT(ProgramKernels)
    public:
        using UniqueEntryPointGroups = std::vector<core::ref<EntryPointGroupKernels const>>;

        static auto create(
            Device* pDevice,
            ProgramVersion const* pVersion,
            slang::IComponentType* pSpecializedSlangGlobalScope,
            std::vector<slang::IComponentType*> const& typeConformanceSpecializedEntryPoints,
            core::ref<ProgramReflection const> pReflector,
            UniqueEntryPointGroups const& uniqueEntryPointGroups,
            std::string& log,
            std::string const& name = ""
        ) -> core::ref<ProgramKernels>;

        virtual ~ProgramKernels() = default;

        auto getKernel(int32_t type) const -> EntryPointKernel const*;
        auto getName() const -> std::string const& { return m_name; }
        auto getReflector() const -> core::ref<ProgramReflection const> { return m_reflector; }
        auto getProgramVersion() const -> ProgramVersion const* { return m_version; }
        auto getUniqueEntryPointGroups() const -> UniqueEntryPointGroups const& { return m_uniqueEntryPointGroups; }
        auto getUniqueEntryPointGroup(uint32_t index) const -> core::ref<EntryPointGroupKernels const> { return m_uniqueEntryPointGroups[index]; }

        auto getGfxShaderProgram() const -> rhi::IShaderProgram* { return m_gfxShaderProgram; }

    protected:
        ProgramKernels(
            ProgramVersion const* pVersion,
            core::ref<ProgramReflection const> pReflector,
            UniqueEntryPointGroups const& uniqueEntryPointGroups,
            std::string const& name = ""
        );

        rhi::ComPtr<rhi::IShaderProgram> m_gfxShaderProgram{};
        std::string const m_name{};

        UniqueEntryPointGroups m_uniqueEntryPointGroups{};

        void* m_privateData{};
        core::ref<ProgramReflection const> const m_reflector{};

        ProgramVersion const* m_version{nullptr};
    };

    class ProgramVersion : public core::Object
    {
        APRIL_OBJECT(ProgramVersion)
    public:
        auto getProgram() const -> Program* { return m_program; }
        auto getDefines() const -> DefineList const& { return m_defines; }
        auto getName() const -> std::string const& { return m_name; }

        auto getReflector() const -> core::ref<ProgramReflection const>
        {
            // assert(reflector);
            return m_reflector;
        }

        auto getKernels(Device* pDevice, ProgramVariables const* pVariables) const -> core::ref<ProgramKernels const>;

        auto getSlangSession() const -> slang::ISession*;
        auto getSlangGlobalScope() const -> slang::IComponentType*;
        auto getSlangEntryPoint(uint32_t index) const -> slang::IComponentType*;
        auto getSlangEntryPoints() const -> std::vector<rhi::ComPtr<slang::IComponentType>> const& { return m_slangEntryPoints; }

    protected:
        friend class Program;
        friend class ProgramManager;

        static auto createEmpty(Program* pProgram, slang::IComponentType* pSlangGlobalScope) -> core::ref<ProgramVersion>;

        ProgramVersion(Program* pProgram, slang::IComponentType* pSlangGlobalScope);

        void init(
            DefineList const& defineList,
            core::ref<ProgramReflection const> pReflector,
            std::string const& name,
            std::vector<rhi::ComPtr<slang::IComponentType>> const& slangEntryPoints
        );

        Program* m_program{nullptr};
        DefineList m_defines{};
        core::ref<ProgramReflection const> m_reflector{};
        std::string m_name{};
        rhi::ComPtr<slang::IComponentType> m_slangGlobalScope{};
        std::vector<rhi::ComPtr<slang::IComponentType>> m_slangEntryPoints{};

        mutable std::unordered_map<std::string, core::ref<ProgramKernels const>> m_kernels{};
    };
}
