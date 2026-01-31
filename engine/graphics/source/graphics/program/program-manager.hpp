// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program.hpp"
#include "program-version.hpp"
#include "program-reflection.hpp"
#include "define-list.hpp"
#include "rhi/fwd.hpp"

#include <slang.h>
#include <vector>
#include <string>

namespace april::graphics
{
    class Program;
    class ProgramVariables;

    /**
     * Manager for shader programs.
     * Handles global compilation settings, flags, and reloading.
     */
    class ProgramManager
    {
    public:
        ProgramManager(Device* pDevice);

        /**
         * Defines flags that should be forcefully disabled or enabled on all shaders.
         * When a flag is in both groups, it gets enabled.
         */
        struct ForcedCompilerFlags
        {
            SlangCompilerFlags enabled = SlangCompilerFlags::None;  ///< Compiler flags forcefully enabled on all shaders
            SlangCompilerFlags disabled = SlangCompilerFlags::None; ///< Compiler flags forcefully disabled on all shaders
        };

        struct CompilationStats
        {
            size_t programVersionCount{};
            size_t programKernelsCount{};
            double programVersionMaxTime{};
            double programKernelsMaxTime{};
            double programVersionTotalTime{};
            double programKernelsTotalTime{};
        };

        auto applyForcedCompilerFlags(ProgramDesc desc) const -> ProgramDesc;
        auto registerProgramForReload(Program* program) -> void;
        auto unregisterProgramForReload(Program* program) -> void;

        auto createProgramVersion(Program const& program, std::string& log) const -> core::ref<ProgramVersion const>;

        auto createProgramKernels(
            Program const& program,
            ProgramVersion const& programVersion,
            ProgramVariables const* programVars,
            std::string& log
        ) const -> core::ref<ProgramKernels const>;

        auto createEntryPointGroupKernels(
            std::vector<core::ref<EntryPointKernel>> const& kernels,
            core::ref<EntryPointBaseReflection> const& reflector
        ) const -> core::ref<EntryPointGroupKernels const>;

        /// Get the global HLSL language prelude.
        auto getHlslLanguagePrelude() const -> std::string;

        /// Set the global HLSL language prelude.
        auto setHlslLanguagePrelude(std::string const& prelude) -> void;

        /**
         * Reload and relink all programs.
         * @param[in] forceReload Force reloading all programs.
         * @return True if any program was reloaded, false otherwise.
         */
        auto reloadAllPrograms(bool forceReload = false) -> bool;

        /**
         * Add a list of defines applied to all programs.
         * @param[in] defineList List of macro definitions.
         */
        auto addGlobalDefines(DefineList const& defineList) -> void;

        /**
         * Remove a list of defines applied to all programs.
         * @param[in] defineList List of macro definitions.
         */
        auto removeGlobalDefines(DefineList const& defineList) -> void;

        /**
         * Set compiler arguments applied to all programs.
         * @param[in] args Compiler arguments.
         */
        auto setGlobalCompilerArguments(std::vector<std::string> const& args) -> void;

        /**
         * Get compiler arguments applied to all programs.
         * @return List of compiler arguments.
         */
        auto getGlobalCompilerArguments() const -> std::vector<std::string> const& { return m_globalCompilerArguments; }

        /**
         * Enable/disable global generation of shader debug info.
         * @param[in] enabled Enable/disable.
         */
        auto setGenerateDebugInfoEnabled(bool enabled) -> void;

        /**
         * Check if global generation of shader debug info is enabled.
         * @return Returns true if enabled.
         */
        auto isGenerateDebugInfoEnabled() -> bool;

        /**
         * Sets compiler flags that will always be forced on and forced off on each program.
         * If a flag is in both groups, it results in being forced on.
         * @param[in] forceOn Flags to be forced on.
         * @param[in] forceOff Flags to be forced off.
         */
        auto setForcedCompilerFlags(ForcedCompilerFlags forcedCompilerFlags) -> void;

        /**
         * Retrieve compiler flags that are always forced on all shaders.
         * @return The forced compiler flags.
         */
        auto getForcedCompilerFlags() -> ForcedCompilerFlags;

        auto getCompilationStats() -> CompilationStats const& { return m_compilationStats; }
        auto resetCompilationStats() -> void { m_compilationStats = {}; }

    private:
        auto createSlangCompileRequest(Program const& program) const -> SlangCompileRequest*;

        Device* mp_device{};

        std::vector<Program*> m_loadedPrograms{};
        mutable CompilationStats m_compilationStats{};

        DefineList m_globalDefineList{};
        std::vector<std::string> m_globalCompilerArguments{};
        bool m_generateDebugInfo{false};
        ForcedCompilerFlags m_forcedCompilerFlags{};

        mutable uint32_t m_hitGroupID{};
    };
}
