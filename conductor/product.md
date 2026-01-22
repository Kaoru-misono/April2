# Product Guide - April Engine

## Initial Concept
A C++23 modular game engine focused on high-performance desktop 3D graphics and graphics research.

## Vision
April is designed to be a cutting-edge C++23 game engine that serves as a high-performance desktop 3D graphics platform and a specialized research environment. It leverages the latest language features and modern rendering techniques to provide a robust yet flexible foundation for advanced graphics development.

## Core Technical Pillars
- **C++23 Excellence:** Extensive use of C++23 standards, including Modules and Concepts, to ensure a modern, type-safe, and performant codebase.
- **Modern Shader Pipeline:** A shader-first graphics architecture built around Slang and an advanced Render Hardware Interface (RHI) to simplify complex cross-API development.
- **Modular Architecture:** A clean, decoupled structure that allows for rapid prototyping, easy experimentation with new rendering techniques, and specialized research applications.

## Key Features
- **Advanced Material System:** Integration of a powerful material system with Slang-based shader generation for dynamic and efficient rendering pipelines.
- **Modern Asset Pipeline:** Automated support for industry-standard formats like glTF 2.0 to streamline the import and management of 3D assets.
- **Integrated Ray Tracing:** Native support for Real-time Ray Tracing and Path Tracing, enabling state-of-the-art lighting and global illumination.
- **Game Editor Foundation:** A modular, professional-grade editor built with Dear ImGui, featuring a docking workspace, high-level UI components (Camera, Tonemapper, Property Editor), offscreen viewport rendering, and a high-performance multi-threaded CPU/GPU Profiler with hierarchical tracking, ImPlot visualization (Line/Bar/Pie charts), and Chrome Tracing export.

## Target Audience
April is primarily targeted at **hobbyists** interested in mastering C++23 and exploring advanced graphics APIs through hands-on development in a modern engine environment.

## Rendering Goals
The initial functional milestones aim to achieve a versatile rendering output:
- **Photorealistic PBR:** Leveraging integrated ray tracing for physically accurate lighting, shadows, and reflections.
- **High-Performance Stylized Rendering (NPR):** Providing the flexibility to implement efficient non-photorealistic techniques alongside standard PBR.
