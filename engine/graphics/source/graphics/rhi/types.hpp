// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "shader-resource-type.hpp"

#include <cstdint>
#include <core/tools/enum-flags.hpp>
#include <core/tools/enum.hpp>

namespace april::graphics
{
    enum class ShaderModel : uint32_t
    {
        Unknown = 0,
        SM6_0 = 60,
        SM6_1 = 61,
        SM6_2 = 62,
        SM6_3 = 63,
        SM6_4 = 64,
        SM6_5 = 65,
        SM6_6 = 66,
        SM6_7 = 67,
    };
    AP_ENUM_INFO(
        ShaderModel,
        {
            {ShaderModel::Unknown, "Unknown"},
            {ShaderModel::SM6_0, "SM6_0"},
            {ShaderModel::SM6_1, "SM6_1"},
            {ShaderModel::SM6_2, "SM6_2"},
            {ShaderModel::SM6_3, "SM6_3"},
            {ShaderModel::SM6_4, "SM6_4"},
            {ShaderModel::SM6_5, "SM6_5"},
            {ShaderModel::SM6_6, "SM6_6"},
            {ShaderModel::SM6_7, "SM6_7"},
        }
    );
    AP_ENUM_REGISTER(ShaderModel);

    inline auto getShaderModelMajorVersion(ShaderModel sm) -> uint32_t
    {
        return uint32_t(sm) / 10;
    }
    inline auto getShaderModelMinorVersion(ShaderModel sm) -> uint32_t
    {
        return uint32_t(sm) % 10;
    }

    enum class ShaderType
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute,
        RayGeneration,
        Intersection,
        AnyHit,
        ClosestHit,
        Miss,
        Callable,
        Count
    };
    AP_ENUM_INFO(
        ShaderType,
        {
            {ShaderType::Vertex, "Vertex"},
            {ShaderType::Pixel, "Pixel"},
            {ShaderType::Geometry, "Geometry"},
            {ShaderType::Hull, "Hull"},
            {ShaderType::Domain, "Domain"},
            {ShaderType::Compute, "Compute"},
            {ShaderType::RayGeneration, "RayGeneration"},
            {ShaderType::Intersection, "Intersection"},
            {ShaderType::AnyHit, "AnyHit"},
            {ShaderType::ClosestHit, "ClosestHit"},
            {ShaderType::Miss, "Miss"},
            {ShaderType::Callable, "Callable"},
        }
    );
    AP_ENUM_REGISTER(ShaderType);

    enum class DataType
    {
        int8,
        int16,
        int32,
        int64,
        uint8,
        uint16,
        uint32,
        uint64,
        float16,
        float32,
        float64,
    };
    AP_ENUM_INFO(
        DataType,
        {
            {DataType::int8, "int8"},
            {DataType::int16, "int16"},
            {DataType::int32, "int32"},
            {DataType::int64, "int64"},
            {DataType::uint8, "uint8"},
            {DataType::uint16, "uint16"},
            {DataType::uint32, "uint32"},
            {DataType::uint64, "uint64"},
            {DataType::float16, "float16"},
            {DataType::float32, "float32"},
            {DataType::float64, "float64"},
        }
    );
    AP_ENUM_REGISTER(DataType);

    enum class ComparisonFunc
    {
        Disabled,
        Never,
        Always,
        Less,
        Equal,
        NotEqual,
        LessEqual,
        Greater,
        GreaterEqual,
    };
    AP_ENUM_INFO(
        ComparisonFunc,
        {
            {ComparisonFunc::Disabled, "Disabled"},
            {ComparisonFunc::Never, "Never"},
            {ComparisonFunc::Always, "Always"},
            {ComparisonFunc::Less, "Less"},
            {ComparisonFunc::Equal, "Equal"},
            {ComparisonFunc::NotEqual, "NotEqual"},
            {ComparisonFunc::LessEqual, "LessEqual"},
            {ComparisonFunc::Greater, "Greater"},
            {ComparisonFunc::GreaterEqual, "GreaterEqual"},
        }
    );
    AP_ENUM_REGISTER(ComparisonFunc);
}
