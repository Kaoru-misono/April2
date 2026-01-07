#pragma once

#include <core/log/logger.hpp>

#include <slang.h>
#include <cstdlib>

namespace april
{
    template <typename T>
    inline auto checkValue(T value, char const* msg, ISlangBlob* diag = nullptr) -> void
    {
        if (!value)
        {
            AP_CRITICAL("{}", msg);

            if (diag)
            {
                AP_CRITICAL("[Diagnostics]\n{}", static_cast<char const*>(diag->getBufferPointer()));
            }

            std::exit(1);
        }
    }

    // Helper to check RHI results.
    inline auto checkResult(SlangResult res, char const* msg, ISlangBlob* diag = nullptr) -> void
    {
        if (SLANG_FAILED(res))
        {
            AP_CRITICAL("{} (Error: {:#x})", msg, static_cast<uint32_t>(res));

            if (diag)
            {
                AP_CRITICAL("[Diagnostics]\n{}", static_cast<char const*>(diag->getBufferPointer()));
            }

            std::exit(1);
        }
    }

    inline auto diagnoseIfNeeded(ISlangBlob* diagnosticsBlob) -> void
    {
        if (diagnosticsBlob != nullptr)
        {
            AP_ERROR("{}", std::string((char const*)diagnosticsBlob->getBufferPointer()));
        }
    }
}
