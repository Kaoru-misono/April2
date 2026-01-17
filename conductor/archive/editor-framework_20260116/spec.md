# Specification: Editor Framework & Widget Integration

## 1. Overview
Implement the core `EditorSystem` to provide a professional-grade graphical user interface for the April Engine. This track focuses on establishing the docking workspace, a dedicated scene viewport, and integrating advanced property inspection and rendering widgets, utilizing the architecture and patterns from the `nvapp` and `nvgui` libraries.

## 2. Functional Requirements

### 2.1 Editor Core & Workspace (Reference: `nvapp`)
- **AppElement Architecture:** Implement a modular interface similar to `nvapp::IAppElement` (with methods like `onAttach`, `onDetach`, `onUIRender`, `onRender`) to decouple editor tools.
- **Docking System:** Port the robust docking setup from `nvapp::Application::setupImguiDock`, ensuring a "Viewport" central node and customizable layout.
- **Styling:** Adopt the visual style (colors, rounding, fonts) defined in `nvgui/style.hpp` and `nvgui/fonts.hpp`.

### 2.2 Scene Viewport
- **Offscreen Rendering:** Render the 3D scene to an offscreen framebuffer.
- **Viewport Element:** Implement the Viewport as an `AppElement`.
- **Camera Control:** Implement camera navigation (orbit/pan) adapting logic from `nvgui` or `nvapp`, likely as a separate attached element or integrated into the Viewport element.

### 2.3 Property Inspection
- **Property Inspector Element:** Create a dedicated `AppElement` for property inspection.
- **Integration:** Adapt logic from `nvgui/property_editor.cpp` to inspect and modify object properties.

### 2.4 Rendering Widgets (nvgui)
- **Rendering Control Element:** Create an `AppElement` to host rendering controls.
- **Tone Mapper:** Integrate `tonemapperWidget` (`nvgui/tonemapper.hpp`).
- **Sky Controls:** Integrate `skySimpleParametersUI` (`nvgui/sky.hpp`).

## 3. Non-Functional Requirements
- **Performance:** Editor UI rendering should not significantly degrade 3D rendering performance.
- **Modularity:** Strict adherence to the `IAppElement` pattern for all new editor components.
- **Code Style:** Follow project conventions and the `nvapp`/`nvgui` architectural patterns.

## 4. Acceptance Criteria
- [ ] An `IAppElement`-like interface is defined and integrated into the Engine's update loop.
- [ ] The editor launches with a central docking space matching `nvapp`'s layout behavior.
- [ ] A "Scene" Viewport is implemented as an Element, displaying the 3D render.
- [ ] Camera control works within the Viewport.
- [ ] A "Properties" Element allows editing of basic object transforms.
- [ ] A "Rendering" Element hosts Tone Mapper and Sky widgets, updating scene parameters.
- [ ] Visual style matches `nvgui`.

## 5. Out of Scope
- Asset Browser (deferred).
- Log/Console Panel (deferred).
- Replacing the entire `Engine` class with `nvapp::Application`.
