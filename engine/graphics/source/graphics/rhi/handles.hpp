// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <slang.h>
#include <slang-gfx.h>
#include <slang-com-ptr.h>
#include <cstdint>

namespace april::graphics
{
    using GpuAddress = uint64_t;

#if defined(_WIN32)
    using SharedResourceApiHandle = void*; // HANDLE
    using SharedFenceApiHandle = void*;    // HANDLE
#elif defined(__linux__)
    using SharedResourceApiHandle = void*;
    using SharedFenceApiHandle = void*;
#else
    using SharedResourceApiHandle = void*;
    using SharedFenceApiHandle = void*;
#endif
} // namespace april::graphics
