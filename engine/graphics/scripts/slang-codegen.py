#!/usr/bin/env python3
"""
Generate C++ and shader declarations from shared Slang schemas.
"""

from __future__ import annotations

import argparse
import os
import re
from dataclasses import dataclass
from pathlib import Path


SLANG_TO_CPP_TYPES = {
    "void": "void",
    "bool": "bool",
    "int": "int32_t",
    "uint": "uint32_t",
    "float": "float",
    "half": "float",
    "float16_t": "float16_t",
    "float16_t2": "float16_t2",
    "float16_t3": "float16_t3",
    "float16_t4": "float16_t4",
    "float2": "float2",
    "float3": "float3",
    "float4": "float4",
    "int2": "int2",
    "int3": "int3",
    "int4": "int4",
    "uint2": "uint2",
    "uint3": "uint3",
    "uint4": "uint4",
    "float2x2": "float2x2",
    "float3x3": "float3x3",
    "float4x4": "float4x4",
}

CAST_TYPE_MAP = {
    "bool": "bool",
    "int": "int32_t",
    "uint": "uint32_t",
    "float": "float",
    "uint16_t": "uint16_t",
    "int16_t": "int16_t",
}

ANNOTATION_PATTERN = re.compile(r"^[ \t]*//\s*@export-cpp(?::\s*([A-Za-z_]\w*))?\s*$", re.MULTILINE)
IMPORT_PATTERN = re.compile(r"^\s*(?:__exported\s+)?import\s+([A-Za-z0-9_.]+)\s*;", re.MULTILINE)
IMPORT_LINE_PATTERN = re.compile(r"^\s*(?:__exported\s+)?import\s+[A-Za-z0-9_.]+\s*;\s*$", re.MULTILINE)
MUTATING_MARKER = "__APRIL_MUTATING__"


@dataclass
class EnumDecl:
    name: str
    cpp_name: str
    backing_type: str | None
    body: str


@dataclass
class StructDecl:
    name: str
    cpp_name: str
    body: str


@dataclass
class ConstantDecl:
    name: str
    cpp_name: str
    type_name: str
    value: str


@dataclass
class ParsedFile:
    path: Path
    imports: list[str]
    import_lines: list[str]
    enums: list[EnumDecl]
    structs: list[StructDecl]
    constants: list[ConstantDecl]

    @property
    def has_exports(self) -> bool:
        return bool(self.enums or self.structs or self.constants)


def normalize_module_name(name: str) -> str:
    return name.replace("-", "_").lower()


def map_type_name(type_name: str) -> str:
    token = type_name.strip()
    return SLANG_TO_CPP_TYPES.get(token, token)


def skip_whitespace_and_comments(text: str, index: int) -> int:
    n = len(text)
    i = index
    while i < n:
        while i < n and text[i].isspace():
            i += 1
        if text.startswith("//", i):
            line_end = text.find("\n", i)
            if line_end == -1:
                return n
            i = line_end + 1
            continue
        if text.startswith("/*", i):
            close = text.find("*/", i + 2)
            if close == -1:
                return n
            i = close + 2
            continue
        break
    return i


def find_matching_brace(text: str, open_brace_index: int) -> int:
    depth = 0
    for i in range(open_brace_index, len(text)):
        char = text[i]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return i
    raise ValueError("Unmatched '{' in Slang source.")


def parse_imports(content: str) -> list[str]:
    imports: list[str] = []
    for match in IMPORT_PATTERN.finditer(content):
        module = match.group(1).split(".")[-1]
        imports.append(normalize_module_name(module))
    return imports


def parse_import_lines(content: str) -> list[str]:
    return [match.group(0).strip() for match in IMPORT_LINE_PATTERN.finditer(content)]


def parse_exported_declarations(content: str) -> tuple[list[EnumDecl], list[StructDecl], list[ConstantDecl]]:
    enums: list[EnumDecl] = []
    structs: list[StructDecl] = []
    constants: list[ConstantDecl] = []

    for annotation in ANNOTATION_PATTERN.finditer(content):
        alias = annotation.group(1)
        cursor = skip_whitespace_and_comments(content, annotation.end())
        remaining = content[cursor:]

        enum_header = re.match(
            r"enum\s+(?:class\s+)?([A-Za-z_]\w*)(?:\s*:\s*([A-Za-z_]\w*))?\s*\{",
            remaining,
            re.DOTALL,
        )
        if enum_header:
            name = enum_header.group(1)
            backing_type = enum_header.group(2)
            open_brace = cursor + enum_header.end() - 1
            close_brace = find_matching_brace(content, open_brace)
            body = content[open_brace + 1 : close_brace]
            enums.append(
                EnumDecl(
                    name=name,
                    cpp_name=alias or name,
                    backing_type=backing_type,
                    body=body,
                )
            )
            continue

        struct_header = re.match(
            r"struct\s+([A-Za-z_]\w+)(?:\s*:\s*[^{]+)?\s*\{",
            remaining,
            re.DOTALL,
        )
        if struct_header:
            name = struct_header.group(1)
            open_brace = cursor + struct_header.end() - 1
            close_brace = find_matching_brace(content, open_brace)
            body = content[open_brace + 1 : close_brace]
            structs.append(
                StructDecl(
                    name=name,
                    cpp_name=alias or name,
                    body=body,
                )
            )
            continue

        statement_end = content.find(";", cursor)
        if statement_end == -1:
            continue
        statement = content[cursor:statement_end].strip()
        const_match = re.match(
            r"(?:static\s+)?const\s+([A-Za-z_][\w:]*)\s+([A-Za-z_]\w*)\s*=\s*(.+)",
            statement,
            re.DOTALL,
        )
        if const_match:
            type_name = const_match.group(1).strip()
            name = const_match.group(2).strip()
            value = const_match.group(3).strip()
            constants.append(
                ConstantDecl(
                    name=name,
                    cpp_name=alias or name,
                    type_name=type_name,
                    value=value,
                )
            )

    return enums, structs, constants


def add_const_to_non_mutating_methods(code: str, struct_name: str | None) -> str:
    lines = code.split("\n")
    out_lines: list[str] = []
    brace_depth = 0
    next_method_is_mutating = False

    signature_pattern = re.compile(
        r"^(\s*)(?!static\b)(?!enum\b)(?!struct\b)"
        r"([A-Za-z_][\w:<>,\s\*&~]*?)\s+([A-Za-z_]\w*)\s*\(([^;]*)\)\s*(const)?(\s*\{.*)?$"
    )

    for line in lines:
        stripped = line.strip()
        if stripped == MUTATING_MARKER:
            next_method_is_mutating = True
            continue

        if brace_depth == 0:
            match = signature_pattern.match(line)
            if match:
                method_name = match.group(3)
                has_const = match.group(5) is not None
                has_body = match.group(6) is not None
                is_constructor = struct_name is not None and method_name == struct_name

                if (not next_method_is_mutating) and (not has_const) and (not is_constructor):
                    if has_body:
                        line = line.replace("{", " const {", 1)
                    else:
                        line = f"{line} const"

                next_method_is_mutating = False

        brace_depth += line.count("{")
        brace_depth -= line.count("}")
        if brace_depth < 0:
            brace_depth = 0

        out_lines.append(line)

    return "\n".join(out_lines)


def transform_slang_code(code: str, struct_name: str | None = None) -> str:
    transformed = code.replace("\r\n", "\n").replace("\r", "\n")

    transformed = re.sub(r"\[\s*mutating\s*\]\s*", f"{MUTATING_MARKER}\n", transformed)
    transformed = re.sub(r"(?m)^\s*\[[^\]]+\]\s*", "", transformed)
    transformed = re.sub(r"\bSETTER_DECL\b", "", transformed)
    transformed = re.sub(r"\bCONST_FUNCTION\b", "const", transformed)
    transformed = transformed.replace("this.", "this->")

    transformed = re.sub(r"(\d+\.\d+|\d+)h\b", r"\1f", transformed)

    if struct_name:
        transformed = re.sub(r"\b__init\s*\(", f"{struct_name}(", transformed)

    type_keys = sorted(SLANG_TO_CPP_TYPES.keys(), key=len, reverse=True)
    for slang_type in type_keys:
        cpp_type = SLANG_TO_CPP_TYPES[slang_type]
        transformed = re.sub(rf"\b{re.escape(slang_type)}\b", cpp_type, transformed)

    for slang_type, cpp_type in CAST_TYPE_MAP.items():
        transformed = re.sub(
            rf"\b{re.escape(slang_type)}\s*\(",
            f"static_cast<{cpp_type}>(",
            transformed,
        )

    transformed = add_const_to_non_mutating_methods(transformed, struct_name)
    transformed = re.sub(r"\n{3,}", "\n\n", transformed)
    return transformed.strip("\n")


def parse_file(path: Path) -> ParsedFile:
    content = path.read_text(encoding="utf-8")
    imports = parse_imports(content)
    import_lines = parse_import_lines(content)
    enums, structs, constants = parse_exported_declarations(content)
    return ParsedFile(
        path=path,
        imports=imports,
        import_lines=import_lines,
        enums=enums,
        structs=structs,
        constants=constants,
    )


def is_flag_enum(enum: EnumDecl) -> bool:
    body = enum.body
    return ("0x" in body) or ("<<" in body) or ("|" in body)


def indent_block(text: str, spaces: int) -> list[str]:
    pad = " " * spaces
    lines = text.split("\n")
    return [f"{pad}{line}".rstrip() if line else "" for line in lines]


def generate_header(
    parsed: ParsedFile,
    output_path: Path,
    dependency_outputs: list[Path],
) -> str:
    lines: list[str] = []
    lines.append(f"// AUTO-GENERATED from {parsed.path.as_posix()} - DO NOT EDIT")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <core/math/type.hpp>")
    lines.append("#include <core/tools/enum-flags.hpp>")
    lines.append("#include <glm/gtc/packing.hpp>")
    lines.append("#include <cstdint>")
    lines.append("")

    dep_includes = sorted({dep for dep in dependency_outputs if dep != output_path})
    for dep in dep_includes:
        rel = os.path.relpath(dep, output_path.parent).replace("\\", "/")
        lines.append(f'#include "{rel}"')
    if dep_includes:
        lines.append("")

    lines.append("#ifndef APRIL_GRAPHICS_GENERATED_SLANG_HELPERS")
    lines.append("#define APRIL_GRAPHICS_GENERATED_SLANG_HELPERS")
    lines.append("namespace april::graphics::inline generated")
    lines.append("{")
    lines.append("    inline auto PACK_BITS(uint32_t numBits, uint32_t offset, uint32_t value, uint32_t field) -> uint32_t")
    lines.append("    {")
    lines.append("        if (numBits == 0)")
    lines.append("        {")
    lines.append("            return value;")
    lines.append("        }")
    lines.append("        auto const fieldMask = numBits >= 32 ? 0xffffffffu : ((1u << numBits) - 1u);")
    lines.append("        auto const mask = fieldMask << offset;")
    lines.append("        return (value & ~mask) | ((field << offset) & mask);")
    lines.append("    }")
    lines.append("")
    lines.append("    inline auto PACK_BITS_UNSAFE(uint32_t numBits, uint32_t offset, uint32_t value, uint32_t field) -> uint32_t")
    lines.append("    {")
    lines.append("        return PACK_BITS(numBits, offset, value, field);")
    lines.append("    }")
    lines.append("")
    lines.append("    inline auto EXTRACT_BITS(uint32_t numBits, uint32_t offset, uint32_t value) -> uint32_t")
    lines.append("    {")
    lines.append("        if (numBits == 0)")
    lines.append("        {")
    lines.append("            return 0u;")
    lines.append("        }")
    lines.append("        auto const fieldMask = numBits >= 32 ? 0xffffffffu : ((1u << numBits) - 1u);")
    lines.append("        return (value >> offset) & fieldMask;")
    lines.append("    }")
    lines.append("")
    lines.append("    inline auto IS_BIT_SET(uint32_t value, uint32_t bit) -> bool")
    lines.append("    {")
    lines.append("        return (value & (1u << bit)) != 0u;")
    lines.append("    }")
    lines.append("")
    lines.append("    inline auto asuint16(float value) -> uint16_t")
    lines.append("    {")
    lines.append("        auto const packed = glm::packHalf2x16(float2(value, 0.0f));")
    lines.append("        return static_cast<uint16_t>(packed & 0xffffu);")
    lines.append("    }")
    lines.append("")
    lines.append("    inline auto asfloat16(uint16_t value) -> float")
    lines.append("    {")
    lines.append("        auto const unpacked = glm::unpackHalf2x16(static_cast<uint32_t>(value));")
    lines.append("        return unpacked.x;")
    lines.append("    }")
    lines.append("")
    lines.append("    struct float16_t")
    lines.append("    {")
    lines.append("        uint16_t bits{0};")
    lines.append("")
    lines.append("        float16_t() = default;")
    lines.append("        float16_t(float value)")
    lines.append("            : bits(asuint16(value))")
    lines.append("        {")
    lines.append("        }")
    lines.append("")
    lines.append("        auto operator=(float value) -> float16_t&")
    lines.append("        {")
    lines.append("            bits = asuint16(value);")
    lines.append("            return *this;")
    lines.append("        }")
    lines.append("")
    lines.append("        operator float() const")
    lines.append("        {")
    lines.append("            return asfloat16(bits);")
    lines.append("        }")
    lines.append("    };")
    lines.append("")
    lines.append("    struct float16_t2")
    lines.append("    {")
    lines.append("        uint16_t x{0};")
    lines.append("        uint16_t y{0};")
    lines.append("")
    lines.append("        float16_t2() = default;")
    lines.append("        float16_t2(float value)")
    lines.append("            : x(asuint16(value))")
    lines.append("            , y(asuint16(value))")
    lines.append("        {")
    lines.append("        }")
    lines.append("        float16_t2(float2 const& value)")
    lines.append("            : x(asuint16(value.x))")
    lines.append("            , y(asuint16(value.y))")
    lines.append("        {")
    lines.append("        }")
    lines.append("")
    lines.append("        auto operator=(float2 const& value) -> float16_t2&")
    lines.append("        {")
    lines.append("            x = asuint16(value.x);")
    lines.append("            y = asuint16(value.y);")
    lines.append("            return *this;")
    lines.append("        }")
    lines.append("")
    lines.append("        operator float2() const")
    lines.append("        {")
    lines.append("            return float2(asfloat16(x), asfloat16(y));")
    lines.append("        }")
    lines.append("    };")
    lines.append("")
    lines.append("    struct float16_t3")
    lines.append("    {")
    lines.append("        uint16_t x{0};")
    lines.append("        uint16_t y{0};")
    lines.append("        uint16_t z{0};")
    lines.append("")
    lines.append("        float16_t3() = default;")
    lines.append("        float16_t3(float value)")
    lines.append("            : x(asuint16(value))")
    lines.append("            , y(asuint16(value))")
    lines.append("            , z(asuint16(value))")
    lines.append("        {")
    lines.append("        }")
    lines.append("        float16_t3(float3 const& value)")
    lines.append("            : x(asuint16(value.x))")
    lines.append("            , y(asuint16(value.y))")
    lines.append("            , z(asuint16(value.z))")
    lines.append("        {")
    lines.append("        }")
    lines.append("")
    lines.append("        auto operator=(float3 const& value) -> float16_t3&")
    lines.append("        {")
    lines.append("            x = asuint16(value.x);")
    lines.append("            y = asuint16(value.y);")
    lines.append("            z = asuint16(value.z);")
    lines.append("            return *this;")
    lines.append("        }")
    lines.append("")
    lines.append("        operator float3() const")
    lines.append("        {")
    lines.append("            return float3(asfloat16(x), asfloat16(y), asfloat16(z));")
    lines.append("        }")
    lines.append("    };")
    lines.append("")
    lines.append("    struct float16_t4")
    lines.append("    {")
    lines.append("        uint16_t x{0};")
    lines.append("        uint16_t y{0};")
    lines.append("        uint16_t z{0};")
    lines.append("        uint16_t w{0};")
    lines.append("")
    lines.append("        float16_t4() = default;")
    lines.append("        float16_t4(float value)")
    lines.append("            : x(asuint16(value))")
    lines.append("            , y(asuint16(value))")
    lines.append("            , z(asuint16(value))")
    lines.append("            , w(asuint16(value))")
    lines.append("        {")
    lines.append("        }")
    lines.append("        float16_t4(float4 const& value)")
    lines.append("            : x(asuint16(value.x))")
    lines.append("            , y(asuint16(value.y))")
    lines.append("            , z(asuint16(value.z))")
    lines.append("            , w(asuint16(value.w))")
    lines.append("        {")
    lines.append("        }")
    lines.append("")
    lines.append("        auto operator=(float4 const& value) -> float16_t4&")
    lines.append("        {")
    lines.append("            x = asuint16(value.x);")
    lines.append("            y = asuint16(value.y);")
    lines.append("            z = asuint16(value.z);")
    lines.append("            w = asuint16(value.w);")
    lines.append("            return *this;")
    lines.append("        }")
    lines.append("")
    lines.append("        operator float4() const")
    lines.append("        {")
    lines.append("            return float4(asfloat16(x), asfloat16(y), asfloat16(z), asfloat16(w));")
    lines.append("        }")
    lines.append("    };")
    lines.append("")
    lines.append("    inline auto asuint16(float16_t value) -> uint16_t")
    lines.append("    {")
    lines.append("        return value.bits;")
    lines.append("    }")
    lines.append("} // namespace april::graphics::inline generated")
    lines.append("#endif")
    lines.append("")

    lines.append("namespace april::graphics::inline generated")
    lines.append("{")

    for const in parsed.constants:
        cpp_type = map_type_name(const.type_name)
        cpp_value = transform_slang_code(const.value)
        lines.append(f"    inline constexpr {cpp_type} {const.cpp_name} = {cpp_value};")
    if parsed.constants:
        lines.append("")

    for enum in parsed.enums:
        backing_type = map_type_name(enum.backing_type or "uint")
        body = transform_slang_code(enum.body)
        lines.append(f"    enum class {enum.cpp_name} : {backing_type}")
        lines.append("    {")
        lines.extend(indent_block(body, 8))
        lines.append("    };")
        if is_flag_enum(enum):
            lines.append(f"    AP_ENUM_CLASS_OPERATORS({enum.cpp_name});")
        lines.append("")

    for struct in parsed.structs:
        body = transform_slang_code(struct.body, struct_name=struct.cpp_name)
        lines.append(f"    struct {struct.cpp_name}")
        lines.append("    {")
        if body:
            lines.extend(indent_block(body, 8))
        lines.append("    };")
        lines.append("")

    lines.append("} // namespace april::graphics::inline generated")
    lines.append("")
    return "\n".join(lines)


def map_shared_output_name(filename: str, extension: str) -> str:
    marker = ".shared.slang"
    if filename.endswith(marker):
        return f"{filename[:-len(marker)]}.generated{extension}"
    stem = filename.rsplit(".", 1)[0]
    return f"{stem}.generated{extension}"


def generate_shared_slang(
    parsed: ParsedFile,
    shader_output_path: Path,
    dependency_outputs: list[Path],
) -> str:
    lines: list[str] = []
    lines.append(f"// AUTO-GENERATED from {parsed.path.as_posix()} - DO NOT EDIT")
    lines.append("// Generated shader-side declarations from shared schema.")
    lines.append("")

    dep_imports = sorted({dep for dep in dependency_outputs if dep != shader_output_path})
    for dep in dep_imports:
        module = dep.stem.replace(".generated", "")
        module_name = module.replace("-", "_")
        lines.append(f"import material.generated.{module_name};")

    if dep_imports and parsed.import_lines:
        lines.append("")

    for import_line in parsed.import_lines:
        lines.append(import_line)

    if parsed.import_lines:
        lines.append("")

    for const in parsed.constants:
        lines.append(f"static const {const.type_name} {const.name} = {const.value};")
    if parsed.constants:
        lines.append("")

    for enum in parsed.enums:
        backing = f" : {enum.backing_type}" if enum.backing_type else ""
        lines.append(f"enum class {enum.name}{backing}")
        lines.append("{")
        lines.extend(indent_block(enum.body, 4))
        lines.append("};")
        lines.append("")

    for struct in parsed.structs:
        lines.append(f"struct {struct.name}")
        lines.append("{")
        if struct.body:
            lines.extend(indent_block(struct.body, 4))
        lines.append("};")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def process_shared_directory(shared_input_dir: Path, shader_output_dir: Path, cpp_output_dir: Path) -> int:
    shared_files = sorted(shared_input_dir.glob("**/*.shared.slang"))
    parsed_files = [parse_file(path) for path in shared_files]
    exported = [parsed for parsed in parsed_files if parsed.has_exports]

    file_to_cpp_output: dict[Path, Path] = {}
    file_to_shader_output: dict[Path, Path] = {}
    module_to_cpp_output: dict[str, Path] = {}
    module_to_shader_output: dict[str, Path] = {}

    for parsed in exported:
        rel = parsed.path.relative_to(shared_input_dir)
        cpp_name = map_shared_output_name(rel.name, ".hpp")
        shader_name = map_shared_output_name(rel.name, ".slang")
        cpp_out = (cpp_output_dir / rel.parent / cpp_name)
        shader_out = (shader_output_dir / rel.parent / shader_name)

        file_to_cpp_output[parsed.path] = cpp_out
        file_to_shader_output[parsed.path] = shader_out

        module_key = normalize_module_name(parsed.path.stem.replace(".shared", ""))
        module_to_cpp_output[module_key] = cpp_out
        module_to_shader_output[module_key] = shader_out

    generated_count = 0
    for parsed in exported:
        cpp_out = file_to_cpp_output[parsed.path]
        shader_out = file_to_shader_output[parsed.path]

        cpp_dependencies = [module_to_cpp_output[module] for module in parsed.imports if module in module_to_cpp_output]
        shader_dependencies = [module_to_shader_output[module] for module in parsed.imports if module in module_to_shader_output]

        cpp_generated = generate_header(parsed, cpp_out, cpp_dependencies)
        shader_generated = generate_shared_slang(parsed, shader_out, shader_dependencies)

        cpp_out.parent.mkdir(parents=True, exist_ok=True)
        shader_out.parent.mkdir(parents=True, exist_ok=True)

        cpp_out.write_text(cpp_generated, encoding="utf-8")
        shader_out.write_text(shader_generated, encoding="utf-8")

        print(f"Generated {cpp_out}")
        print(f"Generated {shader_out}")
        generated_count += 1

    for parsed in parsed_files:
        if not parsed.has_exports:
            print(f"No @export-cpp annotations found in {parsed.path}")

    return generated_count


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate C++ and shader declarations from shared Slang schema files")
    parser.add_argument("--shared-input-dir", type=str, help="Input directory containing shared Slang schema files")
    parser.add_argument("--shared-shader-output-dir", type=str, help="Output directory for generated shader Slang files")
    parser.add_argument("--shared-cpp-output-dir", type=str, help="Output directory for generated C++ headers")
    args = parser.parse_args()

    if args.shared_input_dir and args.shared_shader_output_dir and args.shared_cpp_output_dir:
        process_shared_directory(
            Path(args.shared_input_dir),
            Path(args.shared_shader_output_dir),
            Path(args.shared_cpp_output_dir),
        )
        return

    parser.error("Specify --shared-input-dir, --shared-shader-output-dir, and --shared-cpp-output-dir")


if __name__ == "__main__":
    main()
