# Plan: Comprehensive Testing for Graphics Program Module

## Phase 1: Test Infrastructure [checkpoint: 0fd2201]
- [x] Task: Integrate Test Framework (doctest) 1593f45
    - [x] Subtask: Add `doctest.h` to the project (e.g., in `engine/test/external/`).
    - [x] Subtask: Create `engine/test/test-main.cpp` to initialize the unified test runner.
    - [x] Subtask: Update `engine/test/CMakeLists.txt` to build a `test-suite` executable linking `April_graphics`.
- [x] Task: Conductor - User Manual Verification 'Test Infrastructure' (Protocol in workflow.md)

## Phase 2: Compilation & Configuration Tests [checkpoint: ebe4ac5]
- [x] Task: Implement Shader Compilation Tests 48124d5
    - [x] Subtask: Write unit tests verifying `Program` can be created from source strings.
    - [x] Subtask: Write tests verifying `ProgramManager` successfully compiles valid Slang code.
    - [x] Subtask: Write tests verifying compilation failure reporting for invalid Slang code.
- [x] Task: Implement Define & Macro Tests 397c4bb
    - [x] Subtask: Write tests for `addDefine` and `removeDefine` functionality.
    - [x] Subtask: Verify that changing defines correctly triggers program re-linkage (Red/Green phase).
- [x] Task: Conductor - User Manual Verification 'Compilation & Configuration Tests' (Protocol in workflow.md)
    - [ ] Subtask: Verify that changing defines correctly triggers program re-linkage (Red/Green phase).
- [ ] Task: Conductor - User Manual Verification 'Compilation & Configuration Tests' (Protocol in workflow.md)

## Phase 3: Reflection & Resource Mapping Tests [checkpoint: 1b91820]
- [x] Task: Implement Reflection Tests 89f198b
    - [x] Subtask: Create test shaders with various resources (ConstantBuffers, Textures, Samplers).
    - [x] Subtask: Write tests verifying `ProgramReflection` correctly identifies all resource ranges and types.
- [x] Task: Implement Resource Binding Tests 75a5e22
    - [x] Subtask: Write tests verifying `ProgramVariables` correctly binds buffers and textures to the RHI shader objects.
- [x] Task: Conductor - User Manual Verification 'Reflection & Resource Mapping Tests' (Protocol in workflow.md)

## Phase 4: System Features (Reloading & Type Conformance) [checkpoint: none]
- [x] Task: Implement Hot Reloading Tests 0f3b18e
    - [x] Subtask: Write tests verifying `ProgramManager::reloadAllPrograms` correctly resets and re-links programs.
- [x] Task: Implement Type Conformance Tests 95a3d7b
    - [x] Subtask: Write tests verifying `addTypeConformance` works for linking interface implementations.
- [x] Task: Conductor - User Manual Verification 'System Features' (Protocol in workflow.md)

## Phase 5: Advanced Scenarios [checkpoint: 95df43d]
- [x] Task: Implement Advanced Program Tests 95df43d
    - [x] Subtask: Write tests for Ray Tracing hit group reflection and creation.
    - [x] Subtask: Write tests verifying `ProgramManager` global state propagation.
    - [x] Subtask: Write tests for nested `ParameterBlock` reflection.
- [x] Task: Conductor - User Manual Verification 'Advanced Scenarios' (Protocol in workflow.md)
