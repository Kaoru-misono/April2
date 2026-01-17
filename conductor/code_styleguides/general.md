# General Code Style Principles

This document outlines general coding principles that apply across all languages and frameworks used in this project.

## Readability
- Code should be easy to read and understand by humans.
- Avoid overly clever or obscure constructs.

## Consistency
- Follow existing patterns in the codebase.
- Maintain consistent formatting, naming, and structure.

## Simplicity
- Prefer simple solutions over complex ones.
- Break down complex problems into smaller, manageable parts.

## Maintainability
- Write code that is easy to modify and extend.
- Minimize dependencies and coupling.

## Documentation
- Document *why* something is done, not just *what*.
- Keep documentation up-to-date with code changes.

## Encapsulation Hierarchy (CRITICAL)
- Do NOT bypass internal abstractions: If a third-party library (e.g., GLFW, ImGui, Slang) is wrapped by an internal engine class, you MUST use the internal class.
- Check before Import: Before including a raw library header (e.g., #include <vulkan/vulkan.h>), verify if an engine header (e.g., #include "rhi/vulkan_types.h") already exists.
- Extension Pattern: If the internal wrapper lacks a specific feature you need, do not revert to raw library usage elsewhere. Instead, extend the wrapper to support the new feature, then use the updated wrapper.
