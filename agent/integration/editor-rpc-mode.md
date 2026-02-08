# Editor ↔ Engine RPC (cross-module protocol)

> Serialization can be JSON / msgpack / protobuf. This doc defines semantics + fields.

## Version
- protocol_version: 1

## Envelope (example)
- request: { id, method, params }
- response: { id, ok, result?, error? }

## Methods
### GetSceneGraph
- params: {}
- result: { root_id, nodes: [{id, name, parent_id, type}] }

### GetObjectProperties
- params: { object_id }
- result: { object_id, properties: [{path, type, value, readonly, meta}] }

### SetProperty
- params: { object_id, path, value }
- result: { applied: true }

### AddComponent / RemoveComponent
- params: { object_id, component_type }
- result: { ok: true }

### Undo / Redo
- params: {}
- result: { ok: true }

### Play / Pause / Step
- params: {}
- result: { state: "playing|paused" }

## Errors
- error: { code, message, data? }
