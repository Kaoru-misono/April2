#!/usr/bin/env python3
"""
Parses Slang files and generates matching C++ headers.
Structs and enums marked with @export-cpp annotation are converted.

Usage:
    python slang-codegen.py --input path/to/file.slang --output path/to/file.generated.hpp
    python slang-codegen.py --input-dir path/to/shaders --output-dir path/to/generated
"""

import re
import argparse
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

# Type mapping from Slang to C++
SLANG_TO_CPP_TYPES = {
    # Scalar types
    'uint': 'uint32_t',
    'int': 'int32_t',
    'float': 'float',
    'bool': 'bool',
    'half': 'uint16_t',  # Use uint16_t for half precision storage

    # Vector types
    'float2': 'float2',
    'float3': 'float3',
    'float4': 'float4',
    'int2': 'int2',
    'int3': 'int3',
    'int4': 'int4',
    'uint2': 'uint2',
    'uint3': 'uint3',
    'uint4': 'uint4',
    'bool2': 'bool2',
    'bool3': 'bool3',
    'bool4': 'bool4',

    # Matrix types
    'float2x2': 'float2x2',
    'float3x3': 'float3x3',
    'float4x4': 'float4x4',
}

# Size of types in bytes for alignment calculation
TYPE_SIZES = {
    'uint32_t': 4,
    'int32_t': 4,
    'float': 4,
    'bool': 4,  # In GPU structs, bool is typically 4 bytes
    'uint16_t': 2,
    'float2': 8,
    'float3': 12,
    'float4': 16,
    'int2': 8,
    'int3': 12,
    'int4': 16,
    'uint2': 8,
    'uint3': 12,
    'uint4': 16,
    'bool2': 8,
    'bool3': 12,
    'bool4': 16,
    'float2x2': 16,
    'float3x3': 48,  # 3 float4 rows
    'float4x4': 64,
}


@dataclass
class EnumValue:
    name: str
    value: Optional[str]


@dataclass
class EnumDef:
    cpp_name: str
    slang_name: str
    values: list[EnumValue]
    backing_type: str = 'uint32_t'


@dataclass
class Field:
    type: str
    name: str
    array_size: Optional[int] = None


@dataclass
class StructDef:
    cpp_name: str
    slang_name: str
    fields: list[Field]


def parse_enum_values(body: str) -> list[EnumValue]:
    """Parse enum values from the body of an enum definition."""
    values = []
    for line in body.strip().split('\n'):
        line = line.strip().rstrip(',')
        if not line or line.startswith('//'):
            continue

        # Parse name = value or just name
        if '=' in line:
            parts = line.split('=', 1)
            name = parts[0].strip()
            value = parts[1].strip()
        else:
            name = line
            value = None

        if name:
            values.append(EnumValue(name=name, value=value))

    return values


def parse_slang_enums(content: str) -> list[EnumDef]:
    """Extract enums marked with @export-cpp."""
    # Match enum class with optional backing type
    pattern = r'//\s*@export-cpp(?::\s*(\w+))?\s*\n\s*enum\s+(?:class\s+)?(\w+)(?:\s*:\s*(\w+))?\s*\{([^}]+)\}'
    matches = re.findall(pattern, content, re.MULTILINE | re.DOTALL)

    enums = []
    for cpp_name, slang_name, backing_type, body in matches:
        if not cpp_name:
            cpp_name = slang_name
        backing_type_cpp: str
        if not backing_type:
            backing_type_cpp = 'uint32_t'
        else:
            backing_type_cpp = SLANG_TO_CPP_TYPES.get(backing_type, str(backing_type))

        values = parse_enum_values(body)
        enums.append(EnumDef(
            cpp_name=cpp_name,
            slang_name=slang_name,
            values=values,
            backing_type=backing_type_cpp
        ))

    return enums


def parse_struct_fields(body: str) -> list[Field]:
    """Parse fields from the body of a struct definition."""
    fields = []
    for line in body.strip().split('\n'):
        # Strip whitespace
        line = line.strip()

        # Skip empty lines and comments
        if not line or line.startswith('//'):
            continue

        # Remove inline comments (// ...)
        if '//' in line:
            line = line[:line.index('//')].strip()

        # Skip functions/methods
        if '(' in line:
            continue

        # Remove trailing semicolon
        line = line.rstrip(';').strip()

        if not line:
            continue

        # Parse array type: type name[size]
        array_match = re.match(r'(\w+)\s+(\w+)\[(\d+)\]', line)
        if array_match:
            field_type = array_match.group(1)
            field_name = array_match.group(2)
            array_size = int(array_match.group(3))
            cpp_type = SLANG_TO_CPP_TYPES.get(field_type, field_type)
            fields.append(Field(type=cpp_type, name=field_name, array_size=array_size))
            continue

        # Parse simple type: type name
        parts = line.split()
        if len(parts) >= 2:
            field_type = parts[0]
            field_name = parts[1].rstrip(';')  # Extra safety for semicolons
            cpp_type = SLANG_TO_CPP_TYPES.get(field_type, field_type)
            fields.append(Field(type=cpp_type, name=field_name))

    return fields


def parse_slang_structs(content: str) -> list[StructDef]:
    """Extract structs marked with @export-cpp."""
    pattern = r'//\s*@export-cpp(?::\s*(\w+))?\s*\n\s*struct\s+(\w+)(?:\s*:\s*[^{]+)?\s*\{([^}]+)\}'
    matches = re.findall(pattern, content, re.MULTILINE | re.DOTALL)

    structs = []
    for cpp_name, slang_name, body in matches:
        if not cpp_name:
            cpp_name = slang_name

        fields = parse_struct_fields(body)
        if fields:  # Only add if we found valid fields
            structs.append(StructDef(
                cpp_name=cpp_name,
                slang_name=slang_name,
                fields=fields
            ))

    return structs


def get_field_size(field: Field, type_sizes: dict[str, int]) -> int:
    """Return field size in bytes using known scalar/vector/struct sizes."""
    size = type_sizes.get(field.type, 4)
    if field.array_size:
        size *= field.array_size
    return size


def calculate_struct_size(fields: list[Field], type_sizes: dict[str, int]) -> int:
    """Calculate struct size in bytes using a simple linear packing model."""
    total = 0
    for field in fields:
        total += get_field_size(field, type_sizes)
    return ((total + 15) // 16) * 16


def calculate_field_offsets(fields: list[Field], type_sizes: dict[str, int]) -> list[int]:
    """Calculate byte offsets for fields using the same packing model as calculate_struct_size()."""
    offsets: list[int] = []
    current_offset = 0
    for field in fields:
        offsets.append(current_offset)
        current_offset += get_field_size(field, type_sizes)
    return offsets


def is_flags_enum(enum: EnumDef) -> bool:
    """Check if an enum is a flags enum (has bitwise values)."""
    # Check if the enum name contains "Flags" or "Type" with bit values
    if 'Flags' in enum.cpp_name or 'Type' in enum.cpp_name:
        for val in enum.values:
            if val.value and ('0x' in val.value or '<<' in val.value):
                return True
    return False


def generate_cpp_header(enums: list[EnumDef], structs: list[StructDef], source_file: str) -> str:
    """Generate C++ header from parsed enums and structs."""
    lines = [
        f'// AUTO-GENERATED from {source_file} - DO NOT EDIT',
        '#pragma once',
        '',
        '#include <core/math/type.hpp>',
        '#include <core/tools/enum-flags.hpp>',
        '#include <cstddef>',
        '#include <cstdint>',
        '',
        'namespace april::graphics::inline generated',
        '{',
        '',
    ]

    # Generate enums
    for enum in enums:
        lines.append(f'    enum class {enum.cpp_name} : {enum.backing_type}')
        lines.append('    {')
        for val in enum.values:
            if val.value is not None:
                lines.append(f'        {val.name} = {val.value},')
            else:
                lines.append(f'        {val.name},')
        lines.append('    };')

        # Add AP_ENUM_CLASS_OPERATORS for flag enums
        if is_flags_enum(enum):
            lines.append(f'    AP_ENUM_CLASS_OPERATORS({enum.cpp_name});')

        lines.append('')

    # Resolve struct sizes/offsets, including nested struct fields.
    type_sizes = dict(TYPE_SIZES)
    struct_layouts: dict[str, tuple[int, list[int]]] = {}
    unresolved = list(structs)

    while unresolved:
        remaining: list[StructDef] = []
        progressed = False

        for s in unresolved:
            field_types_known = all((field.type in type_sizes) or (field.type == s.cpp_name) for field in s.fields)
            if not field_types_known:
                remaining.append(s)
                continue

            size = calculate_struct_size(s.fields, type_sizes)
            offsets = calculate_field_offsets(s.fields, type_sizes)
            struct_layouts[s.cpp_name] = (size, offsets)
            type_sizes[s.cpp_name] = size
            progressed = True

        if not progressed:
            unresolved_names = ', '.join(s.cpp_name for s in remaining)
            raise ValueError(f'Unable to resolve struct sizes for: {unresolved_names}')

        unresolved = remaining

    # Generate structs
    for s in structs:
        size, offsets = struct_layouts[s.cpp_name]
        lines.append(f'    struct alignas(16) {s.cpp_name}')
        lines.append('    {')
        for f in s.fields:
            if f.array_size:
                lines.append(f'        {f.type} {f.name}[{f.array_size}];')
            else:
                lines.append(f'        {f.type} {f.name};')
        lines.append('    };')
        lines.append(f'    static_assert(sizeof({s.cpp_name}) == {size}, "Size mismatch for {s.cpp_name}");')
        for field, offset in zip(s.fields, offsets):
            lines.append(
                f'    static_assert(offsetof({s.cpp_name}, {field.name}) == {offset}, "Offset mismatch for {s.cpp_name}::{field.name}");'
            )
        lines.append('')

    lines.append('} // namespace april::graphics::inline generated')
    lines.append('')
    return '\n'.join(lines)


def process_file(input_path: Path, output_path: Path) -> bool:
    """Process a single Slang file and generate a C++ header."""
    content = input_path.read_text(encoding='utf-8')

    enums = parse_slang_enums(content)
    structs = parse_slang_structs(content)

    if not enums and not structs:
        print(f"No @export-cpp annotations found in {input_path}")
        return False

    cpp_code = generate_cpp_header(enums, structs, input_path.name)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(cpp_code, encoding='utf-8')
    print(f"Generated {output_path}")
    return True


def main():
    parser = argparse.ArgumentParser(description='Generate C++ headers from Slang files')
    parser.add_argument('--input', type=str, help='Input Slang file')
    parser.add_argument('--output', type=str, help='Output C++ header file')
    parser.add_argument('--input-dir', type=str, help='Input directory containing Slang files')
    parser.add_argument('--output-dir', type=str, help='Output directory for generated headers')
    args = parser.parse_args()

    if args.input and args.output:
        # Single file mode
        process_file(Path(args.input), Path(args.output))
    elif args.input_dir and args.output_dir:
        # Directory mode
        input_dir = Path(args.input_dir)
        output_dir = Path(args.output_dir)

        for slang_file in input_dir.glob('**/*.slang'):
            # Generate output path with same relative structure
            rel_path = slang_file.relative_to(input_dir)
            output_path = output_dir / rel_path.with_suffix('.generated.hpp')
            process_file(slang_file, output_path)
    else:
        parser.error('Either --input/--output or --input-dir/--output-dir must be specified')


if __name__ == '__main__':
    main()
