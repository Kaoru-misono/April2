# Asset Format (cross-module contract)

## Goal
Define how assets are identified, located, versioned, and dependency-tracked across editor/import/runtime.

## Asset ID
- GUID: globally unique, stable even if path changes
- Path: mutable, used for discovery/display, not identity

## Manifest fields (example)
- guid
- type (texture/model/material/scene/...)
- source_path
- imported_path
- importer
- import_settings
- deps: [guid...]
- version: int

## Versioning
- Any binary import output format change => version++
- runtime loader handles version or reports a clear error (see error-model.md)
